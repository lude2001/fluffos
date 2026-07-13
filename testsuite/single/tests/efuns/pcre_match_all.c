void do_tests() {
    mixed r = pcre_match_all("a1 b2 c3", "([a-z])([0-9])");
    ASSERT_EQ(3, sizeof(r));
    ASSERT_EQ(({ "a1", "a", "1" }), r[0]);

    r = pcre_match_all("hi there", "(\\w*)");
    ASSERT_EQ(3, sizeof(r));
    ASSERT_EQ(({ "hi", "hi" }), r[0]);
    ASSERT_EQ(({ "", "" }), r[1]);
    ASSERT_EQ(({ "there", "there" }), r[2]);

    r = pcre_match_all("abc", "x*");
    ASSERT_EQ(3, sizeof(r));
    ASSERT_EQ(({ "" }), r[0]);

    r = pcre_match_all("a中b", "([ab]*)");
    ASSERT_EQ(3, sizeof(r));
    ASSERT_EQ(({ "a", "a" }), r[0]);
    ASSERT_EQ(({ "", "" }), r[1]);
    ASSERT_EQ(({ "b", "b" }), r[2]);
}
