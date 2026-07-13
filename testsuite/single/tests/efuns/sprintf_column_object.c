void do_tests() {
    string r;

    r = sprintf("%-=20O", ({ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 }));
    ASSERT(stringp(r));
    ASSERT(strlen(r) > 0);

    r = sprintf("%-=40O", ({ "alpha", "beta", "gamma", "delta", "epsilon" }));
    ASSERT(stringp(r));

    r = sprintf("%-=15O%-=15O", ({ 1, 2, 3, 4, 5 }), ({ 6, 7, 8, 9, 10 }));
    ASSERT(stringp(r));

    ASSERT(stringp(sprintf("%-=10s", "a fairly long string that wraps")));
}
