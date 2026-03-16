#include "base/package_api.h"

#include "packages/sockets/http_efuns.h"
#include "packages/sockets/http_parser.h"

#include <cctype>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

namespace {

struct HttpParserState {
  bool in_use = false;
  std::string buffer;
};

std::vector<HttpParserState> g_http_parsers;

std::string lower_ascii(std::string_view input) {
  std::string lowered;

  lowered.reserve(input.size());
  for (unsigned char ch : input) {
    lowered.push_back(static_cast<char>(std::tolower(ch)));
  }
  return lowered;
}

std::string trim_ascii(std::string_view input) {
  size_t start = 0;
  size_t end = input.size();

  while (start < end && std::isspace(static_cast<unsigned char>(input[start]))) {
    start++;
  }
  while (end > start && std::isspace(static_cast<unsigned char>(input[end - 1]))) {
    end--;
  }

  return std::string(input.substr(start, end - start));
}

svalue_t *insert_mapping_slot(mapping_t *map, const char *key) {
  svalue_t lv;
  svalue_t *slot;

  lv.type = T_STRING;
  lv.subtype = STRING_CONSTANT;
  lv.u.string = const_cast<char *>(key);

  slot = find_for_insert(map, &lv, 1);
  free_string(lv.u.string);
  return slot;
}

void add_mapping_mapping(mapping_t *map, const char *key, mapping_t *value) {
  auto *slot = insert_mapping_slot(map, key);

  slot->type = T_MAPPING;
  slot->subtype = 0;
  slot->u.map = value;
  value->ref++;
}

mapping_t *make_error(int status, const char *reason, const char *message) {
  mapping_t *error = allocate_mapping(3);

  add_mapping_pair(error, "status", status);
  add_mapping_string(error, "reason", reason);
  add_mapping_string(error, "message", message);
  return error;
}

bool parse_request_line(std::string_view line, std::string *method, std::string *path,
                        std::string *version, std::string *query_string) {
  size_t first_space = line.find(' ');
  size_t second_space;
  std::string_view target;
  std::string_view version_view;
  size_t query_pos;

  if (first_space == std::string_view::npos) {
    return false;
  }

  second_space = line.find(' ', first_space + 1);
  if (second_space == std::string_view::npos) {
    return false;
  }

  *method = std::string(line.substr(0, first_space));
  target = line.substr(first_space + 1, second_space - first_space - 1);
  version_view = line.substr(second_space + 1);

  if (method->empty() || target.empty() || version_view.size() <= 5 ||
      version_view.substr(0, 5) != "HTTP/") {
    return false;
  }

  *version = std::string(version_view.substr(5));
  query_pos = target.find('?');
  if (query_pos == std::string_view::npos) {
    *path = std::string(target);
    query_string->clear();
  } else {
    *path = std::string(target.substr(0, query_pos));
    *query_string = std::string(target.substr(query_pos + 1));
  }

  return !path->empty() && !version->empty();
}

bool parse_content_length(std::string_view value, size_t *content_length) {
  size_t parsed = 0;

  if (value.empty()) {
    return false;
  }

  for (unsigned char ch : value) {
    if (!std::isdigit(ch)) {
      return false;
    }
    if (parsed > (std::numeric_limits<size_t>::max() - (ch - '0')) / 10) {
      return false;
    }
    parsed = parsed * 10 + (ch - '0');
  }

  *content_length = parsed;
  return true;
}

bool is_form_content_type(std::string_view content_type) {
  static constexpr std::string_view kFormType = "application/x-www-form-urlencoded";

  return content_type.substr(0, kFormType.size()) == kFormType;
}

mapping_t *build_request_mapping(const std::string &method, const std::string &path,
                                 const std::string &version, const std::string &query_string,
                                 mapping_t *headers, const std::string &body,
                                 const std::string &content_type) {
  mapping_t *request = allocate_mapping(8);
  mapping_t *query = http_decode_kv_string(query_string);
  mapping_t *form = is_form_content_type(content_type) ? http_decode_kv_string(body)
                                                       : allocate_mapping(0);

  add_mapping_string(request, "method", method.c_str());
  add_mapping_string(request, "path", path.c_str());
  add_mapping_string(request, "version", version.c_str());
  add_mapping_string(request, "query_string", query_string.c_str());
  add_mapping_mapping(request, "query", query);
  query->ref--;
  add_mapping_mapping(request, "headers", headers);
  headers->ref--;
  add_mapping_string(request, "body", body.c_str());
  add_mapping_mapping(request, "form", form);
  form->ref--;

  return request;
}

mapping_t *build_feed_result(const std::vector<mapping_t *> &requests, mapping_t *error,
                             bool need_more) {
  mapping_t *result = allocate_mapping(3);
  array_t *request_array = allocate_empty_array(requests.size());

  for (size_t i = 0; i < requests.size(); i++) {
    request_array->item[i].type = T_MAPPING;
    request_array->item[i].subtype = 0;
    request_array->item[i].u.map = requests[i];
  }

  add_mapping_array(result, "requests", request_array);
  request_array->ref--;

  if (error) {
    add_mapping_mapping(result, "error", error);
    error->ref--;
  } else {
    add_mapping_pair(result, "error", 0);
  }

  add_mapping_pair(result, "need_more", need_more ? 1 : 0);
  return result;
}

HttpParserState *find_http_parser(LPC_INT handle) {
  if (handle < 1 || static_cast<size_t>(handle) > g_http_parsers.size()) {
    return nullptr;
  }

  auto &state = g_http_parsers[handle - 1];
  if (!state.in_use) {
    return nullptr;
  }

  return &state;
}

}  // namespace

LPC_INT http_parser_create_handle() {
  for (size_t i = 0; i < g_http_parsers.size(); i++) {
    if (!g_http_parsers[i].in_use) {
      g_http_parsers[i].in_use = true;
      g_http_parsers[i].buffer.clear();
      return static_cast<LPC_INT>(i + 1);
    }
  }

  g_http_parsers.push_back(HttpParserState{true, ""});
  return static_cast<LPC_INT>(g_http_parsers.size());
}

mapping_t *http_parser_feed_handle(LPC_INT handle, std::string_view chunk) {
  auto *state = find_http_parser(handle);
  std::vector<mapping_t *> requests;
  mapping_t *error_map = nullptr;
  bool need_more = false;

  if (!state) {
    error("Attempt to use an invalid HTTP parser handle\n");
  }

  state->buffer.append(chunk.data(), chunk.size());

  while (true) {
    size_t header_end = state->buffer.find("\r\n\r\n");
    std::string method;
    std::string path;
    std::string version;
    std::string query_string;
    std::string content_type;
    mapping_t *headers;
    size_t content_length = 0;

    if (header_end == std::string::npos) {
      need_more = true;
      break;
    }

    std::string_view head(state->buffer.data(), header_end);
    size_t line_end = head.find("\r\n");
    std::string_view request_line =
        line_end == std::string_view::npos ? head : head.substr(0, line_end);
    size_t offset = line_end == std::string_view::npos ? head.size() : line_end + 2;

    if (!parse_request_line(request_line, &method, &path, &version, &query_string)) {
      error_map = make_error(400, "malformed_request", "Invalid HTTP request line");
      break;
    }

    headers = allocate_mapping(4);
    while (offset < head.size()) {
      size_t next_end = head.find("\r\n", offset);
      std::string_view raw_line =
          next_end == std::string_view::npos ? head.substr(offset) : head.substr(offset, next_end - offset);
      size_t colon = raw_line.find(':');
      std::string header_name;
      std::string header_value;

      if (colon == std::string_view::npos || colon == 0) {
        free_mapping(headers);
        error_map = make_error(400, "malformed_header", "Invalid HTTP header line");
        break;
      }

      header_name = lower_ascii(trim_ascii(raw_line.substr(0, colon)));
      header_value = trim_ascii(raw_line.substr(colon + 1));
      add_mapping_string(headers, header_name.c_str(), header_value.c_str());

      if (header_name == "content-length") {
        if (!parse_content_length(header_value, &content_length)) {
          free_mapping(headers);
          error_map = make_error(400, "invalid_content_length", "Invalid Content-Length header");
          break;
        }
      } else if (header_name == "content-type") {
        content_type = lower_ascii(header_value);
      }

      if (next_end == std::string_view::npos) {
        offset = head.size();
      } else {
        offset = next_end + 2;
      }
    }

    if (error_map) {
      break;
    }

    if (state->buffer.size() < header_end + 4 + content_length) {
      free_mapping(headers);
      need_more = true;
      break;
    }

    std::string body = state->buffer.substr(header_end + 4, content_length);
    requests.push_back(
        build_request_mapping(method, path, version, query_string, headers, body, content_type));
    state->buffer.erase(0, header_end + 4 + content_length);

    if (state->buffer.empty()) {
      break;
    }
  }

  return build_feed_result(requests, error_map, need_more);
}

void http_parser_close_handle(LPC_INT handle) {
  auto *state = find_http_parser(handle);

  if (!state) {
    return;
  }

  state->buffer.clear();
  state->in_use = false;
}
