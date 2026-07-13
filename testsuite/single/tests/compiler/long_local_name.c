void do_tests() {
    string longname = "";
    string src;
    object ob;
    int i;

    // This branch's old lexer caps one physical line at MAXLINE=4096, so the
    // official >4096 identifier regression cannot be expressed directly in LPC
    // source here. Pin the near-limit case that still reaches normal parsing.
    for (i = 0; i < 4000; i++) {
        longname += "a";
    }

    src = "int " + longname + ";\n"
          "int query_value() { return 1; }\n";

    rm("/gen_long_local.c");
    ASSERT(write_file("/gen_long_local.c", src));
    ob = load_object("/gen_long_local");
    ASSERT(ob);
    ASSERT_EQ(1, ob->query_value());

    destruct(ob);
    rm("/gen_long_local.c");
}
