#include "llm.h"
#include <boost/asio.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <chrono>
#include <format>
#include <string>
using namespace baiyu;
namespace beast = boost::beast;
namespace net = boost::asio;
namespace ssl = net::ssl;
using tcp = net::ip::tcp;

// LLM():anthropicBaseUrl_("https://api.deepseek.com/anthropic");
LLM::LLM(const std::string &anthropicBaseUrl, const std::string &apiKey) {
  this->anthropicBaseUrl_ = anthropicBaseUrl;
  this->apiKey_ = apiKey;
  this->setApiTimeOutMs();
}
LLM::LLM(const std::string &anthropicBaseUrl) {
  this->anthropicBaseUrl_ = anthropicBaseUrl;
  this->setApiTimeOutMs();
}
LLM &LLM::setAnthropicBaseUrl(const std::string &anthropicBaseUrl) {
  this->anthropicBaseUrl_ = anthropicBaseUrl;
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
std::string LLM::sendMessage(const std::string &msg) { return ""; }
std::string LLM::getAnthropicBaseUrl() { return this->anthropicBaseUrl_; }
std::string LLM::getApiKey() { return this->apiKey_; }
std::chrono::milliseconds LLM::getApiTimeOutMs() { return this->ms_; }
std::string LLM::getAnthropicModel() { return this->model_; }
std::string LLM::chat(const std::string &msg) { return msg; }
std::string LLM::getAllInformation() {
  return std::format(
      "anthropicBaseUrl = {},\napikey = {},\nms = {},\nmodel = {}.\n",
      this->anthropicBaseUrl_, this->apiKey_, this->ms_.count(), this->model_);
}
LLM::~LLM() {}