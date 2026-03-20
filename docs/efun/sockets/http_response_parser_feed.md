---
layout: doc
title: sockets / http_response_parser_feed
---
# http_response_parser_feed

### 名称

    http_response_parser_feed() - 向增量式 HTTP 响应解析器喂入原始字节数据

### 语法

    mapping http_response_parser_feed(mixed parser, string chunk);

### 说明

    http_response_parser_feed() 会把 `chunk` 追加到解析器缓存中，并返回一个
    包含三个 key 的 mapping：

    - `responses` : 0 个或多个完整响应 mapping 组成的数组
    - `error` : 成功时为 `0`，解析失败时为一个错误 mapping
    - `need_more` : 非 0 表示还需要更多字节才能组成完整响应

    每个响应 mapping 包含：

    - `version`
    - `status_code`
    - `reason`
    - `headers`
    - `body`

    header 名会统一规范化为小写。

    当前解析器支持：

    - 通过 `Content-Length` 定界的响应
    - 无 body 的响应，例如 `1xx`、`204` 和 `304`

    目前不支持 `Transfer-Encoding: chunked`、gzip/deflate 解码，
    也不支持以连接关闭作为结束条件的响应体。

### 返回值

    返回上面描述的结果 mapping。

### 示例

    mixed parser = http_response_parser_create();
    mapping result = http_response_parser_feed(parser,
        "HTTP/1.1 200 OK\r\n"
        "Content-Length: 2\r\n"
        "\r\n"
        "{}");

### 另见

    http_response_parser_create(3), http_response_parser_close(3)
