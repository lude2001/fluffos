#ifndef PACKAGES_SOCKETS_HTTP_EFUNS_H_
#define PACKAGES_SOCKETS_HTTP_EFUNS_H_

#include <string>
#include <string_view>

struct mapping_t;

std::string http_url_decode(std::string_view input, bool plus_as_space);
std::string http_url_encode(std::string_view input);
mapping_t *http_decode_kv_string(std::string_view input);
std::string http_build_response_string(int status, mapping_t *headers, std::string_view body);

#endif /* PACKAGES_SOCKETS_HTTP_EFUNS_H_ */
