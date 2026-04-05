#ifndef LLM_H
#define LLM_H
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <chrono>
#include <string>
namespace baiyu {
class LLM {
public:
  explicit LLM(const std::string &anthropicBaseUrl =
          "https://api.deepseek.com/anthropic");
  LLM(const std::string &anthropicBaseUrl, const std::string &apiKey);
  LLM& setAnthropicBaseUrl(const std::string &anthropicBaseUrl);
  LLM& setApiKey(const std::string &apiKey);
  LLM& setApiTimeOutMs(int ms = 600000);
  LLM& setAnthropicModel(const std::string &model);
  std::string sendMessage(const std::string &msg);
  std::string getAnthropicBaseUrl();
  std::string getApiKey();
  std::chrono::milliseconds getApiTimeOutMs();
  std::string getAnthropicModel();
  // send msg , return std::string
  std::string chat(const std::string& msg);
  std::string getAllInformation();
  ~LLM();

private:
  std::string anthropicBaseUrl_;
  std::string apiKey_;
  std::chrono::milliseconds ms_;
  std::string model_;
};
} // namespace baiyu

#endif