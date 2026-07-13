#ifdef __PACKAGE_SOCKETS__
#define STREAM 1

void rcb(int fd) { }
void wcb(int fd) { }
#endif

void do_tests() {
#ifdef __PACKAGE_SOCKETS__
    int fd = socket_create(STREAM, "rcb", "wcb");

    if (fd >= 0) {
        string longhost = sprintf("%'a'2000s", "") + " 80";

        ASSERT(socket_connect(fd, longhost, "rcb", "wcb") < 0);
        socket_close(fd);
    }
#endif
    ASSERT(1);
}
