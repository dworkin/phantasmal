#include <kernel/user.h>
#include <config.h>

#define WELCOME_MESSAGE "/data/text/welcome.msg"

static string welcome;
static object logd;

static void create(varargs int clone) {
  if(!find_object(SYSTEM_USER)) { compile_object(SYSTEM_USER); }
  logd = find_object(LOGD);
  if(clone) {
    logd->write_syslog("Telnetd cloned!");
  }
  welcome = ::read_file(WELCOME_MESSAGE);
}

object select(string str)
{
  return clone_object(SYSTEM_USER);
}

int query_timeout(object connection)
{
  return DEFAULT_TIMEOUT;
}

string query_banner(object connection)
{
  return welcome;
}
