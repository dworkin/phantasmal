#include <kernel/kernel.h>

#include <phantasmal/log.h>
#include <phantasmal/lpc_names.h>
#include <phantasmal/lpc_names.h>

#include <gameconfig.h>
#include <type.h>

/* Since the ConfigD sets the path_special object, obviously ConfigD
   gets compiled before the path_special object is set. */
inherit COMMON_AUTO;

#define AUTHORIZED() (SYSTEM() || KERNEL() || GAME())

/* Prototypes */
void upgraded(varargs int clone);


/* Data from config file*/
object game_driver;

static void create(void) {
  game_driver = nil;

  if(!find_object(PATHSPECIAL))
    compile_object(PATHSPECIAL);

  INITD->set_path_special_object(find_object(PATHSPECIAL));  

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
  if(previous_program() == GAME_INITD || previous_program() == INITD) {
    PATHSPECIAL->set_game_path_object(new_obj);

    INITD->set_path_special_object(find_object(PATHSPECIAL));
  } else {
    error("Only GAME_INITD can set the path_special object!");
  }
}
