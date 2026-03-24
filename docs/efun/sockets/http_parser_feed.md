---
layout: doc
title: sockets / http_parser_feed
---
# http_parser_feed

### 名称

    http_parser_feed() - 向增量式 HTTP 解析器喂入原始字节数据

### 语法

    mapping http_parser_feed(mixed parser, string chunk);

### 说明

    http_parser_feed() 会把 `chunk` 追加到解析器缓存中，并返回一个
    包含三个 key 的 mapping：

    - `requests` : 0 个或多个完整请求 mapping 组成的数组
    - `error` : 成功时为 `0`，解析失败时为一个错误 mapping
    - `need_more` : 非 0 表示还需要更多字节才能组成完整请求

    每个请求 mapping 包含：

    - `method`
    - `path`
    - `version`
    - `query_string`
    - `query`
    - `headers`
    - `body`
    - `form`

    header 名会统一规范化为小写。

    当前解析器支持按 `Content-Length` 定界的请求体。
    目前不支持 `Transfer-Encoding: chunked`。

### 返回值

    返回上面描述的结果 mapping。

### 示例

    mixed parser = http_parser_create();
    mapping result = http_parser_feed(parser,
        "GET /ping?name=codex HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "\r\n");

### 另见

    http_parser_create(3), http_parser_close(3), http_decode_query(3),
    http_decode_form(3)
