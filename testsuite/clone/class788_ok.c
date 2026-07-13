class Person {
  string name;
}

probe() {
  class Person p = new(class Person);
  p->name = "jeremy";
  return p->name;
}

class Counter {
  int n;
}

int typed_after_class;

class Person Me;

string probe2() {
  Me = new(class Person);
  Me->name = "kyle";
  return Me->name;
}
