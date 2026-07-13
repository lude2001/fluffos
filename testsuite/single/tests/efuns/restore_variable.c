#define ERR(i, x) ASSERT2(catch(restore_variable(x)), sprintf("Unexpected successful restore idx %d,\n %O\n", i, x))
#define IS(x, y) ASSERT_EQ(y, restore_variable(x))

string *value_errs = ({ "\"\"x", "\"\\", "\"\\x\\", "\"\\x\"x", "-x", "\"", "\"\\", "\"\r\"\r" });
mapping values = ([
    "\"\r\"" : "\n",
    "\"\\\"\"" : "\"",
    "\"\\\"\\x\r\"" : "\"x\n",
    // Special cases for encoding '\r'
    "-1" : -1,
    "0" : 0,
    "1" : 1,
    "22" : 22,
    "1.2" : 1.2,
    "333" : 333,
    "({})" : ({}),
    "({,})" : ({0}),
    "\"∮ E⋅da = Q,  n → ∞, ∑ f(i) = ���∏ g(i), ∀x∈ℝ: ⌈x⌉ = −⌊−x⌋, α ∧ ¬β = ¬(¬α ∨ �β)\"": "∮ E⋅da = Q,  n → ∞, ∑ f(i) = ���∏ g(i), ∀x∈ℝ: ⌈x⌉ = −⌊−x⌋, α ∧ ¬β = ¬(¬α ∨ �β)",
]);

void do_tests() {
    mixed x, y;
    int i = 0 ;
    for (i = 0; i < sizeof(value_errs); i++) {
	  ERR(i, value_errs[i]);
    }

    foreach (x, y in values) {
	IS(x, y);
    }

    // arrays
    foreach (x, y in values) {
	IS("({" + x + ",})", ({ y }));
	IS("({" + x + "," + x + ",})", ({ y, y }));
    }

    // mappings
    foreach (x, y in values) {
	IS("([1:" + x + ",])", ([1:y ]));
	IS("([" + x + ":" + x + ",])", ([ y: y ]));
    }

    {
	string wide_map = "";
	string wide_arr = "";
	mapping rm;
	mixed *ra;

	for (i = 0; i < 500; i++) {
	    wide_map += i + ":({" + i + ",}),";
	    wide_arr += "({" + i + ",}),";
	}

	rm = restore_variable("([" + wide_map + "])");
	ASSERT_EQ(500, sizeof(rm));
	ASSERT_EQ(7, rm[7][0]);

	ra = restore_variable("({" + wide_arr + "})");
	ASSERT_EQ(500, sizeof(ra));
	ASSERT_EQ(499, ra[499][0]);
    }

    {
	string deep_arr = "0";
	string deep_map = "0";

	for (i = 0; i < 400; i++) {
	    deep_arr = "({" + deep_arr + ",})";
	    deep_map = "([0:" + deep_map + ",])";
	}
	ASSERT2(catch(restore_variable(deep_arr)),
		"Deeply-nested array should be rejected, not crash");
	ASSERT2(catch(restore_variable(deep_map)),
		"Deeply-nested mapping should be rejected, not crash");
    }
}
