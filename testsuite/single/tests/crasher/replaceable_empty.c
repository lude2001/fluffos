int my_dummy() { return 1; }

void do_tests() {
#ifdef __PACKAGE_CONTRIB__
    ASSERT_EQ(0, catch(replaceable(this_object(), ({}))));
    ASSERT_EQ(0, catch(replaceable(this_object(), ({ "my_dummy" }))));
#endif
    ASSERT(1);
}
