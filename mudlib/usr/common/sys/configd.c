#include <kernel/kernel.h>

#include <phantasmal/log.h>

#include <config.h>
#include <gameconfig.h>
#include <type.h>

#define AUTHORIZED() (SYSTEM() || KERNEL() || GAME())

/* Prototypes */
void upgraded(varargs int clone);


/* Data from config file*/
int    start_room;
int    meat_locker;
object game_driver;

static void create(void) {
  game_driver = nil;

  upgraded();
}

void upgraded(varargs int clone) {
  if(SYSTEM() || COMMON()) {

  }
}

int get_start_room(void) {
  if(!AUTHORIZED())
    return -1;

  return start_room;
}

void set_start_room(int new_room) {
  if(!AUTHORIZED())
    return;

  start_room = new_room;
}

int get_meat_locker(void) {
  if(!AUTHORIZED())
    return -1;

  return meat_locker;
}

void set_meat_locker(int new_room) {
  if(!AUTHORIZED())
    return;

  meat_locker = new_room;
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
