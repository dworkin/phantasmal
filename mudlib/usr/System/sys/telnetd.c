#include <kernel/user.h>
#include <config.h>
#include <log.h>

#define WELCOME_MESSAGE "/data/text/welcome.msg"

static string welcome;
static object logd;
static int    suspended;

void upgraded(varargs int clone);

static void create(varargs int clone) {
  if(clone) {
    error("Can't clone Telnetd!");
  }

  upgraded();
}

void upgraded(varargs int clone) {
  if(!find_object(SYSTEM_USER)) { compile_object(SYSTEM_USER); }

  logd = find_object(LOGD);
  welcome = ::read_file(WELCOME_MESSAGE);
  if(!welcome)
    error("Can't read welcome message file " + WELCOME_MESSAGE + "!");
}

void suspend_input(void) {
  if(suspended)
    LOGD->write_syslog("Suspended again without release!", LOG_ERR);

  suspended = 1;
}

void release_input(void) {
  if(!suspended)
    LOGD->write_syslog("Released without suspend!", LOG_ERR);
  suspended = 0;
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
