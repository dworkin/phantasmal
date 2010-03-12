#include <phantasmal/lpc_names.h>

#include <kernel/kernel.h>

#include <config.h>
#include <gameconfig.h>

int meat_locker;
int start_room;

#define AUTHORIZED() (SYSTEM() || KERNEL() || GAME())

string welcome_message, shutdown_message, suspended_message;
string sitebanned_message;

static void create(void) {
  string file_tmp;

  file_tmp = read_file("/usr/game/text/welcome.msg");
  if(!file_tmp)
    error("Can't read /usr/game/text/welcome.msg!");
  welcome_message = file_tmp;

  file_tmp = read_file("/usr/game/text/shutdown.msg");
  if(!file_tmp)
    error("Can't read /usr/game/text/shutdown.msg!");
  shutdown_message = file_tmp;

  file_tmp = read_file("/usr/game/text/suspended.msg");
  if(!file_tmp)
    error("Can't read /usr/game/text/suspended.msg!");
  suspended_message = file_tmp;

  file_tmp = read_file("/usr/game/text/sitebanned.msg");
  if(!file_tmp)
    error("Can't read /usr/game/text/sitebanned.msg!");
  sitebanned_message = file_tmp;
}

int get_meat_locker(void) {
  return meat_locker;
}

void set_meat_locker(int new_ml) {
  if(GAME())
    meat_locker = new_ml;
  else
    error("Can't call function!  Not authorized!");
}

int get_start_room(void) {
  return start_room;
}

void set_start_room(int new_sr) {
  if(GAME())
    start_room = new_sr;
  else
    error("Can't call function!  Not authorized!");
}

/**************** Hooks called by Phantasmal: **********************/

#define GAME_USER "/usr/game/obj/user"
object new_user_connection(string first_line) {
  if(!AUTHORIZED())
    error("Can't call function!  Not authorized!");

  if(!find_object(GAME_USER))
    compile_object(GAME_USER);

  return clone_object(GAME_USER);
}

string get_welcome_message(object connection) {
  if(!AUTHORIZED())
    error("Can't call function!  Not authorized!");

  return welcome_message;
}

string get_shutdown_message(object connection) {
  if(!AUTHORIZED())
    error("Can't call function!  Not authorized!");

  return shutdown_message;
}

string get_suspended_message(object connection) {
  if(!AUTHORIZED())
    error("Can't call function!  Not authorized!");

  return suspended_message;
}

string get_sitebanned_message(object connection) {
  if(!AUTHORIZED())
    error("Can't call function!  Not authorized!");

  return sitebanned_message;
}

int site_is_banned(string ip_address) {
  if(!AUTHORIZED())
    error("Can't call function!  Not authorized!");

  return 0;
}
