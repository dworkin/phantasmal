#include <kernel/kernel.h>

#include <phantasmal/log.h>

#include <config.h>
#include <gameconfig.h>
#include <type.h>

#define AUTHORIZED() (SYSTEM() || KERNEL() || GAME())

/* Prototypes */
void upgraded(varargs int clone);


/* Data from config file*/
object game_driver;

static void create(void) {
  game_driver = nil;

  upgraded();
}

void upgraded(varargs int clone) {
  if(SYSTEM() || COMMON()) {

  }
}

void set_game_driver(object new_driver) {
  if(previous_program() == GAME_INITD)
    game_driver = new_driver;
}

object get_game_driver(void) {
  if(SYSTEM() || COMMON())
    return game_driver;

  return nil;
}

void set_path_special_object(object new_obj) {
  if(previous_program() == GAME_INITD) {
    INITD->set_path_special_object(new_obj);
  }
}
