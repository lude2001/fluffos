void do_tests() {
  int min_int;
  int value;

  min_int = 0x7fffffffffffffff + 1;

  ASSERT_EQ(min_int, min_int / -1);
  ASSERT_EQ(0, min_int % -1);

  value = min_int;
  value /= -1;
  ASSERT_EQ(min_int, value);

  value = min_int;
  value %= -1;
  ASSERT_EQ(0, value);
}
