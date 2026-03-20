---
layout: doc
title: sockets / url_decode
---
# url_decode

### 名称

    url_decode() - 解码百分号编码的 URL 字符串

### 语法

    string url_decode(string value);

### 说明

    url_decode() 会解码 `%xx` 形式的转义序列，同时把 `+` 转换为空格。
    这与查询字符串以及 `application/x-www-form-urlencoded`
    请求体常见所需的行为一致。

    非法的 `%` 转义序列会原样保留。

### 返回值

    返回解码后的字符串。

### 示例

    url_decode("%2Fadm%2Ffoo+bar")
    // 返回 "/adm/foo bar"

### 另见

    url_encode(3), http_decode_query(3), http_decode_form(3)
