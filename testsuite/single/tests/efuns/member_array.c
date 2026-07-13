void do_tests() {
    function f = (: 1 :);

    ASSERT(member_array('c', "foo") == -1);
    ASSERT(member_array('b', "abar") == 1);
    ASSERT(member_array('y', "xyzzy") == 1);
    ASSERT(member_array('y', "xyzzy", 2) == 4);
    ASSERT(member_array(2, ({ 1, 2, 3 })) == 1);
    ASSERT(member_array("foo", ({ 1, "foo", 3 })) == 1);

    ASSERT(member_array(99, ({ 1, 2, 3 }), 0, 2) == -1);
    ASSERT_EQ(2, member_array((: $1 > 10 :), ({ 1, 5, 42, 77 }), 0, 4));
    ASSERT_EQ(-1, member_array((: $1 > 100 :), ({ 1, 5, 42 }), 0, 4));
    ASSERT_EQ(3, member_array((: $1 > 10 :), ({ 1, 5, 42, 77 }), 0, 6));
    ASSERT_EQ(3, member_array((: $1 > 10 :), ({ 42, 1, 5, 77 }), 1, 4));
    ASSERT_EQ(1, member_array(f, ({ 0, f, 2 })));
}
