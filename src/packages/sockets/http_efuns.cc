#include "base/package_api.h"

#include "packages/sockets/http_efuns.h"

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
