#include <kernel/kernel.h>
#include <config.h>
#include <gameconfig.h>
#include <log.h>
#include <type.h>

#define AUTHORIZED() (SYSTEM() || KERNEL() || GAME())

/* Prototypes */
void upgraded(varargs int clone);


/* Data from config file*/
int start_room;
int meat_locker;
string welcome_message, shutdown_message, suspended_message;


static void create(void) {
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

string get_welcome_message(void) {
  if(!AUTHORIZED())
    return nil;

  return welcome_message;
}

void set_welcome_message(string new_room) {
  if(!AUTHORIZED())
    return;

  welcome_message = new_room;
}

string get_shutdown_message(void) {
  if(!AUTHORIZED())
    return nil;

  return shutdown_message;
}

void set_shutdown_message(string new_room) {
  if(!AUTHORIZED())
    return;

  shutdown_message = new_room;
}

string get_suspended_message(void) {
  if(!AUTHORIZED())
    return nil;

  return suspended_message;
}

void set_suspended_message(string new_room) {
  if(!AUTHORIZED())
    return;

  suspended_message = new_room;
}


void set_path_special_object(object new_obj) {
  if(GAME()) {
    INITD->set_path_special_object(new_obj);
  }
}
