---
layout: doc
title: sockets / http_response_parser_create
---
# http_response_parser_create

### 名称

    http_response_parser_create() - 创建一个增量式 HTTP 响应解析器

### 语法

    mixed http_response_parser_create();

### 说明

    http_response_parser_create() 会创建一个解析器句柄，可传给
    `http_response_parser_feed()` 和 `http_response_parser_close()` 使用。

    返回的句柄对 LPC 代码来说是不透明的，并会在多次 `feed` 调用之间保留
    尚未解析完成的响应字节数据。

### 返回值

    返回一个不透明的解析器句柄。

### 示例

    mixed parser = http_response_parser_create();

### 另见

    http_response_parser_feed(3), http_response_parser_close(3)
