void do_tests() {
  string range_err = "*Buffer byte value out of range: must be 0..255.\n";
  string item_err = "*Illegal array item in buffer conversion: must be ints 0..255.\n";
  buffer b = allocate_buffer(2);
  buffer c;
  mixed bad = 3.5;

  b[0] = 0;
  b[1] = 255;
  ASSERT_EQ(0, b[0]);
  ASSERT_EQ(255, b[1]);

  ASSERT_EQ(range_err, catch(b[0] = -1));
  ASSERT_EQ(range_err, catch(b[0] = 256));
  ASSERT_EQ(range_err, catch(b[0]--));
  ASSERT_EQ(0, b[0]);
  ASSERT_EQ(range_err, catch(b[1]++));
  ASSERT_EQ(255, b[1]);

  b[0] = 254;
  ASSERT_EQ(255, b[0] += 1);
  ASSERT_EQ(range_err, catch(b[0] += 1));
  ASSERT_EQ(255, b[0]);

  b[0] = 1;
  ASSERT_EQ(0, b[0] -= 1);
  ASSERT_EQ(range_err, catch(b[0] -= 1));
  ASSERT_EQ(0, b[0]);

  c = to_buffer("AB");
  ASSERT_EQ(2, sizeof(c));
  ASSERT_EQ('A', c[0]);
  ASSERT_EQ('B', c[1]);

  c = to_buffer(({ 1, 2, 255 }));
  ASSERT_EQ(3, sizeof(c));
  ASSERT_EQ(1, c[0]);
  ASSERT_EQ(2, c[1]);
  ASSERT_EQ(255, c[2]);

  c += "C";
  ASSERT_EQ(4, sizeof(c));
  ASSERT_EQ('C', c[3]);

  c = c + ({ 0 });
  ASSERT_EQ(5, sizeof(c));
  ASSERT_EQ(0, c[4]);

  ASSERT_EQ(item_err, catch(to_buffer(({ "x" }))));
  ASSERT(catch(to_buffer(bad)));
}
