#include "base/package_api.h"

#include <nlohmann/json.hpp>

#include <limits>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>

using json = nlohmann::json;

namespace {

bool is_undefined(const svalue_t *sv) {
  return sv->type == T_NUMBER && sv->subtype == T_UNDEFINED && sv->u.number == 0;
}

struct visit_key_t {
  unsigned short type;
  const void *ptr;

  bool operator==(const visit_key_t &other) const {
    return type == other.type && ptr == other.ptr;
  }
};

struct visit_key_hash_t {
  size_t operator()(const visit_key_t &key) const {
    auto type_hash = std::hash<unsigned short>{}(key.type);
    auto ptr_hash = std::hash<const void *>{}(key.ptr);
    return type_hash ^ (ptr_hash + 0x9e3779b9 + (type_hash << 6) + (type_hash >> 2));
  }
};

visit_key_t visit_key(unsigned short type, const void *ptr) { return {type, ptr}; }

json svalue_to_json_compat(const svalue_t *sv, std::unordered_set<visit_key_t, visit_key_hash_t> &seen) {
  if (is_undefined(sv)) {
    return nullptr;
  }

  switch (sv->type) {
    case T_NUMBER:
      return sv->u.number;
    case T_REAL:
      return sv->u.real;
    case T_STRING:
      return std::string(sv->u.string, SVALUE_STRLEN(sv));
    case T_ARRAY: {
      auto key = visit_key(T_ARRAY, sv->u.arr);
      if (seen.find(key) != seen.end()) {
        return nullptr;
      }
      seen.insert(key);
      json out = json::array();
      for (int i = 0; i < sv->u.arr->size; i++) {
        out.push_back(svalue_to_json_compat(&sv->u.arr->item[i], seen));
      }
      seen.erase(key);
      return out;
    }
    case T_MAPPING: {
      auto key = visit_key(T_MAPPING, sv->u.map);
      if (seen.find(key) != seen.end()) {
        return nullptr;
      }
      seen.insert(key);
      json out = json::object();
      for (unsigned int i = 0; i <= sv->u.map->table_size; i++) {
        for (auto *node = sv->u.map->table[i]; node; node = node->next) {
          auto *map_key = &node->values[0];
          if (map_key->type != T_STRING) {
            continue;
          }
          out[std::string(map_key->u.string, SVALUE_STRLEN(map_key))] =
              svalue_to_json_compat(&node->values[1], seen);
        }
      }
      seen.erase(key);
      return out;
    }
    default:
      return nullptr;
  }
}

svalue_t json_to_svalue_compat(const json &value) {
  svalue_t sv{};

  if (value.is_null()) {
    sv = const0;
    return sv;
  }

  if (value.is_boolean()) {
    sv.type = T_NUMBER;
    sv.subtype = 0;
    sv.u.number = value.get<bool>() ? 1 : 0;
    return sv;
  }

  if (value.is_number_unsigned()) {
    auto number = value.get<uint64_t>();
    if (number > static_cast<uint64_t>(std::numeric_limits<LPC_INT>::max())) {
      throw std::runtime_error("Unsigned integer exceeds LPC_INT range");
    }
    sv.type = T_NUMBER;
    sv.subtype = 0;
    sv.u.number = static_cast<LPC_INT>(number);
    return sv;
  }

  if (value.is_number_integer()) {
    sv.type = T_NUMBER;
    sv.subtype = 0;
    sv.u.number = value.get<LPC_INT>();
    return sv;
  }

  if (value.is_number_float()) {
    sv.type = T_REAL;
    sv.subtype = 0;
    sv.u.real = value.get<LPC_FLOAT>();
    return sv;
  }

  if (value.is_string()) {
    auto text = value.get<std::string>();
    sv.type = T_STRING;
    sv.subtype = STRING_MALLOC;
    sv.u.string = string_copy(text.c_str(), "json_to_svalue_compat: string");
    return sv;
  }

  if (value.is_array()) {
    sv.type = T_ARRAY;
    sv.subtype = 0;
    sv.u.arr = allocate_array(value.size());
    try {
      for (int i = 0; i < value.size(); i++) {
        auto item = json_to_svalue_compat(value[i]);
        assign_svalue_no_free(&sv.u.arr->item[i], &item);
        free_svalue(&item, "json_to_svalue_compat: array item");
      }
    } catch (...) {
      free_array(sv.u.arr);
      throw;
    }
    return sv;
  }

  if (value.is_object()) {
    sv.type = T_MAPPING;
    sv.subtype = 0;

    array_t *keys = allocate_array(value.size());
    array_t *vals = allocate_array(value.size());
    int idx = 0;

    try {
      for (auto it = value.begin(); it != value.end(); ++it) {
        svalue_t key{};
        key.type = T_STRING;
        key.subtype = STRING_MALLOC;
        key.u.string = string_copy(it.key().c_str(), "json_to_svalue_compat: key");

        auto val = json_to_svalue_compat(it.value());

        assign_svalue_no_free(&keys->item[idx], &key);
        assign_svalue_no_free(&vals->item[idx], &val);

        free_svalue(&key, "json_to_svalue_compat: key");
        free_svalue(&val, "json_to_svalue_compat: value");
        idx++;
      }

      sv.u.map = mkmapping(keys, vals);
    } catch (...) {
      free_array(keys);
      free_array(vals);
      throw;
    }

    free_array(keys);
    free_array(vals);
    return sv;
  }

  sv = const0;
  return sv;
}

void assign_return_svalue(svalue_t *dest, svalue_t *value, const char *tag) {
  free_svalue(dest, tag);
  assign_svalue_no_free(dest, value);
  free_svalue(value, tag);
}

}  // namespace

#ifdef F_JSON_DECODE
void f_json_decode() {
  std::string input(sp->u.string, SVALUE_STRLEN(sp));
  if (input.empty()) {
    free_svalue(sp, "f_json_decode: empty input");
    *sp = const0;
    return;
  }

  try {
    auto parsed = json::parse(input);
    auto result = json_to_svalue_compat(parsed);
    assign_return_svalue(sp, &result, "f_json_decode");
  } catch (const std::exception &e) {
    error("json_decode error: %s\n", e.what());
  }
}
#endif

#ifdef F_JSON_ENCODE
void f_json_encode() {
  try {
    std::unordered_set<visit_key_t, visit_key_hash_t> seen;
    auto dumped = svalue_to_json_compat(sp, seen).dump();
    put_malloced_string(string_copy(dumped.c_str(), "f_json_encode"));
  } catch (const std::exception &e) {
    error("json_encode error: %s\n", e.what());
  }
}
#endif

#ifdef F_JSON_FORMAT
void f_json_format() {
  int const num_arg = st_num_arg;
  int indent = 2;
  std::string input((sp - num_arg + 1)->u.string, SVALUE_STRLEN(sp - num_arg + 1));

  if (num_arg == 2) {
    indent = (sp - num_arg + 2)->u.number;
    if (indent <= 0) {
      indent = 2;
    }
  }

  try {
    auto formatted = json::parse(input).dump(indent);
    pop_n_elems(num_arg);
    push_malloced_string(string_copy(formatted.c_str(), "f_json_format"));
  } catch (const std::exception &e) {
    error("json_format error: %s\n", e.what());
  }
}
#endif
