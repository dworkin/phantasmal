#include <config.h>
#include <log.h>

void create(void) {
  write_file("/usr/game/test.txt", "We made it here.\n");
}
