# LPC HTTP 服务的驱动层协议辅助设计

## 摘要

本设计提议在驱动层补一组小而实用的 HTTP 相关辅助能力，把 LPC HTTP 服务里的协议胶水代码下沉出去，但不把驱动做成一个完整的 Web 框架。

当前 `lpc_example_http/httpd.c` 把传输处理、HTTP 解析、表单解码、路由、控制器调度、IP 过滤和响应拼装都混在一个文件里。这里面真正重的并不是控制器调度，而是“半包、组包、请求拆解、URL 解码、响应格式化”这一层协议杂活。

这次改动的目标，是把低层、稳定、重复的协议处理收进驱动，同时继续把路由和业务逻辑留在 LPC。

## 问题

当前 LPC 实现有几个明显的维护和正确性风险：

- 一次 socket 读取就被当作一个完整 HTTP 请求。
- 请求行、请求头、请求体用一条 `sscanf` 直接拆，隐含假设是数据已经一次读全。
- 没有根据 `Content-Length` 做跨多次读取的 body 组装。
- 查询串和表单串现在只保留 value，不保留 key。
- URL 解码是临时写在 LPC 里的，重复且不完整。
- 响应报文是手写 `sprintf` 拼接的。
- header 规范化留给了应用层自己处理。

这些都属于协议胶水层。它们不是不能在 LPC 写，但写起来繁琐、容易漏边角，而且会持续挤占业务代码的可读性和维护精力。

## 目标

- 保持 HTTP 路由和控制器调度在 LPC 层。
- 把重复的 HTTP 协议辅助能力下沉到驱动。
- 提供一组足够小、但通用且容易测试的接口。
- 支持基于原始 socket chunk 的增量解析。
- 能把 URL 查询串和 URL 编码表单解码成 mapping。
- 提供标准响应拼装函数，去掉手写响应报文。

## 非目标

- 不把驱动做成完整 HTTP 服务器框架。
- 不把路由和控制器查找逻辑移进驱动。
- 不在解析器内部直接回调 LPC 业务逻辑。
- 这一轮不实现 multipart 表单解析。
- 不在驱动里自动生成 JSON 响应。
- 不把公开 API 设计成强依赖 socket `fd` 的形式。

## LPC 与驱动的职责边界

保留在 LPC 的职责：

- 根据请求路径做路由
- 校验允许的 HTTP method
- 做 IP 白名单判断
- 调用控制器函数
- 把业务结果编码成 JSON
- 决定应用层的状态码与响应头

下沉到驱动的职责：

- URL 编码 / 解码
- 查询串与表单串解码
- HTTP 请求边界判断
- header 规范化
- 基于 `Content-Length` 的 body 组包
- HTTP 响应字符串拼装

## 考虑过的方案

### 方案一：按 socket `fd` 绑定的解析器

驱动按 socket 描述符保存 HTTP 解析状态，LPC 用 `fd` 把 chunk 喂进去。

优点：

- LPC 侧状态管理最少
- LPC 调用面很小

缺点：

- HTTP 解析逻辑和 socket 内部实现绑得太紧
- 不利于独立测试
- 以后想做离线解析或其他传输层复用时不够灵活

### 方案二：解析器句柄 + 通用辅助 efun

LPC 为每个连接创建一个 parser handle，把原始 chunk 送进解析器。驱动内部负责解析状态、请求组包、header 规范化以及 query/form 解码。LPC 拿到完整 request mapping 后，再做业务流程。

优点：

- 传输层和协议辅助层边界清晰
- 易于做增量测试
- 路由和业务逻辑仍然留在 LPC
- 比 `fd` 绑定方案更通用、可复用

缺点：

- LPC 需要自己管理每个连接对应的 parser 生命周期
- 公开 API 会比完全 `fd` 绑定方案稍大一点

推荐：

这是本次推荐方案。它把最脆弱的协议处理从 LPC 挪到驱动，但又不会把驱动做成一个沉重的 HTTP 框架，也不会过度耦合 socket 子系统。

### 方案三：驱动直接提供高层 HTTP 服务

例如驱动直接接受请求并帮 LPC 做控制器调度。

优点：

- LPC 代码量最少

缺点：

- 会把应用层的设计决策写死在驱动里
- 不利于保留现有 LPC 控制器模式
- 对当前目标来说过重、过早

因此这个方案不采用。

## 建议的 API

### 字符串与表单辅助函数

```c
string url_decode(string s);
string url_encode(string s);
mapping http_decode_query(string s);
mapping http_decode_form(string body);
```

语义：

- `url_decode` 负责解码 `%xx` 形式的转义，并在 query/form 场景下正确处理 `+`
- `url_encode` 负责把保留字符编码成可安全放进 URL 的形式
- `http_decode_query` 把 `a=1&b=2` 这样的查询串转成 mapping
- `http_decode_form` 把 `application/x-www-form-urlencoded` 请求体转成 mapping

初始版本的 mapping 行为建议如下：

- key 和 value 都是解码后的字符串
- 允许空值
- 重复 key 先采用“后值覆盖前值”的简单语义，除非实现阶段确认现有代码更需要数组聚合

### 有状态 HTTP 解析器接口

```c
mixed http_parser_create();
mapping http_parser_feed(mixed parser, string chunk);
void http_parser_close(mixed parser);
```

语义：

- `http_parser_create` 返回一个不透明的 parser handle
- `http_parser_feed` 接收下一段原始字节串并返回结构化结果
- `http_parser_close` 释放解析器状态

`http_parser_feed` 的返回值形状建议为：

```c
([
  "requests": ({ /* 0 个或多个完整请求 mapping */ }),
  "error": 0,
  "need_more": 1,
])
```

如果解析失败，则返回：

```c
([
  "requests": ({}),
  "error": ([
    "status": 400,
    "reason": "malformed_header",
    "message": "Invalid HTTP header line",
  ]),
  "need_more": 0,
])
```

语义说明：

- `requests` 表示当前缓冲区里已经凑齐的完整请求，可以是 0 个、1 个或多个
- `need_more` 表示当前没有错误，但还需要更多字节才能组成完整请求
- `error` 为 `0` 或一个结构化错误 mapping，便于 LPC 直接据此返回 HTTP 错误响应

之所以用 `requests` 数组，是因为一次读取可能读不到完整请求，也可能刚好读到一个完整请求，甚至可能一次读到多个完整请求。

### HTTP 响应拼装函数

```c
string http_build_response(int status, mapping headers, string body);
```

语义：

- 生成标准 HTTP 响应字符串
- 内置常见状态码对应的 reason phrase
- 自动补 `Content-Length`
- 如果未显式指定，则默认补 `Connection: close`
- body 内容由 LPC 自己准备

这个函数不负责 JSON 编码，也不自动推断 `content-type`。

## 请求对象的结构

`http_parser_feed` 返回的每个完整请求，建议使用一个稳定、朴素的 mapping 结构：

```c
([
  "method": "POST",
  "path": "/update_code/update_file",
  "version": "1.1",
  "query_string": "file_name=%2Fadm%2Fdaemon%2Ffoo.c",
  "query": ([ "file_name": "/adm/daemon/foo.c" ]),
  "headers": ([
    "content-type": "application/x-www-form-urlencoded",
    "content-length": "31",
  ]),
  "body": "file_name=%2Fadm%2Fdaemon%2Ffoo.c",
  "form": ([ "file_name": "/adm/daemon/foo.c" ]),
])
```

补充说明：

- `headers` 的 key 统一小写
- 原始数据和解码后的结果同时保留
- `form` 仅在 `application/x-www-form-urlencoded` 时填充
- 诸如远端地址这类连接元数据，当前先不放进 parser 结果里，仍然由 LPC 通过 socket API 获取

## LPC 集成后的形态

`lpc_example_http/httpd.c` 会明显变薄，主流程大致变成：

1. 接受 socket 连接
2. 为该连接创建 parser handle
3. 每次读到 chunk 时调用 `http_parser_feed`
4. 如果 `need_more` 为真，就继续等待更多数据
5. 如果有 `error`，就用 `http_build_response` 生成错误响应并关闭连接
6. 对每个完整请求执行：
   - 从 path 中拆出 controller 和 function
   - 校验 method
   - 检查 IP 白名单
   - 调用控制器
   - 对结果执行 `json_encode`
   - 用 `http_build_response` 生成响应
7. 连接结束时关闭 parser

这样以后 LPC 不再需要自己处理：

- 请求行和 header/body 边界拆分
- 半包 body 组装
- header 规范化
- 控制器里零散的 URL 解码函数
- 手写 HTTP 响应字符串

## 兼容策略

第一阶段建议尽量保持现有控制器调用方式不变。

短期策略：

- 控制器仍然可以继续接收位置参数字符串
- HTTP 守护进程负责把 `request["query"]` / `request["form"]` 转换成现有控制器所需的参数形式

后续如果需要，再考虑：

- 让控制器直接接收完整 request mapping
- 但这应当是单独的一步，不和本次协议层下沉耦合在一起

这样能把“协议层整理”与“业务接口升级”拆开，降低一次改动的风险。

## 错误处理

解析器对于畸形请求，应返回结构化、可直接映射到 HTTP 的错误，而不是抛出笼统异常让 LPC 自己猜。

至少应覆盖这些类别：

- 错误的请求行
- 错误的 header 行
- 缺失或非法的 `Content-Length`
- body 未接收完整
- 暂不支持的传输模式

这样 LPC 层只需要根据 `error` mapping 统一返回标准 HTTP 错误响应。

## 测试策略

最有价值的测试不是路由，而是协议边界测试。

需要覆盖：

- 一个请求被拆成多次读取
- header 在一次读取里，body 在后续读取里
- 一次读取正好是一个完整请求
- 一次读取里包含多个完整请求
- 合法和非法的 `Content-Length`
- `%xx` 与 `+` 的 URL 解码
- 查询串解码为 mapping
- `application/x-www-form-urlencoded` 表单解码
- header key 规范化为小写
- UTF-8 内容下 `Content-Length` 的正确性

建议的测试层次：

- 驱动层单元测试，验证 parser 与辅助 efun 的行为
- LPC 示例的端到端测试，验证简化后的 `httpd.c` 流程仍然可用

## 暂时保留的开放点

以下问题先不在本轮定死，除非实现时确实需要立刻拍板：

- 重复 query/form key 是否要聚合成数组
- `HEAD` 请求是否要在 `http_build_response` 中特殊处理为“不发 body”
- 将来是否需要额外暴露更底层的元数据，例如原始请求行、连接标志等

## 最终建议

本次建议在驱动里实现以下 efun：

- `url_decode`
- `url_encode`
- `http_decode_query`
- `http_decode_form`
- `http_parser_create`
- `http_parser_feed`
- `http_parser_close`
- `http_build_response`

这是一组“最小但足够有用”的能力集合，能明显减轻 LPC 里的协议胶水负担，同时又保留应用控制权在 LPC 层。
