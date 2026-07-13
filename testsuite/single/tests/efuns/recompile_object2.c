inherit "/inherit/tests";

#define DIR "/data/recompile_object2"
private void write_src(string file, string body) {
  ASSERT2(write_file(file, body, 1), "write " + file);
}

private void cleanup() {
  foreach (string prog in ({ DIR + "/ct", DIR + "/sh_target", DIR + "/sh_shadow",
                             DIR + "/co", DIR + "/act" })) {
    foreach (object ob in children(prog)) {
      if (ob) destruct(ob);
    }
  }
  foreach (string file in ({ "/ct.c", "/sh_target.c", "/sh_shadow.c",
                             "/co.c", "/act.c" })) {
    rm(DIR + file);
  }
}

private void run_checks() {
  object ob, sh, tp;

  ASSERT(mkdir(DIR) || file_size(DIR) == -2);

  write_src(DIR + "/ct.c",
            "string *msgs = ({ });\n" +
            "void catch_tell(string s) { msgs += ({ \"v1:\" + s }); }\n" +
            "string *q() { return msgs; }\n");
  ob = load_object(DIR + "/ct");
  tell_object(ob, "hello");
  write_src(DIR + "/ct.c",
            "string *msgs = ({ });\n" +
            "void catch_tell(string s) { msgs += ({ \"v2:\" + s }); }\n" +
            "string *q() { return msgs; }\n");
  ASSERT_EQ(1, recompile_object(ob));
  tell_object(ob, "again");
  ASSERT_EQ(({ "v1:hello", "v2:again" }), ob->q());

  write_src(DIR + "/sh_target.c",
            "string who() { return \"t1\"; }\n" +
            "string info() { return \"i1\"; }\n");
  write_src(DIR + "/sh_shadow.c",
            "string who() { return \"shadowed\"; }\n" +
            "int attach(object t) { return shadow(t, 1) != 0; }\n");
  ob = load_object(DIR + "/sh_target");
  sh = load_object(DIR + "/sh_shadow");
  ASSERT(sh->attach(ob));
  ASSERT_EQ("shadowed", ob->who());
  ASSERT_EQ("i1", ob->info());
  write_src(DIR + "/sh_target.c",
            "string who() { return \"t2\"; }\n" +
            "string info() { return \"i2 bigger\"; }\n");
  ASSERT_EQ(1, recompile_object(ob));
  ASSERT(query_shadowing(sh) == ob);
  ASSERT_EQ("shadowed", ob->who());
  ASSERT_EQ("i2 bigger", ob->info());

  write_src(DIR + "/sh_shadow.c",
            "string who() { return \"shadowed v2\"; }\n" +
            "int attach(object t) { return shadow(t, 1) != 0; }\n");
  ASSERT_EQ(1, recompile_object(sh));
  ASSERT(query_shadowing(sh) == ob);
  ASSERT_EQ("shadowed v2", ob->who());
  destruct(sh);
  ASSERT_EQ("t2", ob->who());

  write_src(DIR + "/co.c",
            "int h1, h2;\n" +
            "void late_tick() { }\n" +
            "void fpmark() { }\n" +
            "void arm() { h1 = call_out(\"late_tick\", 100); h2 = call_out((: fpmark :), 100); }\n" +
            "int pending() { return find_call_out(\"late_tick\"); }\n" +
            "void clear() { remove_call_out(h1); remove_call_out(h2); }\n");
  ob = load_object(DIR + "/co");
  ob->arm();
  write_src(DIR + "/co.c",
            "int h1, h2;\n" +
            "void late_tick() { }\n" +
            "void fpmark() { }\n" +
            "void arm() { h1 = call_out(\"late_tick\", 100); h2 = call_out((: fpmark :), 100); }\n" +
            "int pending() { return find_call_out(\"late_tick\"); }\n" +
            "void clear() { remove_call_out(h1); remove_call_out(h2); }\n");
  ASSERT_EQ(1, recompile_object(ob));
  ASSERT(ob->pending() >= 0);
  ob->clear();

#ifndef __NO_ADD_ACTION__
  write_src(DIR + "/act.c",
            "int hits;\n" +
            "void arm() { enable_commands(); add_action(\"do_x\", \"xyzzy\");\n" +
            "             set_heart_beat(1); }\n" +
            "int do_x(string s) { hits += 1; return 1; }\n" +
            "int try() { return command(\"xyzzy\"); }\n" +
            "int q() { return hits; }\n" +
            "void heart_beat() { }\n");
  ob = load_object(DIR + "/act");
  SAVETP;
  ob->arm();
  RESTORETP;
  ASSERT(ob->try());
  ASSERT_EQ(1, ob->q());
  ASSERT_EQ(1, query_heart_beat(ob));
  write_src(DIR + "/act.c",
            "int hits;\n" +
            "void arm() { enable_commands(); add_action(\"do_x\", \"xyzzy\");\n" +
            "             set_heart_beat(1); }\n" +
            "int do_x(string s) { hits += 10; return 1; }\n" +
            "int try() { return command(\"xyzzy\"); }\n" +
            "int q() { return hits; }\n" +
            "void heart_beat() { }\n");
  ASSERT_EQ(1, recompile_object(ob));
  ASSERT_EQ(1, query_heart_beat(ob));
  ASSERT(ob->try());
  ASSERT_EQ(11, ob->q());
#endif
}

void do_tests() {
  string err = catch(run_checks());

  cleanup();
  if (err) error(err);
}
