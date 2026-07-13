float global_f;

void do_tests() {
  float f;
  mixed m;
  int i;

  f += 1;
  ASSERT_EQ(1, floatp(f));
  ASSERT_EQ(1.0, f);

  global_f += 1;
  ASSERT_EQ(1, floatp(global_f));
  ASSERT_EQ(1.0, global_f);

  f = 0;
  f -= 1;
  ASSERT_EQ(1, floatp(f));
  ASSERT_EQ(-1.0, f);

  f = 3;
  f *= 2;
  ASSERT_EQ(1, floatp(f));
  ASSERT_EQ(6.0, f);

  f = 1;
  f /= 2;
  ASSERT_EQ(1, floatp(f));
  ASSERT_EQ(0.5, f);

  f = 0;
  f += 1;
  f += 0.0;
  ASSERT_EQ(1, floatp(f));

  m = 1;
  m += 0.5;
  ASSERT_EQ(1, floatp(m));
  ASSERT_EQ(1.5, m);

  m = 4;
  m -= 0.5;
  ASSERT_EQ(1, floatp(m));
  ASSERT_EQ(3.5, m);

  m = 3;
  m *= 1.5;
  ASSERT_EQ(1, floatp(m));
  ASSERT_EQ(4.5, m);

  m = 1;
  m /= 0.5;
  ASSERT_EQ(1, floatp(m));
  ASSERT_EQ(2.0, m);

  m = 1.5;
  m += 1;
  ASSERT_EQ(1, floatp(m));
  ASSERT_EQ(2.5, m);

  i = 10;
  i += 1.5;
  ASSERT_EQ(1, intp(i));
  ASSERT_EQ(11, i);
}
