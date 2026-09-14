#include "base/package_api.h"

#include "packages/lude_http/http_efuns.h"
#include "packages/lude_http/http_parser.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <string>
#include <string_view>

namespace {

int hex_value(char ch) {
  if (ch >= '0' && ch <= '9') {
    return ch - '0';
  }
  if (ch >= 'A' && ch <= 'F') {
    return ch - 'A' + 10;
  }
  if (ch >= 'a' && ch <= 'f') {
    return ch - 'a' + 10;
  }
  return -1;
}

std::string lower_ascii(std::string_view input) {
  std::string lowered;

  lowered.reserve(input.size());
  for (unsigned char ch : input) {
    lowered.push_back(static_cast<char>(std::tolower(ch)));
  }
  return lowered;
}

const char *http_reason_phrase(int status) {
  switch (status) {
    case 200:
      return "OK";
    case 201:
      return "Created";
    case 204:
      return "No Content";
    case 301:
      return "Moved Permanently";
    case 302:
      return "Found";
    case 304:
      return "Not Modified";
    case 400:
      return "Bad Request";
    case 401:
      return "Unauthorized";
    case 403:
      return "Forbidden";
    case 404:
      return "Not Found";
    case 405:
      return "Method Not Allowed";
    case 409:
      return "Conflict";
    case 413:
      return "Payload Too Large";
    case 415:
      return "Unsupported Media Type";
    case 422:
      return "Unprocessable Entity";
    case 429:
      return "Too Many Requests";
    case 500:
      return "Internal Server Error";
    case 501:
      return "Not Implemented";
    case 502:
      return "Bad Gateway";
    case 503:
      return "Service Unavailable";
    default:
      return "Unknown";
  }
}

void add_decoded_pair(mapping_t *map, std::string_view raw_entry) {
  std::string key;
  std::string value;
  auto separator = raw_entry.find('=');

  if (separator == std::string_view::npos) {
    key = http_url_decode(raw_entry, true);
    value = "";
  } else {
    key = http_url_decode(raw_entry.substr(0, separator), true);
    value = http_url_decode(raw_entry.substr(separator + 1), true);
  }

  if (!key.empty()) {
    add_mapping_string(map, key.c_str(), value.c_str());
  }
}

std::string header_value_to_string(svalue_t *value) {
  switch (value->type) {
    case T_STRING:
      return value->u.string ? value->u.string : "";
    case T_NUMBER:
      return std::to_string(value->u.number);
    case T_REAL: {
      char buffer[64];
      snprintf(buffer, sizeof(buffer), "%g", value->u.real);
      return buffer;
    }
    default:
      return "";
  }
}

}  // namespace

std::string http_url_decode(std::string_view input, bool plus_as_space) {
  std::string decoded;

  decoded.reserve(input.size());
  for (size_t i = 0; i < input.size(); i++) {
    unsigned char ch = static_cast<unsigned char>(input[i]);

    if (ch == '+' && plus_as_space) {
      decoded.push_back(' ');
      continue;
    }

    if (ch == '%' && i + 2 < input.size()) {
      int hi = hex_value(input[i + 1]);
      int lo = hex_value(input[i + 2]);
      if (hi >= 0 && lo >= 0) {
        decoded.push_back(static_cast<char>((hi << 4) | lo));
        i += 2;
        continue;
      }
    }

    decoded.push_back(static_cast<char>(ch));
  }

  return decoded;
}

std::string http_url_encode(std::string_view input) {
  static constexpr char kHex[] = "0123456789ABCDEF";
  std::string encoded;

  encoded.reserve(input.size() * 3);
  for (unsigned char ch : input) {
    if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9') ||
        ch == '-' || ch == '_' || ch == '.' || ch == '~') {
      encoded.push_back(static_cast<char>(ch));
      continue;
    }

    encoded.push_back('%');
    encoded.push_back(kHex[(ch >> 4) & 0x0F]);
    encoded.push_back(kHex[ch & 0x0F]);
  }

  return encoded;
}

mapping_t *http_decode_kv_string(std::string_view input) {
  mapping_t *decoded = allocate_mapping(0);
  size_t start = 0;

  if (input.empty()) {
    return decoded;
  }

  while (start <= input.size()) {
    size_t end = input.find('&', start);
    auto token = input.substr(start, end == std::string_view::npos ? input.size() - start : end - start);

    if (!token.empty()) {
      add_decoded_pair(decoded, token);
    }

    if (end == std::string_view::npos) {
      break;
    }
    start = end + 1;
  }

  return decoded;
}

std::string http_build_response_string(int status, mapping_t *headers, std::string_view body) {
  std::string response;
  bool has_content_length = false;
  bool has_connection = false;
  array_t *keys = nullptr;
  array_t *values = nullptr;

  response.reserve(body.size() + 128);
  response += "HTTP/1.1 ";
  response += std::to_string(status);
  response += " ";
  response += http_reason_phrase(status);
  response += "\r\n";

  if (headers) {
    keys = mapping_indices(headers);
    values = mapping_values(headers);

    for (int i = 0; i < keys->size && i < values->size; i++) {
      auto *key = &keys->item[i];
      auto *value = &values->item[i];
      std::string header_name;
      std::string header_value;
      std::string lowered_name;

      if (key->type != T_STRING || !key->u.string) {
        continue;
      }

      header_name = key->u.string;
      header_value = header_value_to_string(value);
      lowered_name = lower_ascii(header_name);

      if (lowered_name == "content-length") {
        has_content_length = true;
      } else if (lowered_name == "connection") {
        has_connection = true;
      }

      response += header_name;
      response += ": ";
      response += header_value;
      response += "\r\n";
    }

    free_array(keys);
    free_array(values);
  }

  if (!has_content_length) {
    response += "Content-Length: ";
    response += std::to_string(body.size());
    response += "\r\n";
  }

  if (!has_connection) {
    response += "Connection: close\r\n";
  }

  response += "\r\n";
  response.append(body.data(), body.size());

  return response;
}

#ifdef F_URL_DECODE
void f_url_decode() {
  if (sp->type != T_STRING) {
    bad_arg(1, F_URL_DECODE);
  }

  auto decoded = http_url_decode(sp->u.string, true);
  free_string_svalue(sp);
  put_malloced_string(string_copy(decoded.c_str(), "f_url_decode"));
}
#endif

#ifdef F_URL_ENCODE
void f_url_encode() {
  if (sp->type != T_STRING) {
    bad_arg(1, F_URL_ENCODE);
  }

  auto encoded = http_url_encode(sp->u.string);
  free_string_svalue(sp);
  put_malloced_string(string_copy(encoded.c_str(), "f_url_encode"));
}
#endif

#ifdef F_HTTP_DECODE_QUERY
void f_http_decode_query() {
  if (sp->type != T_STRING) {
    bad_arg(1, F_HTTP_DECODE_QUERY);
  }

  auto *decoded = http_decode_kv_string(sp->u.string);
  free_string_svalue(sp);
  sp->type = T_MAPPING;
  sp->u.map = decoded;
}
#endif

#ifdef F_HTTP_DECODE_FORM
void f_http_decode_form() {
  if (sp->type != T_STRING) {
    bad_arg(1, F_HTTP_DECODE_FORM);
  }

  auto *decoded = http_decode_kv_string(sp->u.string);
  free_string_svalue(sp);
  sp->type = T_MAPPING;
  sp->u.map = decoded;
}
#endif

#ifdef F_HTTP_BUILD_RESPONSE
void f_http_build_response() {
  auto *arg = sp - 2;

  if (arg[0].type != T_NUMBER) {
    bad_arg(1, F_HTTP_BUILD_RESPONSE);
  }
  if (arg[1].type != T_MAPPING) {
    bad_arg(2, F_HTTP_BUILD_RESPONSE);
  }
  if (arg[2].type != T_STRING) {
    bad_arg(3, F_HTTP_BUILD_RESPONSE);
  }

  auto response = http_build_response_string(arg[0].u.number, arg[1].u.map, arg[2].u.string);
  free_string_svalue(sp);
  sp--;
  free_mapping(sp->u.map);
  sp--;
  put_malloced_string(string_copy(response.c_str(), "f_http_build_response"));
}
#endif

#ifdef F_HTTP_PARSER_CREATE
void f_http_parser_create() { push_number(http_parser_create_handle()); }
#endif

#ifdef F_HTTP_PARSER_FEED
void f_http_parser_feed() {
  auto *arg = sp - 1;
  mapping_t *result;

  if (arg[0].type != T_NUMBER) {
    bad_arg(1, F_HTTP_PARSER_FEED);
  }
  if (arg[1].type != T_STRING) {
    bad_arg(2, F_HTTP_PARSER_FEED);
  }

  result = http_parser_feed_handle(arg[0].u.number, arg[1].u.string);
  free_string_svalue(sp);
  sp--;
  sp->type = T_MAPPING;
  sp->subtype = 0;
  sp->u.map = result;
}
#endif

#ifdef F_HTTP_PARSER_CLOSE
void f_http_parser_close() {
  if (sp->type != T_NUMBER) {
    bad_arg(1, F_HTTP_PARSER_CLOSE);
  }

  http_parser_close_handle(sp->u.number);
  pop_stack();
}
#endif

#ifdef F_HTTP_RESPONSE_PARSER_CREATE
void f_http_response_parser_create() { push_number(http_response_parser_create_handle()); }
#endif

#ifdef F_HTTP_RESPONSE_PARSER_FEED
void f_http_response_parser_feed() {
  auto *arg = sp - 1;
  mapping_t *result;

  if (arg[0].type != T_NUMBER) {
    bad_arg(1, F_HTTP_RESPONSE_PARSER_FEED);
  }
  if (arg[1].type != T_STRING) {
    bad_arg(2, F_HTTP_RESPONSE_PARSER_FEED);
  }

  result = http_response_parser_feed_handle(arg[0].u.number, arg[1].u.string);
  free_string_svalue(sp);
  sp--;
  sp->type = T_MAPPING;
  sp->subtype = 0;
  sp->u.map = result;
}
#endif

#ifdef F_HTTP_RESPONSE_PARSER_CLOSE
void f_http_response_parser_close() {
  if (sp->type != T_NUMBER) {
    bad_arg(1, F_HTTP_RESPONSE_PARSER_CLOSE);
  }

  http_response_parser_close_handle(sp->u.number);
  pop_stack();
}
#endif
