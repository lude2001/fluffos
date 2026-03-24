inherit "/single/tests/compiler/inherit_proto_mid";

string call_parent() {
  return inherit_proto_mid::inherited_target();
}
