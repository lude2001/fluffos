class Person {
  string name;
} Me;

void create() {
  Me = new(class Person);
  Me->name = "Jeremy";
}
