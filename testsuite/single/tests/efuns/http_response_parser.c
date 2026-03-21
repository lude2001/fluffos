void do_tests() {
    int parser;
    mapping result;
    mapping response;
    mixed *responses;

    parser = http_response_parser_create();
    ASSERT(parser > 0);

    result = http_response_parser_feed(parser,
                                       "HTTP/1.1 200 OK\r\n"
                                       "Content-Type: application/json\r\n"
                                       "Content-Length: 2\r\n"
                                       "\r\n"
                                       "{}");
    ASSERT_EQ(0, result["need_more"]);
    ASSERT_EQ(0, result["error"]);
    responses = result["responses"];
    ASSERT_EQ(1, sizeof(responses));
    response = responses[0];
    ASSERT_EQ("HTTP/1.1", response["version"]);
    ASSERT_EQ(200, response["status_code"]);
    ASSERT_EQ("OK", response["reason"]);
    ASSERT_EQ("application/json", response["headers"]["content-type"]);
    ASSERT_EQ("2", response["headers"]["content-length"]);
    ASSERT_EQ("{}", response["body"]);
    http_response_parser_close(parser);

    parser = http_response_parser_create();
    result = http_response_parser_feed(parser,
                                       "HTTP/1.1 201 Created\r\n"
                                       "Content-Length: 11\r\n"
                                       "X-Test: Va");
    ASSERT_EQ(1, result["need_more"]);
    ASSERT_EQ(0, result["error"]);
    ASSERT_EQ(0, sizeof(result["responses"]));

    result = http_response_parser_feed(parser,
                                       "lue\r\n"
                                       "\r\n"
                                       "hello world");
    ASSERT_EQ(0, result["need_more"]);
    ASSERT_EQ(0, result["error"]);
    responses = result["responses"];
    ASSERT_EQ(1, sizeof(responses));
    response = responses[0];
    ASSERT_EQ("HTTP/1.1", response["version"]);
    ASSERT_EQ(201, response["status_code"]);
    ASSERT_EQ("Created", response["reason"]);
    ASSERT_EQ("Value", response["headers"]["x-test"]);
    ASSERT_EQ("hello world", response["body"]);
    http_response_parser_close(parser);

    parser = http_response_parser_create();
    result = http_response_parser_feed(parser,
                                       "HTTP/1.1 204 No Content\r\n"
                                       "\r\n"
                                       "HTTP/1.1 200 OK\r\n"
                                       "Content-Length: 5\r\n"
                                       "\r\n"
                                       "hello");
    ASSERT_EQ(0, result["need_more"]);
    ASSERT_EQ(0, result["error"]);
    responses = result["responses"];
    ASSERT_EQ(2, sizeof(responses));
    ASSERT_EQ(204, responses[0]["status_code"]);
    ASSERT_EQ("", responses[0]["body"]);
    ASSERT_EQ(200, responses[1]["status_code"]);
    ASSERT_EQ("hello", responses[1]["body"]);
    http_response_parser_close(parser);

    parser = http_response_parser_create();
    result = http_response_parser_feed(parser,
                                       "NOT_HTTP\r\n"
                                       "\r\n");
    ASSERT_EQ(0, result["need_more"]);
    ASSERT_EQ(0, sizeof(result["responses"]));
    ASSERT_EQ(400, result["error"]["status"]);
    ASSERT_EQ("malformed_status_line", result["error"]["reason"]);
    http_response_parser_close(parser);

    parser = http_response_parser_create();
    result = http_response_parser_feed(parser,
                                       "HTTP/1.1 200 OK\r\n"
                                       "Transfer-Encoding: chunked\r\n"
                                       "\r\n");
    ASSERT_EQ(0, result["need_more"]);
    ASSERT_EQ(0, sizeof(result["responses"]));
    ASSERT_EQ(400, result["error"]["status"]);
    ASSERT_EQ("unsupported_transfer_encoding", result["error"]["reason"]);
    http_response_parser_close(parser);
}
