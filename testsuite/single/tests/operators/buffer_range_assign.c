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

  {
    buffer rhs = allocate_buffer(3);

    b = allocate_buffer(4);
    b[0] = 'a';
    b[1] = 'b';
    b[2] = 'c';
    b[3] = 'd';
    rhs[0] = 'X';
    rhs[1] = 'Y';
    rhs[2] = 'Z';
    b[1..1] = rhs;

    ASSERT_EQ(6, sizeof(b));
    ASSERT_EQ('a', b[0]);
    ASSERT_EQ('X', b[1]);
    ASSERT_EQ('Y', b[2]);
    ASSERT_EQ('Z', b[3]);
    ASSERT_EQ('c', b[4]);
    ASSERT_EQ('d', b[5]);
  }

  {
    buffer rhs = allocate_buffer(1);

    b = allocate_buffer(4);
    b[0] = 'a';
    b[1] = 'b';
    b[2] = 'c';
    b[3] = 'd';
    rhs[0] = 'Q';
    b[1..3] = rhs;

    ASSERT_EQ(2, sizeof(b));
    ASSERT_EQ('a', b[0]);
    ASSERT_EQ('Q', b[1]);
  }
}
