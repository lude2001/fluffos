string terminal_colour_replace(string s) {
    error("boom from terminal_colour_replace");
}

void do_tests() {
#ifdef __PACKAGE_CONTRIB__
    mapping codes = ([ "RED" : "<r>", "RESET" : "<0>" ]);
    ASSERT_EQ(0, catch(terminal_colour("%^RED%^hello world%^RESET%^", codes)));
#endif
    ASSERT(1);
}
