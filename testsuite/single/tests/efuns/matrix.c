void do_tests() {
  float* m = id_matrix();
  m = lookat_rotate2(m, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1);
  ASSERT_NE(0, catch(translate(({}), 1.0, 1.0, 1.0)));
  ASSERT_NE(0, catch(scale(({}), 1.0, 1.0, 1.0)));
  ASSERT_NE(0, catch(rotate_x(({}), 1.0)));
  ASSERT_NE(0, catch(lookat_rotate(({}), 1.0, 1.0, 1.0)));
}
