#include "phantasmal/log.h"

static void room_created(void) {
  LOGD->write_syslog("Room has been created.", LOG_WARNING);
}

static void room_upgraded(void) {
  LOGD->write_syslog("Room has been upgraded.", LOG_WARNING);
}

static void room_destructed(void) {
  LOGD->write_syslog("Room has been destructed.", LOG_WARNING);
}
