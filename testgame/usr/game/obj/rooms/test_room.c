static void create(void) {
  ::create();
}

void upgraded(void) {
  if(SYSTEM()) {
    ::upgraded();
  }
}

void destructed(void) {
  if(SYSTEM()) {
    ::destructed();
  }
}
