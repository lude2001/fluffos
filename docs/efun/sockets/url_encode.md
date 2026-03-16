---
layout: doc
title: sockets / url_encode
---
# url_encode

### NAME

    url_encode() - percent-encode a URL string

### SYNOPSIS

    string url_encode(string value);

### DESCRIPTION

    url_encode() percent-encodes all characters outside the unreserved URL
    set (`A-Z`, `a-z`, `0-9`, `-`, `_`, `.`, `~`).

    Spaces are encoded as `%20`.

### RETURN VALUE

    Returns the encoded string.

### EXAMPLE

    url_encode("/adm/foo bar")
    // returns "%2Fadm%2Ffoo%20bar"

### SEE ALSO

    url_decode(3), http_decode_query(3), http_decode_form(3)
