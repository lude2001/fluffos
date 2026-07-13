#define RC_CALL_OTHER_TYPE_CHECK 287

int foo() { return 1; }

int same(mixed *x, mixed *y) {
    for (int i = 0; i < sizeof(x); i++) {
	return x[i] == y[i];
    }
}

int bar(int x) {
    return x;
}

int typed_arg(int x) {
    return x;
}

void do_tests() {
    int old_call_other_type_check = get_config(RC_CALL_OTHER_TYPE_CHECK);

    ASSERT(file_name()->foo());
    ASSERT(this_object()->foo());
    ASSERT(same((mixed *)({ file_name(), this_object() })->foo(), ({ 1, 1 })));

    ASSERT(catch(call_other(this_object(), 0)));
    ASSERT(call_other(this_object(), "foo"));
    ASSERT(call_other(this_object(), ({ "foo", 1 })));
    ASSERT(catch(call_other("foadf", "foo")));
    
    ASSERT(undefinedp(this_object()->bazz()));

    set_config(RC_CALL_OTHER_TYPE_CHECK, 1);
    ASSERT(catch(call_other(this_object(), "typed_arg", 1, "extra")));
    set_config(RC_CALL_OTHER_TYPE_CHECK, old_call_other_type_check);

    destruct(this_object());
    ASSERT(undefinedp("/single/master"->valid_bind()));
}
