// living_parse() (parse_command %l) used to dereference oblist elements as
// objects without checking their type. Non-objects in the caller-provided list
// must be skipped instead of crashing.

void do_tests() {
#ifdef __PACKAGE_OPS__
    object target;

    ASSERT_EQ(0, catch(parse_command("get all", ({ 0 }), "'get' %l", target)));
    ASSERT_EQ(0, catch(parse_command("get all", ({ 0, this_object() }), "'get' %l", target)));
#endif
    ASSERT(1);
}
