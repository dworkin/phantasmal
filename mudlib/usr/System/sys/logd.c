#include <config.h>
#include <limits.h>
#include <type.h>
#include <log.h>
#include <kernel/kernel.h>
#include <channel.h>

#define SYSLOGFILE   (LOGDIR + "/System.log")
#define LOGD_DTD     ("/usr/System/sys/logchannel.dtd")

private mapping channels;
private mapping levelname;
private object  chan_dtd;

/* Prototypes */
void write_syslog(string logstring, varargs int level, string channel);

private void read_channels(mixed* unq);

static void create(varargs int clone) {
  if(clone)
    error("Can't clone LOGD!");

  levelname = ([ "fatal" : LOG_ERR_FATAL,
		 "fatalerr" : LOG_ERR_FATAL,
		 "fatal error" : LOG_ERR_FATAL,
		 "fatalerror" : LOG_ERR_FATAL,
		 "err" : LOG_ERR,
		 "error" : LOG_ERR,
		 "warn" : LOG_WARN,
		 "warning" : LOG_WARN,
		 "debug" : LOG_DEBUG,
		 "debugmsg" : LOG_DEBUG,
		 "message" : LOG_DEBUG,
		 "normal" : LOG_NORMAL,
		 "verbose" : LOG_VERBOSE,
		 "ultra" : LOG_ULTRA_VERBOSE,
		 "ultraverbose" : LOG_ULTRA_VERBOSE,
		 "ultra verbose" : LOG_ULTRA_VERBOSE,
		 "everything" : LOG_ULTRA_VERBOSE,
		 ]);
}

void destructed(int clone) {
  if(previous_program() == OBJECTD) {
    destruct_object(chan_dtd);
  }
}

void start_channels(void) {
  string dtd_file;
  string channels_file;
  mixed* unq;

  if(channels)
    error("Channels already started up in LOGD!");

  channels = ([ ]);

  dtd_file = read_file(LOGD_DTD);
  if(strlen(dtd_file) > MAX_STRING_SIZE - 3)
    error("LOGD's DTD file is too long!");
  channels_file = read_file(LOG_CHANNELS);
  if(!channels_file)
    error("Can't read channels file ('" + LOG_CHANNELS + "')");
  if(strlen(channels_file) > MAX_STRING_SIZE - 3)
    error("LOGD's Channels file is too long!");

  if(!find_object(UNQ_PARSER)) { compile_object(UNQ_PARSER); }
  if(!find_object(UNQ_DTD)) { compile_object(UNQ_DTD); }

  chan_dtd = clone_object(UNQ_DTD);
  chan_dtd->load(dtd_file);

  unq = UNQ_PARSER->unq_parse_with_dtd(channels_file, chan_dtd);

  read_channels(unq);
}

void write_syslog(string logstring, varargs int level, string channel) {
  string to_write;
  mixed* the_time;
  mixed  chan_lev;

  if(!level)
    level = LOG_ERR_FATAL;  /* Highest priority */

  if(!channel) {
    channel = previous_program();
  } else if(!SYSTEM())
    error("Only System files can impersonate other channels!");

  if(channels) {
    chan_lev = channels[channel];
  }

  /* If it's a path, include only the final filename in the log */
  if(channel[0] == "/"[0]) {
    string* comp;
    int     ctr;

    comp = explode(channel, "/");
    if(comp) {
      ctr = sizeof(comp) - 1;

      while(ctr >= 0) {
	if(comp[ctr]) {
	  channel = comp[ctr];
	  break;
	}
	ctr--;
      }
    }
  }

  if(channels && chan_lev) {
    if(level < chan_lev) {
      /* Won't write to file */
      /* TODO: decide whether to send out on channeld channel(s) */
      return;
    }

    /* Change output string */
    channel += "(" + chan_lev + ")";
  } else {
    if(level < LOG_NORMAL) {
      /* Channel isn't explicitly set, do nothing for now */
      return;
    }

    channel += "(unset)";
  }

  to_write = channel;

  to_write += " ||| ";
  if(this_user()) {
    if(this_user()->get_Name()) {
      to_write += this_user()->get_Name();
    } else {
      to_write += "(nameless)";
    }
  } else {
    to_write += "(no user)";
  }

  the_time = millitime();
  to_write += " ||| " + ctime(the_time[0]) + " / "
    + (string)the_time[1] + " [" + (string)level + "]\n"
    + "  --> " + logstring + "\n";
  if(!write_file(SYSLOGFILE, to_write)) {
    /* Failure -- try to deal with it */
    send_message("Help!  Log failure!\n");
    write_file("/helplog.txt", "Log failure!  Bad one!\n");
  }
  if(find_object(CHANNELD))
    CHANNELD->string_to_channel(CHANNEL_LOG, to_write, level);
}

private void read_channels(mixed* unq) {
  while(sizeof(unq)) {
    if(unq[0] != "channel"
       || sizeof(unq[1]) != 2) {
      error("Label '" + unq[0] + "/" + sizeof(unq[1])
	    + "' does not appear to be a LOGD channel!");
    }

    if(unq[1][0][0] != "name"
       || unq[1][1][0] != "level") {
      error("Name/level structure (" + unq[1][0][0] + "/" + unq[1][1][0]
	    + ") appears incorrect in LOGD channel!");
    }

    if(typeof(unq[1][0][1]) != T_STRING
       || typeof(unq[1][1][1]) != T_INT) {
      error("Name must be a string and level must be an int in LOGD channel!");
    }

    channels[unq[1][0][1]] = unq[1][1][1];

    unq = unq[2..];
  }
}

int channel_sub(string channel) {
  if(!SYSTEM())
    return -1;

  if(!channels[channel])
    return -1;
  return channels[channel];
}

void set_channel_sub(string channel, int level) {
  if(SYSTEM())
    channels[channel] = level;
}

mixed get_level_by_name(string name) {
  return levelname[name];
}
