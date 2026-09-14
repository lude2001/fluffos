int msame(mapping x, mapping y) {
    mixed key;

    if (sizeof(keys(x)) != sizeof(keys(y))) {
        return 0;
    }

    foreach (key in keys(x)) {
        if (x[key] != y[key]) {
            return 0;
        }
    }

    return 1;
}

void do_tests() {
    int parser;
    mapping result;
    mapping request;
    mixed *requests;

    parser = http_parser_create();
    ASSERT(parser > 0);

    result = http_parser_feed(parser,
                              "GET /demo?a=1&b=hello+world HTTP/1.1\r\n"
                              "Host: example.com\r\n"
                              "X-Test: Value\r\n"
                              "\r\n");
    ASSERT_EQ(0, result["need_more"]);
    ASSERT_EQ(0, result["error"]);
    requests = result["requests"];
    ASSERT_EQ(1, sizeof(requests));
    request = requests[0];
    ASSERT_EQ("GET", request["method"]);
    ASSERT_EQ("/demo", request["path"]);
    ASSERT_EQ("1.1", request["version"]);
    ASSERT_EQ("a=1&b=hello+world", request["query_string"]);
    ASSERT_EQ("1", request["query"]["a"]);
    ASSERT_EQ("hello world", request["query"]["b"]);
    ASSERT_EQ("example.com", request["headers"]["host"]);
    ASSERT_EQ("Value", request["headers"]["x-test"]);
    ASSERT_EQ("", request["body"]);
    ASSERT(msame(([]), request["form"]));
    http_parser_close(parser);

    parser = http_parser_create();
    result = http_parser_feed(parser,
                              "POST /submit HTTP/1.1\r\n"
                              "Host: example.com\r\n"
                              "Content-Type: application/x-www-form-urlencoded\r\n"
                              "Content-Length: 16\r\n"
                              "\r\n"
                              "name=hello");
    ASSERT_EQ(1, result["need_more"]);
    ASSERT_EQ(0, result["error"]);
    ASSERT_EQ(0, sizeof(result["requests"]));

    result = http_parser_feed(parser, "+world");
    ASSERT_EQ(0, result["need_more"]);
    ASSERT_EQ(0, result["error"]);
    requests = result["requests"];
    ASSERT_EQ(1, sizeof(requests));
    request = requests[0];
    ASSERT_EQ("POST", request["method"]);
    ASSERT_EQ("/submit", request["path"]);
    ASSERT_EQ("name=hello+world", request["body"]);
    ASSERT_EQ("hello world", request["form"]["name"]);
    ASSERT_EQ("application/x-www-form-urlencoded", request["headers"]["content-type"]);
    http_parser_close(parser);

    parser = http_parser_create();
    result = http_parser_feed(parser,
                              "GET /bad HTTP/1.1\r\n"
                              "Broken-Header\r\n"
                              "\r\n");
    ASSERT_EQ(0, result["need_more"]);
    ASSERT_EQ(0, sizeof(result["requests"]));
    ASSERT_EQ(400, result["error"]["status"]);
    ASSERT_EQ("malformed_header", result["error"]["reason"]);
    http_parser_close(parser);

    parser = http_parser_create();
    result = http_parser_feed(parser,
                              "GET /one HTTP/1.1\r\n"
                              "Host: example.com\r\n"
                              "\r\n"
                              "GET /two?x=2 HTTP/1.1\r\n"
                              "HOST: example.org\r\n"
                              "\r\n");
    ASSERT_EQ(0, result["need_more"]);
    ASSERT_EQ(0, result["error"]);
    requests = result["requests"];
    ASSERT_EQ(2, sizeof(requests));
    ASSERT_EQ("/one", requests[0]["path"]);
    ASSERT_EQ("example.com", requests[0]["headers"]["host"]);
    ASSERT_EQ("/two", requests[1]["path"]);
    ASSERT_EQ("2", requests[1]["query"]["x"]);
    ASSERT_EQ("example.org", requests[1]["headers"]["host"]);
    http_parser_close(parser);
}
