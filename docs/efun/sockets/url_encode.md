---
layout: doc
title: sockets / url_encode
---
# url_encode

### 名称

    url_encode() - 对 URL 字符串做百分号编码

### 语法

    string url_encode(string value);

### 说明

    url_encode() 会把 URL 中所有不属于保留安全字符集合的字符进行
    百分号编码。该安全字符集合为：
    `A-Z`、`a-z`、`0-9`、`-`、`_`、`.`、`~`。

    空格会被编码为 `%20`。

### 返回值

    返回编码后的字符串。

### 示例

    url_encode("/adm/foo bar")
    // 返回 "%2Fadm%2Ffoo%20bar"

### 另见

    url_decode(3), http_decode_query(3), http_decode_form(3)
