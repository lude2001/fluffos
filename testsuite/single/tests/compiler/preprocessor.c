#define WARNING_LEVEL 1 /* block comment starts on the directive line
and ends on the next physical line */

#define COMMENT_TAIL 10 /* the tail after the comment still belongs
to the directive */ + 5

#define COMMENT_STRING "a/*not-comment*/b"
#define DOUBLE_NEG 1 -/* comment is whitespace */-1

#define CREDITS_KEY "credits" // trailing comment is not macro body
#define MIN2(a, b) ((a) < (b) ? (a) : (b))

#ifdef CREDITS_KEY // trailing comment is ignored for lookup
#define IFDEF_COMMENT_OK 1
#else
#define IFDEF_COMMENT_OK 0
#endif

#if 1 /* live conditional comment spans
the next physical line */ && 1
#define LIVE_IF_COMMENT_OK 1
#else
#define LIVE_IF_COMMENT_OK 0
#endif

#if 0 /* dead conditional comment spans
the next physical line */
this text must stay skipped
#else
#define DEAD_IF_COMMENT_OK 1
#endif

#define UNDEF_ME 42
#undef UNDEF_ME // trailing comment is ignored for lookup

void do_tests() {
  mapping values = ([ CREDITS_KEY: 7 ]);
  int UNDEF_ME = 9;

  ASSERT_EQ(1, WARNING_LEVEL);
  ASSERT_EQ(15, COMMENT_TAIL);
  ASSERT_EQ("a/*not-comment*/b", COMMENT_STRING);
  ASSERT_EQ(2, DOUBLE_NEG);
  ASSERT_EQ(7, values[CREDITS_KEY]);
  ASSERT_EQ(7, MIN2(10, values[CREDITS_KEY]));
  ASSERT_EQ(1, IFDEF_COMMENT_OK);
  ASSERT_EQ(1, LIVE_IF_COMMENT_OK);
  ASSERT_EQ(1, DEAD_IF_COMMENT_OK);
  ASSERT_EQ(9, UNDEF_ME);
}
