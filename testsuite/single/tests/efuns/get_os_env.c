void do_tests() {
    ASSERT(stringp(get_os_env("PATH")));
    ASSERT(undefinedp(get_os_env("FLUFFOS_TEST_RO")));

    ASSERT(catch(get_os_env("HOME")));
    ASSERT(catch(get_os_env("")));

    ASSERT(set_os_env("FLUFFOS_TEST_RW", "hello"));
    ASSERT_EQ("hello", get_os_env("FLUFFOS_TEST_RW"));

    ASSERT(set_os_env("FLUFFOS_TEST_RW"));
    ASSERT(undefinedp(get_os_env("FLUFFOS_TEST_RW")));

    ASSERT(catch(set_os_env("PATH", "/tmp")));
    ASSERT(catch(set_os_env("FLUFFOS_EVIL", "x")));
}
