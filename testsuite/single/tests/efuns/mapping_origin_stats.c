void do_tests() {
    mapping stats = mapping_origin_stats();
    string path = base_name(this_object());

    ASSERT(mapp(stats));
    ASSERT(intp(stats[path]));
    ASSERT(stats[path] > 0);
}
