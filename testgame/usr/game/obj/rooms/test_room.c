static void create(varargs int clone) {
  ::create(clone);
}

void upgraded(varargs int clone) {
  if(SYSTEM()) {
    ::upgraded(clone);
  }
}

void destructed(void) {
  if(SYSTEM()) {
    ::destructed();
  }
}
