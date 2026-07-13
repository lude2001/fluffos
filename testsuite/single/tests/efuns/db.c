#ifdef __PACKAGE_DB__
int calledDB;
#endif

int conn = 0;

#if defined(__PACKAGE_DB__) && defined(__USE_MYSQL__)
void test_mysql_binary_row_lengths() {
    string host = get_os_env("FT_MYSQL_HOST");
    string db = get_os_env("FT_MYSQL_DB");
    string user = get_os_env("FT_MYSQL_USER");
    mixed res;

    if (undefinedp(host) || undefinedp(db) || undefinedp(user)) {
        write("MySQL test env is not configured, skipping MySQL binary row-length test...\n");
        return;
    }

    conn = db_connect(host, db, user, __USE_MYSQL__);
    ASSERT_NE(0, conn);

    db_exec(conn, "DROP TABLE IF EXISTS fluffos_binary_regression");
    ASSERT_EQ(0, db_exec(conn,
        "CREATE TABLE fluffos_binary_regression (id INT PRIMARY KEY, payload VARBINARY(16))"));
    ASSERT_EQ(0, db_exec(conn,
        "INSERT INTO fluffos_binary_regression VALUES (1, X'00'), (2, X'000102')"));
    ASSERT_EQ(2, db_exec(conn,
        "SELECT payload FROM fluffos_binary_regression ORDER BY id"));

    res = db_fetch(conn, 1);
    ASSERT_EQ(1, sizeof(res));
    ASSERT_EQ(1, sizeof(res[0]));
    ASSERT_EQ(0, res[0][0]);

    res = db_fetch(conn, 2);
    ASSERT_EQ(1, sizeof(res));
    ASSERT_EQ(3, sizeof(res[0]));
    ASSERT_EQ(0, res[0][0]);
    ASSERT_EQ(1, res[0][1]);
    ASSERT_EQ(2, res[0][2]);

    ASSERT_EQ(0, db_exec(conn, "DROP TABLE fluffos_binary_regression"));
    db_close(conn);
}
#endif

void do_tests() {
#ifndef __PACKAGE_DB__
    write("PACKAGE_DB is not enabled, skipping DB tests...\n");
    return;
#else
#ifdef __USE_SQLITE3__
    // test db functions through sqlite
    int rows = 0;
    mixed res;

    // Open & Close
    conn = db_connect("", "/test.sqlite", "", __USE_SQLITE3__);
    ASSERT_NE(0, conn);
    db_close(conn);

    // Full tests
    conn = db_connect("", "/test.sqlite", "", __USE_SQLITE3__);
    ASSERT_NE(0, conn);

    ASSERT_NE("", db_status());
    write("db_status: " + db_status() + "\n");

    // https://sqlite.org/cli.html
    rows = db_exec(conn, "DROP TABLE IF EXISTS tbl1");
    rows = db_exec(conn, "create table IF NOT EXISTS tbl1(one varchar(10), two bigint);");
    ASSERT_EQ(0, rows);
    rows = db_exec(conn, "insert into tbl1 values('hello!',10);");
    ASSERT_EQ(0, rows);
    rows = db_exec(conn, "insert into tbl1 values('goodbye', 20);");
    ASSERT_EQ(0, rows);
    rows = db_exec(conn, "insert into tbl1 values('largeint', 9223372036854775807);");
    ASSERT_EQ(0, rows);
    rows = db_exec(conn, "select * from tbl1;");
    ASSERT_EQ(3, rows);

    res = db_fetch(conn, 1); // index start at 1
    ASSERT_EQ(({ "hello!", 10 }), res);

    res = db_fetch(conn, 2);
    ASSERT_EQ(({ "goodbye", 20 }), res);

    res = db_fetch(conn, 3);
    ASSERT_EQ(({ "largeint", MAX_INT }), res);

    rows = db_exec(conn, "drop table tbl1;");
    ASSERT_EQ(0, rows);

    rows = db_exec(conn, "create table IF NOT EXISTS tbl2(one varchar(10) UNIQUE);");
    ASSERT_EQ(0, rows);
    rows = db_exec(conn, "insert into tbl2 values('dup');");
    ASSERT_EQ(0, rows);
    db_exec(conn, "insert into tbl2 values('dup');");
    rows = db_exec(conn, "select * from tbl2;");
    ASSERT_EQ(1, rows);
    ASSERT_EQ(({ "dup" }), db_fetch(conn, 1));
    rows = db_exec(conn, "drop table tbl2;");
    ASSERT_EQ(0, rows);

    db_close(conn);

#ifdef __PACKAGE_ASYNC__
    conn = db_connect("", "/test.sqlite", "", __USE_SQLITE3__);
    ASSERT_NE(0, conn);
    db_exec(conn, "DROP TABLE IF EXISTS tbl1");
    db_exec(conn, "create table IF NOT EXISTS tbl1(one varchar(10), two smallint);");
    async_db_exec(conn, "insert into tbl1 values('hello!',10);", function(int rows) {
        mixed res;

        calledDB = 1;

        write("ASYNC: async_db_exec callback: " + conn + " matched " + rows + " rows\n");

        rows = db_exec(conn, "select * from tbl1;");
        ASSERT_EQ(1, rows);

        res = db_fetch(conn, 1); // index starts at 1
        ASSERT_EQ(({ "hello!", 10 }), res);

        db_close(conn);
    });

    call_out(function() {
        ASSERT_EQ(1, calledDB);
    }, 1);
#else
    write("PACKAGE_ASYNC is not enabled, skipping async db tests...\n");
#endif // __PACKAGE_ASYNC__
#else
    write("USE_SQLITE3 is not enabled, skipping SQLite tests...\n");
#endif // __USE_SQLITE3__

#ifdef __USE_MYSQL__
    test_mysql_binary_row_lengths();
#else
    write("USE_MYSQL is not enabled, skipping MySQL binary row-length test...\n");
#endif // __USE_MYSQL__
#endif // __PACKAGE_DB__
}
