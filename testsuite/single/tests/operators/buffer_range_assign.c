void do_tests() {
  buffer b = allocate_buffer(4);

  b[0] = 1;
  b[1] = 2;
  b[2] = 3;
  b[3] = 4;

  b[1..2] = "AB";
  ASSERT_EQ(4, sizeof(b));
  ASSERT_EQ(1, b[0]);
  ASSERT_EQ('A', b[1]);
  ASSERT_EQ('B', b[2]);
  ASSERT_EQ(4, b[3]);

  b[1..2] = ({ 200, 255 });
  ASSERT_EQ(200, b[1]);
  ASSERT_EQ(255, b[2]);

  b[0..1] = to_buffer("xy");
  ASSERT_EQ('x', b[0]);
  ASSERT_EQ('y', b[1]);

  b[1..2] = ({ 7, 8, 9 });
  ASSERT_EQ(5, sizeof(b));
  ASSERT_EQ('x', b[0]);
  ASSERT_EQ(7, b[1]);
  ASSERT_EQ(8, b[2]);
  ASSERT_EQ(9, b[3]);
  ASSERT_EQ(4, b[4]);
}
