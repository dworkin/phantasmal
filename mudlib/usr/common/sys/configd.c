#include <kernel/kernel.h>
#include <config.h>
#include <log.h>
#include <type.h>

#define AUTHORIZED() (SYSTEM() || KERNEL() || GAME())

/* Prototypes */
void upgraded(varargs int clone);


/* Data from config file*/
int start_room;
int meat_locker;


static void create(void) {
  upgraded();
}

void upgraded(varargs int clone) {
  if(SYSTEM()) {

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
