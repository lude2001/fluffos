void do_tests() {
    string err = catch(load_object("/clone/bad_varargs_void"));

    ASSERT_NE(0, err);
}
