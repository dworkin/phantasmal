static void create(void) {
  ::create();

  ldesc = PHR("You see a test room.  It has been carefully fashioned out"
	      + " of cheese.");
  bdesc = PHR("cheese test room");
  edesc = PHR("You see a test room.  It has been carefully fashioned out"
	      + " of cheese.  It kinda smells funny.");
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
