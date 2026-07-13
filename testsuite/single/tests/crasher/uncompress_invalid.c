void do_tests() {
#ifdef __PACKAGE_COMPRESS__
    ASSERT_NE(0, catch(uncompress(allocate_buffer(1))));
#endif
    ASSERT(1);
}
