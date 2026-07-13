inherit "/inherit/tests";

#define DIR "/data/recompile_object"
#define THING DIR + "/thing"

private void write_src(string body) {
  ASSERT2(write_file(THING + ".c", body, 1), "write " + THING);
}

private void write_src2(string file, string body) {
  ASSERT2(write_file(file, body, 1), "write " + file);
}

private void cleanup() {
  foreach (string prog in ({ THING, DIR + "/seppuku", DIR + "/rp_thing",
                             DIR + "/rp_parent", DIR + "/virt",
                             DIR + "/vbase", DIR + "/initbad" })) {
    foreach (object ob in children(prog)) {
      if (ob) destruct(ob);
    }
  }
  rm(THING + ".c");
  rm(DIR + "/seppuku.c");
  rm(DIR + "/rp_thing.c");
  rm(DIR + "/rp_parent.c");
  rm(DIR + "/vbase.c");
  rm(DIR + "/initbad.c");
  rmdir(DIR);
}

private void run_checks() {
  object ob, c1, c2;
  function fp;
  string err;
  int n;

  ASSERT(mkdir(DIR) || file_size(DIR) == -2);

  write_src("int count;\n" +
            "private int secret = 3;\n" +
            "int gone = 1;\n" +
            "int bump() { return ++count; }\n" +
            "int peek() { return secret; }\n" +
            "void poke(int v) { secret = v; }\n" +
            "string version() { return \"v1\"; }\n");

  ob = load_object(THING);
  c1 = new(THING);
  c2 = new(THING);

  ob->bump();
  c1->bump(); c1->bump();
  c2->bump(); c2->bump(); c2->bump();
  ob->poke(10); c1->poke(11); c2->poke(12);

  write_src("int count;\n" +
            "private int secret = 3;\n" +
            "int extra = 7;\n" +
            "int bump() { return ++count; }\n" +
            "int peek() { return secret; }\n" +
            "void poke(int v) { secret = v; }\n" +
            "int query_extra() { return extra; }\n" +
            "function get_bump_fp() { return (: bump :); }\n" +
            "void arm() { call_out(\"tick\", 100); }\n" +
            "void disarm() { remove_call_out(\"tick\"); }\n" +
            "int armed() { return find_call_out(\"tick\") != -1; }\n" +
            "void tick() { }\n" +
            "string version() { return \"v2\"; }\n");

  n = recompile_object(ob);
  ASSERT_EQ(3, n);
  ASSERT(find_object(THING) == ob);
  ASSERT(objectp(c1) && objectp(c2));
  ASSERT_EQ("v2", ob->version());
  ASSERT_EQ("v2", c1->version());
  ASSERT_EQ("v2", c2->version());
  ASSERT_EQ(2, ob->bump());
  ASSERT_EQ(3, c1->bump());
  ASSERT_EQ(4, c2->bump());
  ASSERT_EQ(10, ob->peek());
  ASSERT_EQ(11, c1->peek());
  ASSERT_EQ(12, c2->peek());
  ASSERT_EQ(7, ob->query_extra());
  ASSERT_EQ(7, c1->query_extra());
  ASSERT(stringp(catch(fetch_variable("gone", ob))));
  ASSERT(stringp(catch(recompile_object(c1))));

  fp = ob->get_bump_fp();
  ASSERT_EQ(3, evaluate(fp));
  ob->arm();

  write_src("int count;\n" +
            "private int secret = 3;\n" +
            "int extra = 7;\n" +
            "int bump() { return ++count; }\n" +
            "int peek() { return secret; }\n" +
            "int query_extra() { return extra; }\n" +
            "void disarm() { remove_call_out(\"tick\"); }\n" +
            "int armed() { return find_call_out(\"tick\") != -1; }\n" +
            "void tick() { }\n" +
            "int self_update() { return recompile_object(this_object()); }\n" +
            "string version() { return \"v3\"; }\n");
  ASSERT_EQ(3, recompile_object(ob));
  ASSERT_EQ("v3", ob->version());
  ASSERT_EQ(4, ob->bump());
  ASSERT_EQ(1, ob->armed());
  err = catch(evaluate(fp));
  ASSERT2(stringp(err) && strsrch(err, "was recompiled") != -1, "stale fp: " + err);
  ob->disarm();

  err = catch(ob->self_update());
  ASSERT2(stringp(err) && strsrch(err, "currently executing") != -1, "self: " + err);

  rm(THING + ".c");
  ASSERT(stringp(catch(recompile_object(ob))));
  ASSERT_EQ("v3", ob->version());

  write_src2(DIR + "/vbase.c",
             "int n;\n" +
             "int bump() { return ++n; }\n" +
             "string vv() { return \"vb1\"; }\n");
  ob = load_object(DIR + "/virt");
  ASSERT(objectp(ob));
  ASSERT(virtualp(ob));
  ASSERT_EQ(DIR + "/virt", file_name(ob));
  ob->bump();
  ASSERT_EQ(2, ob->bump());
  write_src2(DIR + "/vbase.c",
             "int n;\n" +
             "int bump() { return ++n; }\n" +
             "string vv() { return \"vb2 bigger\"; }\n");
  ASSERT(recompile_object(ob) >= 1);
  ASSERT(ob == find_object(DIR + "/virt"));
  ASSERT(virtualp(ob));
  ASSERT_EQ("vb2 bigger", ob->vv());
  ASSERT_EQ(3, ob->bump());

  write_src2(DIR + "/rp_parent.c",
             "int p1;\nint p2;\nint p3;\n" +
             "int get_p3() { return p3; }\n");
  write_src2(DIR + "/rp_thing.c",
             "inherit \"" + DIR + "/rp_parent\";\n" +
             "int a1;\n" +
             "void do_rp() { replace_program(\"" + DIR + "/rp_parent\"); }\n" +
             "string version() { return \"rp1\"; }\n");
  ob = load_object(DIR + "/rp_thing");
  c1 = new(DIR + "/rp_thing");
  write_src2(DIR + "/rp_thing.c",
             "int x = 1;\n" +
             "int kick() {\n" +
             "  foreach (object o in children(\"" + DIR + "/rp_thing\"))\n" +
             "    if (o != this_object()) catch(o->do_rp());\n" +
             "  return 1;\n" +
             "}\n" +
             "int kicked = kick();\n" +
             "string version() { return \"rp2\"; }\n");
  ASSERT_EQ(2, recompile_object(ob));
  ASSERT_EQ("rp2", ob->version());
  ASSERT_EQ("rp2", c1->version());

  write_src2(DIR + "/seppuku.c",
             "int x = 1;\n" +
             "string version() { return \"s1\"; }\n");
  ob = load_object(DIR + "/seppuku");
  c1 = new(DIR + "/seppuku");
  write_src2(DIR + "/seppuku.c",
             "int x = 2;\n" +
             "int go() { destruct(this_object()); return 0; }\n" +
             "int boom = go();\n" +
             "string version() { return \"s2\"; }\n");
  ASSERT_EQ(0, recompile_object(ob));
  ASSERT(!objectp(ob));
  ASSERT(!objectp(c1));
  ASSERT(!find_object(DIR + "/seppuku"));

  write_src2(DIR + "/initbad.c",
             "int n = 1;\n" +
             "int set_n(int v) { return n = v; }\n" +
             "string version() { return \"ib1\"; }\n");
  ob = load_object(DIR + "/initbad");
  c1 = new(DIR + "/initbad");
  c1->set_n(7);
  write_src2(DIR + "/initbad.c",
             "int n = 1;\n" +
             "int go() { error(\"init boom\\n\"); return 0; }\n" +
             "int boom = go();\n" +
             "int set_n(int v) { return n = v; }\n" +
             "string version() { return \"ib2\"; }\n");
  ASSERT_EQ(0, recompile_object(ob));
  ASSERT(objectp(ob) && objectp(c1));
  ASSERT_EQ("ib2", ob->version());
  ASSERT_EQ(1, ob->set_n(1));

  ASSERT_EQ(1, recompile_object(find_object("/single/simul_efun")));
  ASSERT_EQ(1, same(({ 1, 2 }), ({ 1, 2 })));

  err = catch(recompile_object(find_object("/single/master")));
  ASSERT2(stringp(err) && strsrch(err, "currently executing") != -1, "master: " + err);
}

void do_tests() {
  string err = catch(run_checks());

  cleanup();
  if (err) error(err);
}
