#include <kernel/user.h>
#include <kernel/kernel.h>
#include <config.h>
#include <log.h>

static string suspended_banner, shutdown_banner;
static object logd;
static int    suspended, shutdown;

void upgraded(varargs int clone);

static void create(varargs int clone) {
  if(clone) {
    error("Can't clone TelnetD!");
  }

  upgraded();
}

void upgraded(varargs int clone) {
  if(!SYSTEM())
    return;

  if(!find_object(SYSTEM_USER)) { compile_object(SYSTEM_USER); }

  logd = find_object(LOGD);
  suspended_banner = ::read_file(SUSPENDED_MESSAGE);
  if(!suspended_banner)
    error("Can't read 'suspended' message file " + SUSPENDED_MESSAGE + "!");
  shutdown_banner = ::read_file(SHUTDOWN_MESSAGE);
  if(!shutdown_banner)
    error("Can't read 'shutdown' message file " + SHUTDOWN_MESSAGE + "!");
}

void suspend_input(int shutdownp) {
  if(!SYSTEM() && !KERNEL())
    return;

  if(suspended)
    LOGD->write_syslog("Suspended again without release!", LOG_ERR);

  suspended = 1;
  if(shutdownp)
    shutdown = 1;
}

void release_input(void) {
  if(!SYSTEM() && !KERNEL())
    return;

  if(!suspended)
    LOGD->write_syslog("Released without suspend!", LOG_ERR);

  suspended = 0;
}

object select(string str)
{
  if(!SYSTEM() && !KERNEL())
    return nil;

  return clone_object(SYSTEM_USER);
}

int query_timeout(object connection)
{
  if(!SYSTEM() && !KERNEL())
    return -1;

  if(suspended || shutdown)
    return -1;

  return DEFAULT_TIMEOUT;
}

string query_banner(object connection)
{
  if(!SYSTEM() && !KERNEL())
     return nil;

  if(shutdown)
    return CONFIGD->get_shutdown_message();

  if(suspended)
    return CONFIGD->get_suspended_message();

  return CONFIGD->get_welcome_message();
}
