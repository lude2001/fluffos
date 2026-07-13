void do_tests() {
  buffer b = to_buffer("ab");

  foreach(int ref c in b) {
    c++;
  }

  ASSERT_EQ('b', b[0]);
  ASSERT_EQ('c', b[1]);
}
