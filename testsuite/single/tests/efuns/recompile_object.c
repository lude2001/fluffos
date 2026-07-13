inherit "/inherit/tests";

#define DIR "/data/recompile_object"
#define THING DIR + "/thing"

private void write_src(string body) {
  ASSERT2(write_file(THING + ".c", body, 1), "write " + THING);
}

private void cleanup() {
  foreach (object ob in children(THING)) {
    if (ob) destruct(ob);
  }
  rm(THING + ".c");
  rmdir(DIR);
}

private void run_checks() {
  object master, clone;
  function fp;
  string err;

  mkdir(DIR);
  write_src("int kept = 1;\n" +
            "private int secret = 2;\n" +
            "string version() { return \"v1\"; }\n" +
            "int bump() { return ++kept; }\n" +
            "int kept_value() { return kept; }\n" +
            "int secret_value() { return secret; }\n" +
            "function stale_fp() { return (: bump :); }\n");

  master = load_object(THING);
  clone = new(THING);
  ASSERT_EQ("v1", master->version());
  ASSERT_EQ("v1", clone->version());
  ASSERT_EQ(2, master->bump());
  ASSERT_EQ(2, clone->bump());
  fp = master->stale_fp();

  write_src("int kept = 100;\n" +
            "private int secret = 200;\n" +
            "int added = 300;\n" +
            "string version() { return \"v2\"; }\n" +
            "int bump() { return kept += 10; }\n" +
            "int kept_value() { return kept; }\n" +
            "int secret_value() { return secret; }\n" +
            "int added_value() { return added; }\n" +
            "function stale_fp() { return (: bump :); }\n");

  ASSERT_EQ(2, recompile_object(master));
  ASSERT_EQ("v2", master->version());
  ASSERT_EQ("v2", clone->version());
  ASSERT_EQ(2, master->kept_value());
  ASSERT_EQ(2, clone->kept_value());
  ASSERT_EQ(2, master->secret_value());
  ASSERT_EQ(2, clone->secret_value());
  ASSERT_EQ(300, master->added_value());
  ASSERT_EQ(300, clone->added_value());

  err = catch(evaluate(fp));
  ASSERT2(stringp(err) && strsrch(err, "was recompiled") != -1,
          "stale function pointer should fail cleanly: " + sprintf("%O", err));

  err = catch(recompile_object(clone));
  ASSERT2(stringp(err) && strsrch(err, "master copy") != -1,
          "clone recompile should be rejected: " + sprintf("%O", err));
}

void do_tests() {
  string err = catch(run_checks());

  cleanup();
  if (err) error(err);
}
