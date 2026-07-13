string *parse_command_id_list() {
  return ({ "bag" });
}

void create() {
  parse_init();

  if (clonep()) {
    move_object(__FILE__);
  }
}

can_look_wrd_obj() {
  return 1;
}

direct_look_wrd_obj() {
  error("direct apply throws");
}

do_look_wrd_obj() {
  return 0;
}

can_poke_obj() {
  error("can apply throws");
}

direct_poke_obj() {
  return 1;
}

do_poke_obj() {
  error("do apply throws");
}

void do_tests() {
#ifndef __PACKAGE_PARSER__
  write("no package parser, skipped.\n");
#else
  mixed err;

  clone_object(__FILE__);
  parse_add_rule("look", "WRD OBJ");
  parse_add_rule("poke", "OBJ");

  err = catch(parse_sentence("look in bag", 2, all_inventory()));
  err = catch(parse_sentence("poke bag", 2, all_inventory()));

  ASSERT_EQ(1, intp(parse_sentence("look in bag", 0, all_inventory())));
#endif
  ASSERT(1);
}
