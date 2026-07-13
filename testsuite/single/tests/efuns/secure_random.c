void do_tests() {
    ASSERT_EQ(0, secure_random(0));
    ASSERT_EQ(0, secure_random(-1));

    for (int i = 0; i < 100; i++) {
	ASSERT(secure_random(5) >= 0);
	ASSERT(secure_random(5) < 5);
    }
}
