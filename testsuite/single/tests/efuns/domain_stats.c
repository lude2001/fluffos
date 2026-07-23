void do_tests() {
    mapping stats = domain_stats();

    foreach (string name, mapping info in stats) {
        ASSERT(member_array("mapping_count", keys(info)) != -1);
        return;
    }

    ASSERT(0);
}
