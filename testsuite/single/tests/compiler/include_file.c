#define MASTER "/single/master"
#define HOOK "/clone/compile_hook"

private void run_checks() {
  object hook, ob;
  mixed *calls;

  hook = load_object(HOOK);
  hook->reset_rules();
  MASTER->set_compile_hooks(hook);

  hook->set_include_rule("if_redirect.h", "/clone/if_real.h");
  ob = load_object("/clone/if_redirect");
  ASSERT_EQ(5, ob->value());
  calls = hook->query_include_calls();
  ASSERT_EQ("/clone/if_redirect.c", calls[<1][0]);
  ASSERT_EQ("/clone/if_redirect.c", calls[<1][1]);
  ASSERT_EQ("if_redirect.h", calls[<1][2]);

  hook->set_include_rule("if_inline.h", ({ "#define INLINE_VALUE 9" }));
  ob = load_object("/clone/if_inline");
  ASSERT_EQ(9, ob->value());

  ob = load_object("/clone/if_nested");
  ASSERT_EQ(7, ob->value());
  calls = hook->query_include_calls();
  ASSERT(sizeof(filter(calls, (: $1[1] == "/clone/if_outer.h" &&
                                $1[2] == "if_real.h" :))));

  hook->set_include_rule("if_denied.h", 0);
  ASSERT(catch(load_object("/clone/if_denied")));
}

void do_tests() {
  string err = catch(run_checks());

  MASTER->set_compile_hooks(0);
  if (err) error(err);
}
