mixed dev_test() {
  write("before failure\n");
  error("dev_test exploded\n");
  return "";
}
