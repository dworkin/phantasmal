#include <phantasmal/lpc_names.h>
#include <phantasmal/timed.h>

#include <gameconfig.h>
#include <config.h>

static void create(void) {

}

void set_up_heart_beat(void) {
  if(previous_program() == GAME_INITD) {
    TIMED->set_heart_beat(TIMED_HALF_MINUTE, "heart_beat_func");
  }
}

void heart_beat_func(void) {
  if(previous_program() == TIMED) {

  }
}
