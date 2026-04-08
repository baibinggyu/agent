```cpp


```





```cpp
#include "llm.h"

#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/asio/ssl/verify_mode.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/core/tcp_stream.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl/ssl_stream.hpp>
#include <boost/beast/version.hpp>
#include <chrono>
#include <format>
#include <json/json.h>
#include <json/value.h>
#include <json/writer.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <spdlog/spdlog.h>
#include <string>
#include <tuple>

using namespace baiyu;

LLM::LLM(boost::asio::io_context &ioc, const std::string &anthropicBaseUrl)
    : ioc_(ioc),
      resolver_(ioc),
      ssl_ctx_(boost::asio::ssl::context::tlsv13_client),
      stream_(ioc, ssl_ctx_),
      port_(443),
      isBusy_(false),
      onHandshake_(false),
      ms_(600000),
      model_(kDefaultModel),
      systemPrompt_("You are a helpful assistant.") {
  anthropicBaseUrl_ = anthropicBaseUrl;
  setApiTimeOutMs();
  std::tie(host_, target_) = parseUrl(anthropicBaseUrl_);

  ssl_ctx_.set_default_verify_paths();
  ssl_ctx_.set_verify_mode(boost::asio::ssl::verify_peer);
}

LLM::LLM(boost::asio::io_context &ioc, const std::string &anthropicBaseUrl,
         const std::string &apiKey)
    : LLM(ioc, anthropicBaseUrl) {
  apiKey_ = apiKey;
  std::tie(host_, target_) = parseUrl(anthropicBaseUrl_);
}

LLM &LLM::setAnthropicBaseUrl(const std::string &anthropicBaseUrl) {
  anthropicBaseUrl_ = anthropicBaseUrl;
  std::tie(host_, target_) = parseUrl(anthropicBaseUrl_);
  return *this;
}

LLM &LLM::setApiKey(const std::string &apiKey) {
  apiKey_ = apiKey;
  return *this;
}

LLM &LLM::setApiTimeOutMs(int ms) {
  ms_ = std::chrono::milliseconds(ms);
  return *this;
}

LLM &LLM::setAnthropicModel(const std::string &model) {
  model_ = model;
  return *this;
}

LLM &LLM::setPort(unsigned short port) {
  port_ = port;
  return *this;
}

std::string LLM::getAnthropicBaseUrl() { return anthropicBaseUrl_; }
std::string LLM::getApiKey() { return apiKey_; }
std::chrono::milliseconds LLM::getApiTimeOutMs() { return ms_; }
std::string LLM::getAnthropicModel() { return model_; }

void LLM::chat(const std::string &msg, ChatCallBack callback) {
  if (isBusy_) {
    if (callback) {
      callback(x
               "request already in progress");
    }
    return;
  }

  if (apiKey_.empty()) {
    if (callback) {
      callback(boost::asio::error::operation_not_supported,
               "api key is empty");
    }
    return;
  }

  if (host_.empty() || target_.empty()) {
    if (callback) {
      callback(boost::asio::error::invalid_argument,
               "invalid anthropic base url");
    }
    return;
  }
x	
  usrMsg_ = msg;
  callBack_ = std::move(callback);
  isBusy_ = true;
  onHandshake_ = false;

  req_ = {};
  res_ = {};
  flat_buffer_.clear();

  boost::beast::get_lowest_layer(stream_).expires_after(ms_);

  // SNI
  if (!SSL_set_tlsext_host_name(stream_.native_handle(), host_.c_str())) {
    boost::beast::error_code ec{
        static_cast<int>(::ERR_get_error()),
        boost::asio::error::get_ssl_category()};
    fail(ec, "failed to set SNI host name");
    return;
  }

  auto self = shared_from_this();
  resolver_.async_resolve(
      host_, std::to_string(port_),
      [self](const boost::beast::error_code &ec,
             boost::asio::ip::tcp::resolver::results_type results) {
        self->on_resolve(ec, results);
      });
}

std::string LLM::getAllInformation() {
  return std::format(
      "anthropicBaseUrl = {},\napikey = {},\nms = {},\nmodel = {}.\n",
      anthropicBaseUrl_, apiKey_, ms_.count(), model_);
}

LLM::~LLM() {}

void LLM::on_resolve(const boost::beast::error_code &ec,
                     boost::asio::ip::tcp::resolver::results_type results) {
  if (ec) {
    fail(ec, "resolve failed");
    return;
  }

  boost::beast::get_lowest_layer(stream_).expires_after(ms_);

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
    boost::asio::ip::tcp::resolver::results_type::endpoint_type /*ep*/) {
  if (ec) {	
    fail(ec, "connect failed");
    return;
  }

  boost::beast::get_lowest_layer(stream_).expires_after(ms_);

  auto self = shared_from_this();
  stream_.async_handshake(
      boost::asio::ssl::stream_base::client,
      [self](const boost::beast::error_code &ec) { self->on_handshake(ec); });
}

void LLM::on_handshake(const boost::beast::error_code &ec) {
  if (ec) {
    fail(ec, "ssl handshake failed");
    return;
  }

  onHandshake_ = true;
  write();
}

void LLM::write() {
  req_.version(11);
  req_.method(boost::beast::http::verb::post);
  req_.target(target_);
  req_.set(boost::beast::http::field::host, host_);
  req_.set(boost::beast::http::field::user_agent, BOOST_BEAST_VERSION_STRING);
  req_.set(boost::beast::http::field::content_type, "application/json");

  // DeepSeek 的 Anthropic 兼容接口常用这两个头
  req_.set("x-api-key", apiKey_);
  req_.set("anthropic-version", kAnthropicVersion);

  req_.body() = buildRequestBody(usrMsg_);
  req_.prepare_payload();

  boost::beast::get_lowest_layer(stream_).expires_after(ms_);

  auto self = shared_from_this();
  boost::beast::http::async_write(
      stream_, req_,
      [self](const boost::beast::error_code &ec, std::size_t bytes_transferred) {
        self->on_write(ec, bytes_transferred);
      });
}

void LLM::on_write(const boost::beast::error_code &ec,
                   std::size_t /*bytes_transferred*/) {
  if (ec) {
    fail(ec, "http write failed");
    return;
  }
  recv();
}
	
void LLM::recv() {
  boost::beast::get_lowest_layer(stream_).expires_after(ms_);

  auto self = shared_from_this();
  boost::beast::http::async_read(
      stream_, flat_buffer_, res_,
      [self](const boost::beast::error_code &ec, std::size_t bytes_transferred) {
        self->on_read(ec, bytes_transferred);
      });
}

void LLM::on_read(const boost::beast::error_code &ec,
                  std::size_t /*bytes_transferred*/) {
  if (ec) {
    fail(ec, "http read failed");
    return;
  }

  isBusy_ = false;
  onHandshake_ = false;

  std::string body = res_.body();

  boost::beast::error_code shutdownEc;
  stream_.shutdown(shutdownEc);
  if (shutdownEc == boost::asio::error::eof) {
    shutdownEc = {};
  }

  if (res_.result_int() < 200 || res_.result_int() >= 300) {
    if (callBack_) {
      callBack_(boost::asio::error::fault, body);
    }
    return;
  }

  if (callBack_) {
    callBack_({}, body);
  }
}		

std::tuple<std::string, std::string> LLM::parseUrl(const std::string &url) {
  std::size_t posFirst = url.find("://");
  if (posFirst == std::string::npos) {
    return {};
  }

  std::size_t posSecond = url.find('/', posFirst + 3);
  if (posSecond == std::string::npos) {
    return {url.substr(posFirst + 3), "/"};
  }

  return {url.substr(posFirst + 3, posSecond - (posFirst + 3)),
          url.substr(posSecond)};
}

void LLM::fail(const boost::beast::error_code &ec,
               const std::string &errorMsg) {
  isBusy_ = false;
  onHandshake_ = false;

  spdlog::error("{} : {}", errorMsg, ec.message());

  if (callBack_) {
    callBack_(ec, errorMsg + " : " + ec.message());
  }
}

void LLM::setsystempromp(const std::string &msg) { systemPrompt_ = msg; }

std::string LLM::buildRequestBody(const std::string &msg) {
  Json::Value root;
  root["model"] = model_.empty() ? kDefaultModel : model_;
  root["max_tokens"] = MAX_TOEKN;
  root["system"] = systemPrompt_;

  Json::Value messages(Json::arrayValue);
  Json::Value messageTarget;
  messageTarget["role"] = "user";

  Json::Value contentArray(Json::arrayValue);
  Json::Value contentInnerMsg;
  contentInnerMsg["type"] = "text";
  contentInnerMsg["text"] = msg;

  contentArray.append(contentInnerMsg);
  messageTarget["content"] = contentArray;
  messages.append(messageTarget);

  root["messages"] = messages;

  Json::StreamWriterBuilder builder;
  builder["indentation"] = "";
  return Json::writeString(builder, root);
}
```

```cpp
chat()
  -> async_resolve()
      -> on_resolve()
          -> async_connect()
              -> on_connect()
                  -> async_handshake()
                      -> on_handshake()
                          -> write()
                              -> async_write()
                                  -> on_write()
                                      -> recv()
                                          -> async_read()
                                              -> on_read()
```

* 解析域名
* TCP 连接
* TLS 握手
* 发 HTTP POST
* 读 HTTP 响应
* 回调给你结果