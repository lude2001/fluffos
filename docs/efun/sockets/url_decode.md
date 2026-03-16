---
layout: doc
title: sockets / url_decode
---
# url_decode

### NAME

    url_decode() - decode a percent-encoded URL string

### SYNOPSIS

    string url_decode(string value);

### DESCRIPTION

    url_decode() decodes `%xx` escape sequences and also converts `+` into a
    space. This matches the behavior commonly needed for query strings and
    `application/x-www-form-urlencoded` request bodies.

    Invalid `%` sequences are left in place.

### RETURN VALUE

    Returns the decoded string.

### EXAMPLE

    url_decode("%2Fadm%2Ffoo+bar")
    // returns "/adm/foo bar"

### SEE ALSO

    url_encode(3), http_decode_query(3), http_decode_form(3)
