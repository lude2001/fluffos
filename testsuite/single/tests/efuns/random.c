void do_tests() {
    ASSERT_EQ(0, random(0));
    ASSERT_EQ(0, random(-1));

    for (int i = 0; i < 100; i++) {
	ASSERT(random(5) >= 0);
	ASSERT(random(5) < 5);
    }
}
