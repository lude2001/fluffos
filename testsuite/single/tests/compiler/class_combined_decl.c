void do_tests() {
  object o;
  string err;

  err = catch(load_object("/clone/class788_repro"));
  ASSERT_NE(0, err);

  o = load_object("/clone/class788_ok");
  ASSERT_EQ("jeremy", o->probe());
  ASSERT_EQ("kyle", o->probe2());
}
