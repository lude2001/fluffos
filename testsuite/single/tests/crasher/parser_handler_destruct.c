// A verb handler that destructs itself from its direct_* apply while returning
// success left we_are_finished() with best_match set but best_result->ob == 0.
// The parse must not crash or call do_the_call() with a null object.

string *parse_command_id_list() {
    return ({ "bag" });
}

void create() {
    parse_init();
    if (clonep()) {
        parse_add_rule("look", "WRD OBJ");
        move_object(__FILE__);
    }
}

int can_look_wrd_obj() {
    return 1;
}

int direct_look_wrd_obj() {
    destruct(this_object());
    return 1;
}

int do_look_wrd_obj() {
    return 0;
}

void do_tests() {
#ifndef __PACKAGE_PARSER__
    write("no package parser, skipped.\n");
#else
    clone_object(__FILE__);
    catch(parse_sentence("look in bag", 2, all_inventory()));
#endif
    ASSERT(1);
}
