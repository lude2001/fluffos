// Hot-reload daemon: automatically reloads watched programs when any source
// file they were compiled from changes on disk.
//
// The driver's inherit_program / include_file master applies report, during
// every compile, which files feed which program. This daemon records that
// dependency graph, snapshots file size+mtime, and uses recompile_object() for
// state-keeping reloads when possible.

#include <globals.h>

#define MASTER "/single/master"
#define DEFAULT_INTERVAL 2

#define WATCH_PLAIN 1
#define WATCH_KEEP_STATE 2

private mapping file_deps = ([]);
private mapping inherit_deps = ([]);
private mapping watched = ([]);
private mapping snapshot = ([]);

private int poll_interval = 0;
private int reload_count = 0;

private string program_of(string file) {
  if (strlen(file) > 4 && file[<4..] == ".lpc") return file[0..<5];
  if (strlen(file) > 2 && file[<2..] == ".c") return file[0..<3];
  return file;
}

private string source_of(string prog) {
  if (file_size(prog + ".lpc") >= 0) return prog + ".lpc";
  if (file_size(prog + ".c") >= 0) return prog + ".c";
  return 0;
}

private string resolve_include(string from, string path) {
  string *candidates;

  if (path[0] == '/') {
    candidates = ({ path });
  } else {
    int slash = strsrch(from, '/', -1);
    string dir = slash > 0 ? from[0..slash - 1] : "";
    candidates = ({ dir + "/" + path, "/include/" + path });
  }
  foreach (string c in candidates) {
    if (file_size(c) >= 0) return c;
  }
  return 0;
}

mixed include_file(string compiled, string from, string path) {
  string prog = program_of(compiled);
  string dep = resolve_include(from, path);

  if (dep) {
    if (!file_deps[prog]) file_deps[prog] = ([]);
    file_deps[prog][dep] = 1;
  }
  return path;
}

mixed inherit_program(string from, string path, int priv) {
  string prog = program_of(from);

  if (!inherit_deps[prog]) inherit_deps[prog] = ([]);
  inherit_deps[prog][program_of(path)] = 1;
  return path;
}

private mapping closure_files(string prog, mapping seen) {
  mapping out = ([]);
  string src;

  if (seen[prog]) return out;
  seen[prog] = 1;

  src = source_of(prog);
  if (src) out[src] = 1;
  if (file_deps[prog]) out += file_deps[prog];
  if (inherit_deps[prog]) {
    foreach (string parent in keys(inherit_deps[prog])) {
      out += closure_files(parent, seen);
    }
  }
  return out;
}

private string *ancestors(string prog, mapping seen) {
  string *out = ({ });

  if (!inherit_deps[prog]) return out;
  foreach (string parent in keys(inherit_deps[prog])) {
    if (seen[parent]) continue;
    seen[parent] = 1;
    out += ancestors(parent, seen) + ({ parent });
  }
  return out;
}

private mixed *stat_file(string f) {
  mixed *info = get_dir(f, -1);
  if (!sizeof(info)) return 0;
  return ({ info[0][1], info[0][2] });
}

private int file_changed(string f) {
  mixed *now = stat_file(f);
  mixed *old = snapshot[f];

  if (!old) return now != 0;
  return !now || now[0] != old[0] || now[1] != old[1];
}

private int self_changed(string prog) {
  string src = source_of(prog);

  if (src && file_changed(src)) return 1;
  if (file_deps[prog]) {
    foreach (string f in keys(file_deps[prog])) {
      if (file_changed(f)) return 1;
    }
  }
  return 0;
}

private int closure_changed(string prog) {
  if (!find_object(prog)) return 1;
  foreach (string f in keys(closure_files(prog, ([])))) {
    if (file_changed(f)) return 1;
  }
  return 0;
}

private void snapshot_program(string prog) {
  foreach (string f in keys(closure_files(prog, ([])))) {
    snapshot[f] = stat_file(f);
  }
}

private mixed capture_state(object ob) { return ob->hot_reload_state(); }

private void restore_state(object ob, mixed state) {
  if (function_exists("hot_reload_restore", ob)) {
    ob->hot_reload_restore(state);
  }
}

private void recompile_with_records(object ob, string prog) {
  mapping fd = file_deps[prog];
  mapping id = inherit_deps[prog];
  string err;

  map_delete(file_deps, prog);
  map_delete(inherit_deps, prog);
  err = catch(recompile_object(ob));
  if (err) {
    if (fd && !file_deps[prog]) file_deps[prog] = fd;
    if (id && !inherit_deps[prog]) inherit_deps[prog] = id;
    error(err);
  }
}

private void reload(string prog) {
  mixed state;
  int have_state = 0;
  object ob = find_object(prog);
  int cooperative = ob && function_exists("hot_reload_state", ob);
  int use_recompile_object = ob && watched[prog] == WATCH_KEEP_STATE && !cooperative;

  foreach (string a in ancestors(prog, ([]))) {
    if (self_changed(a)) {
      object aob = find_object(a);
      if (aob) {
        recompile_with_records(aob, a);
      } else {
        map_delete(file_deps, a);
        map_delete(inherit_deps, a);
      }
    }
  }

  if (use_recompile_object) {
    recompile_with_records(ob, prog);
  } else {
    map_delete(file_deps, prog);
    map_delete(inherit_deps, prog);
    if (cooperative && watched[prog] == WATCH_KEEP_STATE) {
      state = capture_state(ob);
      have_state = 1;
    }
    if (ob) destruct(ob);
    ob = load_object(prog);
    if (have_state) restore_state(ob, state);
  }
  reload_count++;
  snapshot_program(prog);
}

public int watch(string prog, int keep_state: (: 1 :)) {
  prog = program_of(prog);
  if (!find_object(prog)) load_object(prog);
  watched[prog] = keep_state ? WATCH_KEEP_STATE : WATCH_PLAIN;
  snapshot_program(prog);
  return 1;
}

public void unwatch(string prog) {
  map_delete(watched, program_of(prog));
}

public int check_now() {
  int n = 0;
  string *stale = filter(keys(watched), (: closure_changed($1) :));

  foreach (string prog in stale) {
    if (catch(reload(prog))) continue;
    n++;
  }
  return n;
}

protected void poll() {
  if (poll_interval) call_out("poll", poll_interval);
  check_now();
}

public void enable(int interval) {
  poll_interval = interval ? interval : DEFAULT_INTERVAL;
  MASTER->set_compile_hooks(this_object());
  if (find_call_out("poll") == -1) call_out("poll", poll_interval);
}

public void disable() {
  poll_interval = 0;
  while (remove_call_out("poll") != -1) {
  }
  if (MASTER->query_compile_hooks() == this_object()) {
    MASTER->set_compile_hooks(0);
  }
}

public int query_polling() { return find_call_out("poll") != -1; }

public int query_reload_count() { return reload_count; }
