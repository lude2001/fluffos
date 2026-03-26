// Test JSON sefun

void do_tests() {
  string content = "";
  mapping m;
  mixed *arr;

  ASSERT_EQ(0, json_decode("0"));
  ASSERT_EQ(0.0, json_decode("0e0"));
  ASSERT_EQ(0.0, json_decode("0E12328"));
  ASSERT_EQ(to_float(0), json_decode("0.000"));
  ASSERT_EQ(1, json_decode("true"));
  ASSERT_EQ(0, json_decode("false"));
  ASSERT_EQ(0, json_decode("null"));

  // Decoding
  ASSERT_EQ(({0,1,2,3,4,5,0,4,0.0}), json_decode("[0,1,2,3,4,5,-0,4,0.0]"));
  ASSERT_EQ(({}), json_decode("[]"));

  ASSERT_EQ("你好", json_decode("\"\\u4f60\\u597d\""));
  ASSERT_EQ("😄", json_decode("\"\\ud83d\\ude04\""));

  // Encoding
  ASSERT_EQ("[1,2,3,4]", json_encode(({1,2,3,4})));
  ASSERT_EQ("null", json_encode(([])[0]));
  ASSERT_EQ(([ "ok" : 1 ]), json_decode(json_encode(([ 1 : "ignored", "ok" : 1 ]))));
  ASSERT_EQ(([ "obj" : 0 ]), json_decode(json_encode(([ "obj" : this_object() ]))));

  content = read_file("/single/tests/std/test.json");
  ASSERT(content);
  ASSERT(json_encode(json_decode(content)));

  // Handle \e
  content = "\e你好";
  ASSERT_EQ(content, json_decode(json_encode(content)));

  // Handle \x in LPC
  content = "\x1b你好";
  ASSERT_EQ(content, json_decode(json_encode(content)));

  // e2e
  content = read_file("/single/tests/std/test2.json");
  ASSERT(content);
  ASSERT(json_encode(json_decode(content)));

  arr = allocate(2);
  arr[0] = 1;
  arr[1] = arr;
  ASSERT_EQ("[1,null]", json_encode(arr));

  m = ([ "ok" : 1 ]);
  m["self"] = m;
  ASSERT_EQ(([ "ok" : 1, "self" : 0 ]), json_decode(json_encode(m)));

  // Formatting
  ASSERT_EQ("{\n  \"a\": 1,\n  \"b\": [\n    2,\n    3\n  ]\n}",
            json_format("{\"a\":1,\"b\":[2,3]}"));
  ASSERT_EQ("{\n    \"nested\": {\n        \"ok\": true\n    },\n    \"text\": \"a,\\\"b\\\"{}\"\n}",
            json_format("{\n\t\"text\":\"a,\\\"b\\\"{}\", \"nested\": {\"ok\":true}}", 4));
  ASSERT(catch(json_format("{")));

  ASSERT(catch(json_decode("{")));
  ASSERT(catch(json_decode("\"\\ud83d\"")));
  ASSERT(catch(json_decode("18446744073709551615")));

}
