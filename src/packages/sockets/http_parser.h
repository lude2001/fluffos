#ifndef PACKAGES_SOCKETS_HTTP_PARSER_H
#define PACKAGES_SOCKETS_HTTP_PARSER_H

#include <string_view>

LPC_INT http_parser_create_handle();
mapping_t *http_parser_feed_handle(LPC_INT handle, std::string_view chunk);
void http_parser_close_handle(LPC_INT handle);

LPC_INT http_response_parser_create_handle();
mapping_t *http_response_parser_feed_handle(LPC_INT handle, std::string_view chunk);
void http_response_parser_close_handle(LPC_INT handle);

#endif
