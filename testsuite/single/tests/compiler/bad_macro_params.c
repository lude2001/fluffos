void do_tests() {
    string err = catch(load_object("/clone/bad_macro_params"));
    ASSERT_NE(0, err);
}
