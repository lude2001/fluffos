#define DIR "/data/hot"
#define DAEMON "/single/hot_reload"

private void write_src(string file, string body) {
  ASSERT2(write_file(file, body, 1), "write " + file);
}

private void cleanup(object d) {
  foreach (string prog in ({ DIR + "/widget", DIR + "/base", DIR + "/counter",
                             DIR + "/coop", DIR + "/plain", DIR + "/gadget",
                             DIR + "/kid", DIR + "/mid", DIR + "/grand",
                             DIR + "/loop" })) {
    foreach (object ob in children(prog)) {
      if (ob) destruct(ob);
    }
  }
  if (d) {
    d->disable();
    destruct(d);
  }
  foreach (string f in ({ "/color.h", "/base.c", "/widget.c",
                          "/counter.c", "/coop.c", "/plain.c",
                          "/gadget.c", "/kid.c", "/mid.c",
                          "/grand.c", "/loop.h", "/loop.c" })) {
    rm(DIR + f);
  }
  rmdir(DIR);
}

private void run_checks() {
  object d, widget, ob, g1, g2, p1;

  ASSERT(mkdir(DIR) || file_size(DIR) == -2);

  write_src(DIR + "/color.h", "#define COLOR \"red\"\n");
  write_src(DIR + "/base.c",
            "#include \"" + DIR + "/color.h\"\n" +
            "string color() { return COLOR; }\n");
  write_src(DIR + "/widget.c",
            "inherit \"" + DIR + "/base\";\n" +
            "string describe() { return \"a \" + color() + \" widget\"; }\n");

  d = load_object(DAEMON);
  d->enable(1);
  ASSERT(d->query_polling());

  ASSERT(d->watch(DIR + "/widget"));
  widget = find_object(DIR + "/widget");
  ASSERT(objectp(widget));
  ASSERT_EQ("a red widget", widget->describe());

  ASSERT_EQ(0, d->check_now());
  ASSERT(find_object(DIR + "/widget") == widget);

  write_src(DIR + "/color.h", "#define COLOR \"green\"\n");
  ASSERT_EQ(1, d->check_now());
  widget = find_object(DIR + "/widget");
  ASSERT(objectp(widget));
  ASSERT_EQ("a green widget", widget->describe());
  ASSERT_EQ(1, d->query_reload_count());

  write_src(DIR + "/widget.c",
            "inherit \"" + DIR + "/base\";\n" +
            "string describe() { return \"one very \" + color() + \" widget\"; }\n");
  ASSERT_EQ(1, d->check_now());
  ASSERT_EQ("one very green widget", (DIR + "/widget")->describe());
  ASSERT_EQ(2, d->query_reload_count());

  ASSERT_EQ(0, d->check_now());

  ASSERT(d->watch(DIR + "/base"));
  write_src(DIR + "/color.h", "#define COLOR \"blue\"\n");
  ASSERT_EQ(2, d->check_now());
  ASSERT_EQ("one very blue widget", (DIR + "/widget")->describe());
  ASSERT_EQ("blue", (DIR + "/base")->color());
  ASSERT_EQ(0, d->check_now());

  write_src(DIR + "/base.c", "this does not compile\n");
  ASSERT_EQ(0, d->check_now());
  ASSERT(d->query_polling());
  write_src(DIR + "/base.c",
            "#include \"" + DIR + "/color.h\"\n" +
            "string color() { return \"fixed \" + COLOR; }\n");
  ASSERT_EQ(2, d->check_now());
  ASSERT_EQ("fixed blue", (DIR + "/base")->color());
  ASSERT_EQ("one very fixed blue widget", (DIR + "/widget")->describe());

  write_src(DIR + "/grand.c", "string g() { return \"g1\"; }\n");
  write_src(DIR + "/mid.c",
            "inherit \"" + DIR + "/grand\";\n" +
            "string m() { return \"m1/\" + g(); }\n");
  write_src(DIR + "/kid.c",
            "inherit \"" + DIR + "/mid\";\n" +
            "string k() { return m(); }\n");
  ASSERT(d->watch(DIR + "/kid"));
  ASSERT_EQ("m1/g1", (DIR + "/kid")->k());
  write_src(DIR + "/grand.c", "string g() { return \"g2 new\"; }\n");
  write_src(DIR + "/mid.c",
            "inherit \"" + DIR + "/grand\";\n" +
            "string m() { return \"m2 longer/\" + g(); }\n");
  ASSERT_EQ(1, d->check_now());
  ASSERT_EQ("m2 longer/g2 new", (DIR + "/kid")->k());

  write_src(DIR + "/loop.h", "#define LOOPV \"one\"\n");
  write_src(DIR + "/loop.c",
            "#include \"" + DIR + "/loop.h\"\n" +
            "string v() { return LOOPV; }\n" +
            "int poke(object dmn) { return dmn->check_now(); }\n");
  ASSERT(d->watch(DIR + "/loop"));
  ASSERT_EQ("one", (DIR + "/loop")->v());
  write_src(DIR + "/loop.h", "#define LOOPV \"two three\"\n");
  ASSERT_EQ(0, (DIR + "/loop")->poke(d));
  ASSERT_EQ(1, d->check_now());
  ASSERT_EQ("two three", (DIR + "/loop")->v());

  write_src(DIR + "/counter.c",
            "int hits;\n" +
            "int bump() { return ++hits; }\n" +
            "string version() { return \"v1\"; }\n");
  ASSERT(d->watch(DIR + "/counter"));
  ob = find_object(DIR + "/counter");
  ob->bump();
  ob->bump();
  ASSERT_EQ(3, ob->bump());
  write_src(DIR + "/counter.c",
            "int hits;\n" +
            "int extra = 11;\n" +
            "int bump() { return ++hits; }\n" +
            "int query_extra() { return extra; }\n" +
            "string version() { return \"v2\"; }\n");
  ASSERT_EQ(1, d->check_now());
  ob = find_object(DIR + "/counter");
  ASSERT(objectp(ob));
  ASSERT_EQ("v2", ob->version());
  ASSERT_EQ(4, ob->bump());
  ASSERT_EQ(11, ob->query_extra());

  write_src(DIR + "/gadget.c",
            "int uses;\n" +
            "int use() { return ++uses; }\n" +
            "string label() { return \"mk1\"; }\n");
  ASSERT(d->watch(DIR + "/gadget"));
  ob = find_object(DIR + "/gadget");
  g1 = new(DIR + "/gadget");
  g2 = new(DIR + "/gadget");
  g1->use();
  g2->use();
  g2->use();
  ASSERT_EQ(3, sizeof(children(DIR + "/gadget")));
  ASSERT_EQ(2, sizeof(filter(children(DIR + "/gadget"), (: clonep($1) :))));

  write_src(DIR + "/gadget.c",
            "int uses;\n" +
            "int use() { return ++uses; }\n" +
            "string label() { return \"mk2 improved\"; }\n");
  ASSERT_EQ(1, d->check_now());
  ASSERT(find_object(DIR + "/gadget") == ob);
  ASSERT(objectp(g1) && objectp(g2));
  ASSERT_EQ("mk2 improved", ob->label());
  ASSERT_EQ("mk2 improved", g1->label());
  ASSERT_EQ("mk2 improved", g2->label());
  ASSERT_EQ(1, ob->use());
  ASSERT_EQ(2, g1->use());
  ASSERT_EQ(3, g2->use());

  write_src(DIR + "/coop.c",
            "private int secret = 5;\n" +
            "int peek() { return secret; }\n" +
            "void poke(int v) { secret = v; }\n" +
            "string tag() { return \"first\"; }\n" +
            "mixed hot_reload_state() { return secret; }\n" +
            "void hot_reload_restore(mixed s) { secret = s; }\n");
  ASSERT(d->watch(DIR + "/coop"));
  ob = find_object(DIR + "/coop");
  ob->poke(42);
  write_src(DIR + "/coop.c",
            "private int secret = 5;\n" +
            "int peek() { return secret; }\n" +
            "void poke(int v) { secret = v; }\n" +
            "string tag() { return \"second one\"; }\n" +
            "mixed hot_reload_state() { return secret; }\n" +
            "void hot_reload_restore(mixed s) { secret = s; }\n");
  ASSERT_EQ(1, d->check_now());
  ob = find_object(DIR + "/coop");
  ASSERT_EQ("second one", ob->tag());
  ASSERT_EQ(42, ob->peek());

  write_src(DIR + "/plain.c",
            "int n = 9;\n" +
            "int get() { return n; }\n" +
            "int set(int v) { return n = v; }\n" +
            "string tag() { return \"aaa\"; }\n");
  ASSERT(d->watch(DIR + "/plain", 0));
  ob = find_object(DIR + "/plain");
  ob->set(100);
  p1 = new(DIR + "/plain");
  p1->set(55);
  write_src(DIR + "/plain.c",
            "int n = 9;\n" +
            "int get() { return n; }\n" +
            "int set(int v) { return n = v; }\n" +
            "string tag() { return \"b\"; }\n");
  ASSERT_EQ(1, d->check_now());
  ob = find_object(DIR + "/plain");
  ASSERT_EQ("b", ob->tag());
  ASSERT_EQ(9, ob->get());
  ASSERT_EQ("aaa", p1->tag());
  ASSERT_EQ(55, p1->get());
  foreach (object straggler in filter(children(DIR + "/plain"), (: clonep($1) :))) {
    destruct(straggler);
  }
  ASSERT(!objectp(p1));
  ASSERT_EQ(1, sizeof(children(DIR + "/plain")));

  d->disable();
  ASSERT(!d->query_polling());
  ASSERT(!"/single/master"->query_compile_hooks());
}

void do_tests() {
  string err = catch(run_checks());

  cleanup(find_object(DAEMON));
  "/single/master"->set_compile_hooks(0);
  if (err) error(err);
}
