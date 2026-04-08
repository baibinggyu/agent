#ifndef LLM_H
#define LLM_H
#include <boost/asio.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/core/error.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/http/string_body_fwd.hpp>
#include <boost/beast/ssl.hpp>
#include <chrono>
#include <functional>
#include <spdlog/spdlog.h>
#include <string>
#include <tuple>
#include <utility>
namespace baiyu {
// user-agent simulation browser
constexpr const char *BROWSER_USER_AGENT =
    "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like "
    "Gecko) Chrome/145.0.0.0 Safari/537.36";
constexpr const char *CONTENT_TYPE = "application/json";
constexpr const char *KEY_DEFAULT_MODEL = "deepseek-chat";
constexpr const char *ANTHROPIC_VERSION = "anthropic-version";
constexpr const char *KEY_ANTHROPIC_VERSION = "2023-06-01";
// main effect record error
inline void LLMDefaultCallBack(boost::beast::error_code ec,
                               const std::string &msg) {
  if (ec) {
    spdlog::error("LLM request failed : {}", ec.message());
    spdlog::error("detail : {}", msg);
    return;
  }
  spdlog::info("模型数据返回如下:");
  spdlog::info(msg);
}
class LLM : public std::enable_shared_from_this<LLM> {
public:
  using ChatCallBack =
      std::function<void(boost::beast::error_code, const std::string &)>;
  explicit LLM(boost::asio::io_context &ioc,
               const std::string &anthropicBaseUrl =
                   "https://api.deepseek.com/chat/completions");
  LLM(boost::asio::io_context &ioc, const std::string &anthropicBaseUrl,
      const std::string &apiKey);
  LLM &setAnthropicBaseUrl(const std::string &anthropicBaseUrl);
  LLM &setApiKey(const std::string &apiKey);
  LLM &setApiTimeOutMs(int ms = 600000);
  LLM &setAnthropicModel(const std::string &model);
  // you needn't set ,deflaut is 80
  LLM &setPort(unsigned short port = 443);
  std::string getAnthropicBaseUrl();
  std::string getApiKey();
  std::chrono::milliseconds getApiTimeOutMs();
  std::string getAnthropicModel();
  // send msg , return std::string
  void chat(const std::string &msg, ChatCallBack callback = LLMDefaultCallBack);
  void setsystempromp(const std::string &msg);
  std::string getAllInformation();
  // process quest body
  std::string buildRequestBody(const std::string &msg);

  ~LLM();

private:
  void on_resolve(const boost::beast::error_code &ec,
                  boost::asio::ip::tcp::resolver::results_type results);
  void
  on_connect(const boost::beast::error_code &ec,
             boost::asio::ip::tcp::resolver::results_type::endpoint_type ep);
  void on_handshake(const boost::beast::error_code &ec);
  void write();
  void on_write(const boost::beast::error_code &ec,
                std::size_t bytes_transferred);
  void recv();
  void on_read(const boost::beast::error_code &ec,
               std::size_t bytes_transferred);
  // return host and target
  // https://api.deepseek.com/anthropic
  // url with protocol
  std::tuple<std::string, std::string> parseUrl(const std::string &url);
  void fail(const boost::beast::error_code &ec, const std::string &errorMsg);

private:
  std::string host_;
  std::string target_; // net state path
  std::string usrMsg_;
  boost::asio::io_context &ioc_;
  std::string anthropicBaseUrl_;
  std::string apiKey_;
  unsigned short int port_;
  std::chrono::milliseconds ms_;
  std::string model_;
  // net widget
  boost::asio::ip::tcp::resolver resolver_;
  boost::asio::ssl::context ssl_ctx_;
  boost::beast::ssl_stream<boost::beast::tcp_stream> stream_;
  // http protocol widget
  boost::beast::flat_buffer flat_buffer_;
  boost::beast::http::request<boost::beast::http::string_body> req_;
  boost::beast::http::response<boost::beast::http::string_body> res_;
  // user status
  ChatCallBack callBack_;
  bool isBusy_;
  // is handshake ok ?
  // false TLS not complete
  // true TLS success
  bool onHandshake_;
  // LLM element
  enum { MAX_TOEKN = 1024 };
  std::string systemPrompt_;
};
} // namespace baiyu

#endif
