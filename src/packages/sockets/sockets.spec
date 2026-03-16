
/*
 * socket efuns
 */
    int socket_create(int, string | function, string | function | void);
    int socket_bind(int, int, string | void);
    int socket_listen(int, string | function);
    int socket_accept(int, string | function, string | function);
    int socket_connect(int, string, string | function, string | function);
    int socket_write(int, mixed, string | void);
    int socket_close(int);
    int socket_release(int, object, string | function);
    int socket_acquire(int, string | function, string | function, string | function);
    string socket_error(int);
    string socket_address(int | object, int default: 0);
    void socket_set_option(int, int, mixed);
    mixed socket_get_option(int, int);
    string url_decode(string);
    string url_encode(string);
    mapping http_decode_query(string);
    mapping http_decode_form(string);
    string http_build_response(int, mapping, string);
    mixed http_parser_create();
    mapping http_parser_feed(mixed, string);
    void http_parser_close(mixed);
    /*
     * ret[0] = (int) fd
     * ret[1] = (string) state
     * ret[2] = (string) mode
     * ret[3] = (string) local address
     * ret[4] = (string) remote address
     * ret[5] = (object) owner
     */
    mixed *socket_status(void | int);
