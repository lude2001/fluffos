int clean_up(int inherited) { return 0; }

void do_tests() {
    object master = find_object("/single/master");

    ASSERT_EQ(0, catch(set_clean_up(this_object(), 3600)));
    ASSERT_EQ(0, catch(set_clean_up(this_object())));

    ASSERT_EQ(0, catch(set_clean_up(master, 60)));
    ASSERT_EQ(0, catch(set_clean_up(master)));
}
