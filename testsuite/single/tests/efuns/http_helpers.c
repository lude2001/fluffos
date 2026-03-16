void do_tests() {
  mapping query;
  mapping form;
  string response;

  ASSERT_EQ("/adm/foo bar", url_decode("%2Fadm%2Ffoo+bar"));
  ASSERT_EQ("%2Fadm%2Ffoo%20bar", url_encode("/adm/foo bar"));

  query = http_decode_query("a=1&b=2&empty=&encoded=%2Fadm%2Ffoo+bar&encoded2=hello%20world");
  ASSERT_EQ("1", query["a"]);
  ASSERT_EQ("2", query["b"]);
  ASSERT_EQ("", query["empty"]);
  ASSERT_EQ("/adm/foo bar", query["encoded"]);
  ASSERT_EQ("hello world", query["encoded2"]);
  ASSERT_EQ(([]), http_decode_query(""));

  form = http_decode_form("file=%2Fadm%2Ffoo.c&name=hello+world");
  ASSERT_EQ("/adm/foo.c", form["file"]);
  ASSERT_EQ("hello world", form["name"]);
  ASSERT_EQ(([]), http_decode_form(""));

  response = http_build_response(200, ([
    "content-type": "text/plain; charset=UTF-8",
  ]), "你好");

  ASSERT(strsrch(response, "HTTP/1.1 200 OK\r\n") == 0);
  ASSERT(strsrch(response, "content-type: text/plain; charset=UTF-8\r\n") != -1 ||
         strsrch(response, "Content-Type: text/plain; charset=UTF-8\r\n") != -1);
  ASSERT(strsrch(lower_case(response), "connection: close\r\n") != -1);
  ASSERT(strsrch(lower_case(response), "content-length: 6\r\n") != -1);
  ASSERT(strsrch(response, "\r\n\r\n你好") != -1);

  response = http_build_response(200, ([
    "content-type": "text/plain",
    "connection": "keep-alive",
  ]), "ok");
  ASSERT(strsrch(lower_case(response), "connection: keep-alive\r\n") != -1);
}
