#if defined(__PACKAGE_ASYNC__) && !defined(__NO_ADD_ACTION__) && defined(__THIS_PLAYER_IN_CALL_OUT__)

private object expected;

void got_dir(mixed res) {
  ASSERT_EQ(expected, this_player());
}

void got_read(mixed res) {
  ASSERT_EQ(expected, this_player());
}

int atp_action() {
  expected = this_player();
  ASSERT_EQ(this_object(), expected);

  ASSERT_EQ(0, catch(async_getdir("/std/", (: got_dir :))));
  ASSERT_EQ(0, catch(async_read("/std/base64.c", (: got_read :))));
  return 1;
}

void setup() {
  enable_commands();
  add_action((: atp_action :), "atp");
  ASSERT(command("atp"));
  disable_commands();
}

void do_tests() {
  object peer;

  setup();
  peer = clone_object(__FILE__);
  peer->setup();
}

#else

void do_tests() {
  ASSERT(1);
}

#endif
