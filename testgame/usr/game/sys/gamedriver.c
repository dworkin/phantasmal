#include <kernel/kernel.h>

#include <config.h>
#include <gameconfig.h>

#define AUTHORIZED() (SYSTEM() || KERNEL() || GAME())

string welcome_message, shutdown_message, suspended_message;

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
}

#define GAME_USER "/usr/game/obj/user"
object new_user_connection(string first_line) {
  if(!find_object(GAME_USER))
    compile_object(GAME_USER);

  return clone_object(GAME_USER);
}

string get_welcome_message(object connection) {
  if(!AUTHORIZED())
    return nil;

  return welcome_message;
}

string get_shutdown_message(object connection) {
  if(!AUTHORIZED())
    return nil;

  return shutdown_message;
}

string get_suspended_message(object connection) {
  if(!AUTHORIZED())
    return nil;

  return suspended_message;
}
