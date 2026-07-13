void do_tests() {
  buffer b = allocate_buffer(4);

  ASSERT_EQ(1, write_buffer(b, 0, "abcd"));
  ASSERT_EQ("abcd", read_buffer(b, 0, 4));

  ASSERT_EQ(0, write_buffer(b, 2147483647, "xy"));
  ASSERT_EQ(0, write_buffer(b, 4, "z"));
  ASSERT_EQ(0, write_buffer(b, 3, "zz"));
  ASSERT_EQ(0, write_buffer(b, -2147483647, "z"));

  ASSERT_EQ("abcd", read_buffer(b, 0, 4));
}
