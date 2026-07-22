private mapping *values;

void do_tests() {
    mapping summary;
    string value = "memory-summary-immortal-string";

    values = allocate(5);
    for (int bucket = 0; bucket < sizeof(values); bucket++) {
        values[bucket] = ([]);
        for (int i = 0; i < 15000; i++) {
            values[bucket][i] = value;
        }
    }

    summary = memory_summary();
    ASSERT(mapp(summary));

    values = 0;
}
