---
layout: doc
title: sockets / http_response_parser_close
---
# http_response_parser_close

### 名称

    http_response_parser_close() - 释放增量式 HTTP 响应解析器句柄

### 语法

    void http_response_parser_close(mixed parser);

### 说明

    http_response_parser_close() 会释放由 `http_response_parser_create()`
    创建的解析器句柄，并丢弃该解析器当前缓存的所有数据。

### 示例

    mixed parser = http_response_parser_create();
    http_response_parser_close(parser);

### 另见

    http_response_parser_create(3), http_response_parser_feed(3)
