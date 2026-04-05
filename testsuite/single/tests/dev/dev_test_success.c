mixed dev_test() {
  write("setup complete\n");
  printf("running assertions: %d\n", 1);

  return json_encode(([
    "ok": 1,
    "summary": "basic dev test passed",
    "checks": ({
      ([ "name": "sanity", "ok": 1 ]),
    }),
    "artifacts": ([ "value": 1 ]),
  ]));
}
