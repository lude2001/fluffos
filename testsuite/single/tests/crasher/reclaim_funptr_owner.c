function saved_fp;

void do_tests() {
    object helper = clone_object("/clone/reclaim_fp_helper");

    saved_fp = helper->make_fp();
    ASSERT(functionp(saved_fp));

    destruct(helper);
    reclaim_objects();

    call_out(saved_fp, 1);
    reclaim_objects();

    saved_fp = 0;
    ASSERT(1);
}
