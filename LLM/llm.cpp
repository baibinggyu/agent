#include <boost/asio.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/detail/call_stack.hpp>
#include <boost/asio/error.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/asio/ssl/stream_base.hpp>
#include <boost/asio/ssl/verify_mode.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/core/detail/config.hpp>
#include <boost/beast/core/error.hpp>
#include <boost/beast/core/stream_traits.hpp>
#include <boost/beast/core/tcp_stream.hpp>
#include <boost/beast/http/field.hpp>
#include <boost/beast/http/impl/read.hpp>
#include <boost/beast/http/impl/write.hpp>
#include <boost/beast/ssl/ssl_stream.hpp>
#include <chrono>
#include <format>
#include <json/json.h>
#include <json/value.h>
#include <json/writer.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <openssl/tls1.h>
#include <spdlog/common.h>
#include <spdlog/spdlog.h>
#include <string>
#include <tuple>
using namespace baiyu;

LLM::LLM(boost::asio::io_context &ioc, const std::string &anthropicBaseUrl)
    : ioc_(ioc), resolver_(ioc),
      ssl_ctx_(boost::asio::ssl::context::tlsv13_client),
      stream_(ioc, ssl_ctx_), port_(443), isBusy_(false), onHandshake_(false),
      ms_(600000), systemPrompt_("You are a helpful assistant.") {
  this->anthropicBaseUrl_ = anthropicBaseUrl;
  this->setApiTimeOutMs();
  std::tie(this->host_, this->target_) = this->parseUrl(anthropicBaseUrl);
  this->ssl_ctx_.set_default_verify_paths();
  this->ssl_ctx_.set_verify_mode(boost::asio::ssl::verify_peer);
}
// LLM():anthropicBaseUrl_("https://api.deepseek.com/anthropic");
LLM::LLM(boost::asio::io_context &ioc, const std::string &anthropicBaseUrl,
         const std::string &apiKey)
    : LLM(ioc, anthropicBaseUrl) {
  this->apiKey_ = apiKey;
  std::tie(this->host_, this->target_) = this->parseUrl(anthropicBaseUrl);
}
LLM &LLM::setAnthropicBaseUrl(const std::string &anthropicBaseUrl) {
  this->anthropicBaseUrl_ = anthropicBaseUrl;
  std::tie(this->host_, this->target_) = this->parseUrl(anthropicBaseUrl);
  return *this;
}
LLM &LLM::setApiKey(const std::string &apiKey) {
  this->apiKey_ = apiKey;
  return *this;
}
LLM &LLM::setApiTimeOutMs(int ms) {
  this->ms_ = std::chrono::milliseconds(ms);
  return *this;
}
LLM &LLM::setAnthropicModel(const std::string &model) {
  this->model_ = model;
  return *this;
}
LLM &LLM::setPort(unsigned short port) {
  this->port_ = 443;
  return *this;
}
std::string LLM::getAnthropicBaseUrl() { return this->anthropicBaseUrl_; }
std::string LLM::getApiKey() { return this->apiKey_; }
std::chrono::milliseconds LLM::getApiTimeOutMs() { return this->ms_; }
std::string LLM::getAnthropicModel() { return this->model_; }
void LLM::chat(const std::string &msg, ChatCallBack callback) {
  if (isBusy_) {
    if (callback) {
      callback(boost::asio::error::in_progress, "request already in progress");
    }
    return;
  }
  if (apiKey_.empty()) {
    if (callback) {
      callback(boost::asio::error::operation_not_supported, "api key is empty");
    }
    return;
  }
  if (host_.empty() || target_.empty()) {
    if (callback) {
      callback(boost::asio::error::invalid_argument,
               "host or target is invalid");
    }
    return;
  }
  this->usrMsg_ = msg;
  this->callBack_ = std::move(callback);
  this->isBusy_ = true;
  this->onHandshake_ = false;
  this->req_ = {};
  this->res_ = {};
  this->flat_buffer_.clear();
  boost::beast::get_lowest_layer(stream_).expires_after(this->ms_);
  // SNI
  if (not SSL_set_tlsext_host_name(this->stream_.native_handle(),
                                   host_.c_str())) {
    boost::beast::error_code ec{static_cast<int>(::ERR_get_error()),
                                boost::asio::error::get_ssl_category()};
    this->fail(ec, "failed to set SNI host name");
    return;
  }
  auto self = shared_from_this();
  this->resolver_.async_resolve(
      this->host_, std::to_string(this->port_),
      [self](const boost::beast::error_code &ec,
             boost::asio::ip::tcp::resolver::results_type results) {
        self->on_resolve(ec, results);
      });
}
std::string LLM::getAllInformation() {
  return std::format("anthropicBaseUrl = {}\nhost = {}\ntarget = {}\napikey = "
                     "{}\nms = {}\nmodel = {}\n",
                     this->anthropicBaseUrl_, this->host_, this->target_,
                     this->apiKey_, this->ms_.count(), this->model_);
}
LLM::~LLM() {}
void LLM::on_resolve(const boost::beast::error_code &ec,
                     boost::asio::ip::tcp::resolver::results_type results) {
  if (ec) {
    fail(ec, "resolove failed");
    return;
  }
  boost::beast::get_lowest_layer(stream_).expires_after(this->ms_);
  auto self = shared_from_this();
  boost::beast::get_lowest_layer(stream_).async_connect(
      results,
      [self](const boost::beast::error_code &ec,
             boost::asio::ip::tcp::resolver::results_type::endpoint_type ep) {
        self->on_connect(ec, ep);
      });
}
void LLM::on_connect(
    const boost::beast::error_code &ec,
    boost::asio::ip::tcp::resolver::results_type::endpoint_type ep) {
  if (ec) {
    fail(ec, "connect faild");
    return;
  }
  boost::beast::get_lowest_layer(stream_).expires_after(this->ms_);
  auto self = shared_from_this();
  stream_.async_handshake(
      boost::asio::ssl::stream_base::client,
      [self](const boost::beast::error_code &ec) { self->on_handshake(ec); });
}
void LLM::on_handshake(const boost::beast::error_code &ec) {
  if (ec) {
    fail(ec, "ssl on_handshake faild");
    return;
  }
  onHandshake_ = true;
  write();
}
void LLM::write() {
  this->req_.version(11);
  this->req_.method(boost::beast::http::verb::post);
  this->req_.target(this->target_);
  this->req_.set(boost::beast::http::field::host, this->host_);
  this->req_.set(boost::beast::http::field::user_agent,
                 baiyu::BROWSER_USER_AGENT);
  this->req_.set(boost::beast::http::field::content_type, baiyu::CONTENT_TYPE);
  this->req_.set(boost::beast::http::field::authorization,
                 "Bearer " + this->apiKey_);
  this->req_.set(baiyu::ANTHROPIC_VERSION, baiyu::KEY_ANTHROPIC_VERSION);
  this->req_.body() = buildRequestBody(usrMsg_);
  this->req_.prepare_payload();
  boost::beast::get_lowest_layer(stream_).expires_after(this->ms_);
  auto self = shared_from_this();
  boost::beast::http::async_write(stream_, req_,
                                  [self](const boost::beast::error_code &ec,
                                         std::size_t bytes_transferred) {
                                    self->on_write(ec, bytes_transferred);
                                  });
}
void LLM::on_write(const boost::beast::error_code &ec,
                   std::size_t bytes_transferred) {
  if (ec) {
    fail(ec, "http write failed");
    return;
  }
  recv();
}
void LLM::recv() {
  boost::beast::get_lowest_layer(stream_);
  auto self = shared_from_this();
  boost::beast::http::async_read(stream_, flat_buffer_, res_,
                                 [self](const boost::beast::error_code &ec,
                                        std::size_t bytes_transferred) {
                                   self->on_read(ec, bytes_transferred);
                                 });
}
void LLM::on_read(const boost::beast::error_code &ec,
                  std::size_t bytes_transferred) {
  if (ec) {
    fail(ec, "http read failed");
    return;
  }
  isBusy_ = false;
  onHandshake_ = false;
  std::string body = res_.body();
  if (this->res_.result_int() < 200 || this->res_.result_int() >= 300) {
    if (callBack_) {
      std::string realError = "HTTP Status " +
                              std::to_string(this->res_.result_int()) + " : " +
                              body;
      callBack_(boost::asio::error::fault, realError);
    }
    return;
  }
  if (callBack_)
    callBack_({}, body);
}

std::tuple<std::string, std::string> LLM::parseUrl(const std::string &url) {
  std::size_t posFirst = url.find("://");
  if (posFirst == std::string::npos)
    return {};
  std::size_t posSecond = url.find("/", posFirst + 3);
  if (posSecond == std::string::npos) {
    // 无效路径
    return {url.substr(posFirst + 3), ""};
  }
  return {url.substr(posFirst + 3, posSecond - (posFirst + 3)),
          url.substr(posSecond, std::string::npos)};
}
void LLM::fail(const boost::beast::error_code &ec,
               const std::string &errorMsg) {
  this->isBusy_ = false;
  this->onHandshake_ = false;
  spdlog::error("{} : {}", ec.what(), errorMsg);
}
void LLM::setsystempromp(const std::string &msg) { this->systemPrompt_ = msg; }
std::string LLM::buildRequestBody(const std::string &msg) {
  Json::Value root;
  root["model"] = this->model_;
  root["max_tokens"] = MAX_TOEKN;
  root["system"] = this->systemPrompt_;
  Json::Value messages(Json::arrayValue);
  Json::Value messageTarget;
  messageTarget["role"] = "user";
  Json::Value contentInnerMsg;
  contentInnerMsg["type"] = "text";
  contentInnerMsg["text"] = this->usrMsg_;
  Json::Value contentArray(Json::arrayValue);
  contentArray.append(contentInnerMsg);
  messageTarget["content"] = contentArray;
  messages.append(messageTarget);
  root["messages"] = messages;
  Json::StreamWriterBuilder builder;
  builder["indentation"] = "";
  return Json::writeString(builder, root);
}
