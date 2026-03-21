---
layout: doc
title: sockets / http_build_response
---
# http_build_response

### 名称

    http_build_response() - 构造一个 HTTP/1.1 响应字符串

### 语法

    string http_build_response(int status, mapping headers, string body);

### 说明

    http_build_response() 会根据状态码、响应头 mapping 和响应体字符串，
    组装出一条完整的 HTTP/1.1 响应报文。

    该 efun 会自动补充：

    - 如果尚未提供，则补上 `Content-Length`
    - 如果尚未提供，则补上 `Connection: close`
    - 为常见状态码补上标准 reason phrase

    `headers` 中传入的 header value 会按原样保留。

### 返回值

    返回完整的响应字符串，其中包含状态行、响应头、空行以及响应体。

### 示例

    string body = "{\"ok\":true}";
    string res = http_build_response(200, ([
        "content-type": "application/json; charset=UTF-8",
    ]), body);

### 另见

    http_parser_feed(3)
