# C++ 大模型调用库项目计划书

## 一、项目名称

**llmcpp：基于 Boost.Beast 的 C++ 大模型调用库**

------

## 二、项目背景

随着大语言模型接口逐渐标准化，很多模型服务已经提供了接近 OpenAI 风格的 API。DeepSeek 官方文档明确说明其 API 与 OpenAI 格式兼容，默认 `base_url` 为 `https://api.deepseek.com`，也可使用兼容路径 `https://api.deepseek.com/v1`。这意味着可以围绕统一协议设计一个可扩展的 C++ SDK，而不是把实现写死在某一家平台上。([DeepSeek API Docs](https://api-docs.deepseek.com/?utm_source=chatgpt.com))

另一方面，Boost.Beast 本身就是面向 HTTP/1 与 WebSocket 的底层网络库，并与 Boost.Asio 的异步模型保持一致；Boost.Asio 官方也提供了基于 `awaitable`、`use_awaitable` 和 `co_spawn` 的 C++20 coroutine 支持，适合拿来实现现代 C++ 风格的异步网络 SDK。([Boost](https://www.boost.org/library/latest/beast/?utm_source=chatgpt.com))

因此，本项目拟开发一个**面向 C++20 的通用大模型调用库**，第一阶段聚焦“稳定调用模型 API”，后续再扩展为 agent 能力。

------

## 三、项目目标

### 1. 总体目标

开发一个基于 C++20 的大模型调用库，具备以下能力：

- 默认支持 DeepSeek
- 支持 OpenAI-compatible API
- 支持用户自定义模型地址、模型名和鉴权配置
- 提供同步/异步聊天调用接口
- 支持流式输出
- 提供清晰的错误处理与可扩展接口

### 2. 阶段目标

第一阶段不直接做完整 agent 框架，而是先完成**LLM SDK 基础设施**。
原因很简单：agent 的本质建立在“稳定模型调用能力”之上；只有网络层、协议层、请求封装、错误处理和流式输出先打牢，后续的 tool calling、memory、agent loop 才有可靠基础。

------

## 四、项目定位

本项目定位为：

> **一个通用的 C++ 大模型调用库，而不是单一平台的聊天 demo。**

它的边界很明确：

- **要做的**：模型 API 调用、消息封装、Provider 抽象、流式解析、配置管理、错误处理
- **暂时不做的**：RAG、向量数据库、完整多 agent 协作框架、GUI、复杂业务系统

也就是说，第一阶段重点是把它做成一块**可复用、可扩展、可二次开发的底层库**。

------

## 五、需求分析

## 5.1 功能性需求

### 核心需求

1. 能够发起标准 chat completion 请求
2. 能够配置：
   - API Key
   - Base URL
   - Model
   - Timeout
   - Headers
3. 能够处理多轮 messages
4. 能够解析标准 JSON 响应
5. 能够返回统一的 `ChatResponse`

### 扩展需求

1. 支持流式响应
2. 支持模型列表查询；DeepSeek 官方也提供 `/models` 接口。([DeepSeek API Docs](https://api-docs.deepseek.com/api/list-models?utm_source=chatgpt.com))
3. 支持 OpenAI-compatible provider
4. 支持后续接入 tool calls；DeepSeek 官方已经提供 Tool Calls 能力。([DeepSeek API Docs](https://api-docs.deepseek.com/guides/tool_calls?utm_source=chatgpt.com))

------

## 5.2 非功能性需求

1. **可扩展性**
   不把 DeepSeek 写死，必须通过 Provider 抽象支持更多平台。
2. **可维护性**
   请求构造、响应解析、网络传输、配置管理分层清晰。
3. **可靠性**
   要有超时、错误码、异常处理、状态码校验。
4. **现代 C++ 风格**
   使用 C++20，尽量采用 coroutine 风格封装异步逻辑。Boost.Asio 官方明确支持这套模型。([Boost](https://www.boost.org/doc/libs/latest/doc/html/boost_asio/overview/composition/cpp20_coroutines.html?utm_source=chatgpt.com))
5. **跨平台性**
   目标平台至少包括 Linux 和 Windows。Boost 体系天然适合跨平台网络开发。([Boost](https://www.boost.org/library/latest/beast/?utm_source=chatgpt.com))

------

## 六、技术路线

## 6.1 总体架构

项目采用三层架构：

### 第一层：Transport 层

负责底层 HTTP/HTTPS 通信，包括：

- DNS 解析
- TCP 建连
- TLS 握手
- HTTP 请求发送
- HTTP 响应读取
- 超时控制

这一层基于 Boost.Asio + Boost.Beast + OpenSSL 实现。Beast 官方定位就是用于构建 HTTP/1、WebSocket 等网络协议库。([Boost](https://www.boost.org/library/latest/beast/?utm_source=chatgpt.com))

### 第二层：Provider 层

负责不同模型服务的协议适配，包括：

- DeepSeekProvider
- OpenAICompatibleProvider
- 后续可扩展 CustomProvider

由于 DeepSeek 官方明确采用 OpenAI-compatible API，因此这一层应优先围绕“兼容 OpenAI 风格协议”来设计。([DeepSeek API Docs](https://api-docs.deepseek.com/?utm_source=chatgpt.com))

### 第三层：Client 层

对外提供统一调用接口：

- `chat()`
- `stream_chat()`
- 后续 `list_models()`

------

## 6.2 技术栈选型

### 必选技术栈

- **语言**：C++20
- **网络**：Boost.Asio
- **HTTP/HTTPS**：Boost.Beast
- **TLS**：OpenSSL
- **构建**：CMake
- **JSON**：nlohmann/json
- **格式化**：fmt

### 推荐技术栈

- **日志**：spdlog
- **测试**：Catch2 或 GoogleTest
- **文档**：Doxygen + Markdown

### 选型理由

Boost.Asio 已提供成熟的 coroutine 异步模型支持；Boost.Beast 提供 HTTP/1 和 WebSocket 能力，并且采用与 Asio 一致的异步模型，非常适合做网络 SDK 底层。([Boost](https://www.boost.org/doc/libs/latest/doc/html/boost_asio/overview/composition/cpp20_coroutines.html?utm_source=chatgpt.com))

------

## 七、项目范围

## 7.1 第一阶段范围（必须完成）

### 功能模块

1. HTTP 客户端封装
2. HTTPS 支持
3. Chat Completion 请求
4. JSON 请求构造与响应解析
5. DeepSeek 默认支持
6. OpenAI-compatible 抽象支持
7. 统一错误处理
8. 超时机制
9. 基础示例程序
10. 基础单元测试

## 7.2 第二阶段范围（可选增强）

1. 流式响应
2. Provider 插件化
3. 重试机制
4. 日志中间件
5. 模型列表查询
6. 使用量统计

## 7.3 第三阶段范围（后续演进）

1. Tool Calling
2. Agent Loop
3. Memory 抽象
4. Prompt Template
5. 多 Provider fallback

DeepSeek 官方已经提供 Tool Calls 能力，因此把它作为后续 agent 化演进方向是合理的。([DeepSeek API Docs](https://api-docs.deepseek.com/guides/tool_calls?utm_source=chatgpt.com))

------

## 八、模块设计

## 8.1 目录结构

```text
llmcpp/
├── include/llmcpp/
│   ├── core/
│   │   ├── message.hpp
│   │   ├── chat_request.hpp
│   │   ├── chat_response.hpp
│   │   ├── error.hpp
│   │   └── result.hpp
│   ├── net/
│   │   ├── http_client.hpp
│   │   ├── tls_context.hpp
│   │   └── timeout.hpp
│   ├── provider/
│   │   ├── provider.hpp
│   │   ├── deepseek_provider.hpp
│   │   └── openai_compatible_provider.hpp
│   ├── client/
│   │   └── client.hpp
│   └── util/
│       ├── json.hpp
│       └── log.hpp
├── src/
├── tests/
├── examples/
└── CMakeLists.txt
```

------

## 8.2 核心类设计

### 核心数据类型

- `Message`
- `ChatRequest`
- `ChatResponse`
- `UsageInfo`
- `LlmError`

### 网络层

- `HttpClient`
- `HttpRequest`
- `HttpResponse`

### Provider 层

- `IProvider`
- `DeepSeekProvider`
- `OpenAICompatibleProvider`

### 用户入口

- `Client`

------

## 8.3 抽象原则

### Provider 抽象

统一接口大致应为：

```cpp
class IProvider {
public:
    virtual ChatResponse chat(const ChatRequest&) = 0;
    virtual ~IProvider() = default;
};
```

异步版本后续可扩为 `co_await` 风格接口。

### Message 抽象

消息类型至少包括：

- system
- user
- assistant

后续如扩展 tool calling，再补：

- tool

------

## 九、阶段计划与里程碑

## 阶段一：项目骨架搭建

### 时间建议：第 1 周

#### 目标

完成工程初始化，搭建最小可编译项目。

#### 工作内容

- 建立 CMake 工程
- 配置依赖
- 组织头文件与源文件目录
- 编写最小测试程序
- 约定命名规范、命名空间、错误码风格

#### 交付物

- 可编译工程骨架
- 初版 README
- 依赖说明文档

------

## 阶段二：底层 HTTP/HTTPS 能力

### 时间建议：第 2～3 周

#### 目标

完成基于 Beast 的 HTTPS POST 请求封装。

#### 工作内容

- 域名解析
- TCP 连接
- TLS 握手
- 设置 HTTP headers
- 发送 JSON body
- 读取响应
- 超时处理
- 状态码检查

#### 交付物

- `HttpClient`
- HTTPS 请求样例
- 基础异常处理

Beast 官方文档说明其适合构建 HTTP/1 与 WebSocket 协议库，并与 Asio 模型一致。([Boost](https://www.boost.org/library/latest/beast/?utm_source=chatgpt.com))

------

## 阶段三：DeepSeek Provider 实现

### 时间建议：第 4 周

#### 目标

接通 DeepSeek chat completion。

#### 工作内容

- 构造 DeepSeek 风格请求体
- Bearer Token 鉴权
- 解析 chat completion 响应
- 封装为统一 `ChatResponse`
- 编写 DeepSeek 示例程序

DeepSeek 官方文档给出了兼容 OpenAI 格式的 chat completion 接口说明。([DeepSeek API Docs](https://api-docs.deepseek.com/?utm_source=chatgpt.com))

#### 交付物

- `DeepSeekProvider`
- `examples/deepseek_chat.cpp`

------

## 阶段四：通用 Provider 抽象

### 时间建议：第 5 周

#### 目标

抽象出通用 OpenAI-compatible Provider。

#### 工作内容

- 提炼共性请求字段
- 支持自定义 base_url、model、headers
- 将 DeepSeek 重构为默认配置实例

#### 交付物

- `IProvider`
- `OpenAICompatibleProvider`

------

## 阶段五：异步与流式能力

### 时间建议：第 6～7 周

#### 目标

提供异步接口，开始支持 streaming。

#### 工作内容

- coroutine 化请求流程
- `co_spawn` 驱动异步任务
- 流式 chunk 解析
- 用户回调 / observer 接口设计

Boost.Asio 官方明确支持 `awaitable`、`use_awaitable` 与 `co_spawn` 来组织异步逻辑。([Boost](https://www.boost.org/doc/libs/latest/doc/html/boost_asio/overview/composition/cpp20_coroutines.html?utm_source=chatgpt.com))

#### 交付物

- 异步版本 `chat_async`
- 基础流式接口 `stream_chat`

------

## 阶段六：测试与文档

### 时间建议：第 8 周

#### 目标

使项目达到可开源、可发布状态。

#### 工作内容

- 单元测试
- 错误场景测试
- 示例程序整理
- README 完善
- API 文档整理
- 发布首个 release

#### 交付物

- `tests/`
- `examples/`
- 完整 README
- v0.1.0 Release

------

## 十、风险分析

## 10.1 技术风险

### 1. Beast/Asio 异步实现复杂

异步网络库本身就容易在生命周期、缓冲区、异常传播上出问题。
解决策略：先完成同步版或半异步版，再逐步 coroutine 化。

### 2. 流式解析复杂

SSE 或 chunked response 的边界处理容易出错。
解决策略：第一版先保证普通 completion，流式放在第二阶段。

### 3. 各平台 API 存在细节差异

虽然 DeepSeek 兼容 OpenAI 风格，但不同平台在字段、错误格式、流式行为上可能会有差别。
解决策略：在 Provider 层吸收差异，不污染核心层。DeepSeek 官方文档也提示其兼容 OpenAI 风格，但兼容路径中的 `v1` 与模型版本无关，这种细节就需要在 Provider 中处理。([DeepSeek API Docs](https://api-docs.deepseek.com/?utm_source=chatgpt.com))

------

## 10.2 工程风险

### 1. 一开始做太大

如果一上来就做 agent、RAG、tool system、memory，项目会迅速失控。
解决策略：严格控制第一阶段范围，只做 SDK 基础能力。

### 2. 抽象过早

如果一开始抽象太多，会导致结构复杂、实现困难。
解决策略：先跑通 DeepSeek，再抽象 OpenAI-compatible provider。

------

## 十一、质量要求

项目第一版应达到以下质量标准：

- 能稳定完成 chat completion 请求
- 错误信息清晰可定位
- 至少提供一个完整示例程序
- 支持 Linux/Windows 编译
- README 能指导用户 10 分钟内跑通

------

## 十二、交付物清单

### 代码交付物

- 源代码
- CMake 构建文件
- 示例程序
- 测试代码

### 文档交付物

- README
- 架构说明
- 使用说明
- 错误码说明
- 开发计划与版本路线图

### 版本交付物

- `v0.1.0`：完成基础 chat 调用
- `v0.2.0`：增加异步/流式
- `v0.3.0`：增加 tool calling 预留接口

------

## 十三、版本路线图

### v0.1.0

- DeepSeek chat completion
- OpenAI-compatible 基础抽象
- 同步调用
- 基础错误处理
- 示例程序

### v0.2.0

- coroutine 异步接口
- 流式响应
- 日志与重试机制

### v0.3.0

- tool calls 支持
- agent 扩展预留
- 更丰富的 provider 配置

DeepSeek 官方文档显示其模型已支持 Tool Calls，因此将这部分放在 v0.3.0 是合理路线。([DeepSeek API Docs](https://api-docs.deepseek.com/guides/tool_calls?utm_source=chatgpt.com))

------

## 十四、最终结论

本项目建议采用**分阶段推进策略**：

先做一个**通用 C++ 大模型调用库**，默认支持 DeepSeek，兼容 OpenAI 风格 API；
等第一阶段的网络层、协议层、响应解析和异步能力稳定后，再扩展到 tool calling 与 agent 能力。

这个路线的优点是：

- 目标明确
- 工程风险可控
- 技术含金量高
- 后续扩展空间大

换句话说：

> **第一阶段做 llm sdk，第二阶段再做 agent。**