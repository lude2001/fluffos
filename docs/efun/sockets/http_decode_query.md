---
layout: doc
title: sockets / http_decode_query
---
# http_decode_query

### 名称

    http_decode_query() - 将查询字符串解码为 mapping

### 语法

    mapping http_decode_query(string query);

### 说明

    http_decode_query() 会把类似 `a=1&b=2` 的查询字符串解码为一个
    mapping。

    key 和 value 都会进行 URL 解码。空值会以空字符串保留。重复 key
    以最后一次出现的值为准。

    如果输入为空字符串，则返回空 mapping。

### 返回值

    返回由解码后 key/value 组成的 mapping。

### 示例

    http_decode_query("file=%2Fadm%2Ffoo.c&name=hello+world")
    // 返回 ([ "file": "/adm/foo.c", "name": "hello world" ])

### 另见

    url_decode(3), http_decode_form(3), http_parser_feed(3)
