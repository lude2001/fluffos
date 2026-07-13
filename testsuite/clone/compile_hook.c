mapping inherit_rules = ([]);
mapping include_rules = ([]);
mixed *inherit_calls = ({});
mixed *include_calls = ({});

void reset_rules() {
  inherit_rules = ([]);
  include_rules = ([]);
  inherit_calls = ({});
  include_calls = ({});
}

void create() {
  reset_rules();
}

void set_inherit_rule(string path, mixed rule) {
  inherit_rules[path] = rule;
}

void set_include_rule(string path, mixed rule) {
  include_rules[path] = rule;
}

mixed *query_inherit_calls() {
  return inherit_calls;
}

mixed *query_include_calls() {
  return include_calls;
}

mixed inherit_program(string from, string path, int priv) {
  inherit_calls += ({ ({ from, path, priv }) });
  if (undefinedp(inherit_rules[path])) {
    return path;
  }
  return inherit_rules[path];
}

mixed include_file(string compiled, string from, string path) {
  include_calls += ({ ({ compiled, from, path }) });
  if (undefinedp(include_rules[path])) {
    return path;
  }
  return include_rules[path];
}
