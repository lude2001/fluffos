void do_tests() {
    for (int i = 0; i < 10; i++) {
	ASSERT(sizeof(allocate(i)) == i);
	ASSERT(filter(allocate(i), (: $1 :)) == ({}));
    }
    ASSERT(allocate(0) == ({}));
    ASSERT(catch(allocate(-10)));
    ASSERT_EQ(({ 0, 1, 2, 3 }), allocate(4, (: $1 :)));
    ASSERT(catch(allocate(5, function(int i) {
        if (i == 2) error("boom");
        return i;
    })));
}
