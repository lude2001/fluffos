---
layout: doc
title: sockets / http_decode_form
---
# http_decode_form

### 名称

    http_decode_form() - 将表单请求体解码为 mapping

### 语法

    mapping http_decode_form(string body);

### 说明

    http_decode_form() 会把
    `application/x-www-form-urlencoded` 格式的请求体解码为一个 mapping。

    它的解码规则与 `http_decode_query()` 一致：key 和 value 都会做 URL 解码，
    空值会被保留，重复 key 以最后一次出现的值为准。

### 返回值

    返回由解码后 key/value 组成的 mapping。

### 示例

    http_decode_form("file=%2Fadm%2Ffoo.c&name=hello+world")
    // 返回 ([ "file": "/adm/foo.c", "name": "hello world" ])

### 另见

    url_decode(3), http_decode_query(3), http_parser_feed(3)
