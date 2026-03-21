---
layout: doc
title: sockets / http_parser_create
---
# http_parser_create

### 名称

    http_parser_create() - 创建一个增量式 HTTP 请求解析器

### 语法

    mixed http_parser_create();

### 说明

    http_parser_create() 会创建一个解析器句柄，可传给
    `http_parser_feed()` 和 `http_parser_close()` 使用。

    该句柄会在多次调用之间保留解析器缓存的字节流，因此 LPC 代码可以按
    chunk 逐步喂入 socket 数据。

### 返回值

    返回一个解析器句柄。

### 示例

    mixed parser = http_parser_create();

### 另见

    http_parser_feed(3), http_parser_close(3)
