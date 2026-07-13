#define MASTER "/single/master"
#define HOOK "/clone/compile_hook"

private void run_checks() {
  object hook, ob;
  mixed *calls;

  hook = load_object(HOOK);
  hook->reset_rules();
  MASTER->set_compile_hooks(hook);

  hook->set_inherit_rule("/clone/ip_redirect", "/clone/ip_real");
  ob = load_object("/clone/ip_child_redirect");
  ASSERT_EQ(11, ob->value());
  calls = hook->query_inherit_calls();
  ASSERT_EQ("/clone/ip_child_redirect.c", calls[<1][0]);
  ASSERT_EQ("/clone/ip_redirect", calls[<1][1]);
  ASSERT_EQ(0, calls[<1][2]);

  hook->set_inherit_rule("/clone/ip_inline",
                         ({ "int inline_value() { return 23; }" }));
  ob = load_object("/clone/ip_child_inline");
  ASSERT_EQ(23, ob->value());

  hook->set_inherit_rule("/clone/ip_denied", 0);
  ASSERT(catch(load_object("/clone/ip_child_denied")));

  ob = load_object("/clone/ip_child_priv");
  ASSERT_EQ(1, ob->value());
  calls = hook->query_inherit_calls();
  ASSERT_EQ("/clone/ip_child_priv.c", calls[<1][0]);
  ASSERT_EQ("/clone/ip_real", calls[<1][1]);
  ASSERT_EQ(1, calls[<1][2]);

  ASSERT(!find_object("/clone/ip_real2"));
  hook->set_inherit_rule("/clone/ip_synth2",
                         ({ "inherit \"/clone/ip_real2\";",
                            "int synth2() { return real2() * 2; }" }));
  ob = load_object("/clone/ip_child_inline2");
  ASSERT_EQ(21, ob->child_fn2());
  ASSERT(objectp(find_object("/clone/ip_real2")));

  hook->set_inherit_rule("/clone/ip_synth_outer",
                         ({ "inherit \"/clone/ip_synth_inner\";",
                            "int outer() { return inner() + 1; }" }));
  hook->set_inherit_rule("/clone/ip_synth_inner",
                         ({ "int inner() { return 40; }" }));
  ob = load_object("/clone/ip_child_nested");
  ASSERT_EQ(42, ob->total());
  ASSERT(objectp(find_object("/clone/ip_synth_inner")));
}

void do_tests() {
  string err = catch(run_checks());

  MASTER->set_compile_hooks(0);
  if (err) error(err);
}
