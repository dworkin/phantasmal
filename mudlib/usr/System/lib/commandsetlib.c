#include <config.h>
#include <log.h>
#include <type.h>

/* Commandset processing */
private mixed* command_sets;

/* Prototypes */
private mixed* load_command_sets_file(string filename);


void create(void) {
  command_sets = nil;
}

void upgraded(void) {
  command_sets = load_command_sets_file(USER_COMMANDS_FILE);
  if(!command_sets) {
    LOGD->write_syslog("Command_sets is Nil!", LOG_FATAL);
  }
}

int num_command_sets(int loc) {
  if(!command_sets[loc])
    return 0;

  return sizeof(command_sets[loc]);
}

mixed* query_command_sets(int loc, int num, string cmd) {
  mixed* ret;

  ret = command_sets[loc][num][cmd];

  if(ret) {
    return ret[..];
  } else {
    return nil;
  }
}

private void add_commands_to_set(mixed* tmp_cmd, int loc, string cmds) {
  mixed* entries;
  int    ctr;
  string cmd, func;

  if(!tmp_cmd[loc]) {
    tmp_cmd[loc] = ({ ([ ]) });
  }
  if(!sizeof(tmp_cmd[loc])) {
    tmp_cmd[loc] += ({ ([ ]) });
  }
  if(!tmp_cmd[loc][0]) {
    tmp_cmd[loc][0] = ([ ]);
  }

  entries = explode(cmds, ",");
  for(ctr = 0; ctr < sizeof(entries); ctr++) {
    entries[ctr] = STRINGD->trim_whitespace(entries[ctr]);
    if(sscanf(entries[ctr], "%s/%s", cmd, func) != 2) {
      error("Command entry doesn't match format: '" + entries[ctr]
	    + "'");
    }
    cmd = STRINGD->trim_whitespace(cmd);
    func = STRINGD->trim_whitespace(func);
    tmp_cmd[loc][0][cmd] = ({ func });
  }
}

private mixed* load_command_sets_file(string filename) {
  mixed*  tmp_cmd, *unq;
  string  file;
  int     ctr, err, loc;

  file = read_file(filename);
  if(!file) {
    LOGD->write_syslog("Couldn't find file " + filename
		       + ", not updating command_sets!", LOG_ERR);
    return command_sets;
  }

  if(!find_object(PHRASED)) { compile_object(PHRASED); }
  LOGD->write_syslog("Allocating command_sets", LOG_NORMAL);
  tmp_cmd = allocate(PHRASED->num_locales());

  unq = UNQ_PARSER->basic_unq_parse(file);
  if(!unq) {
    LOGD->write_syslog("Couldn't parse file " + filename
		       + " as UNQ!, not updating command_sets!", LOG_ERR);
    return command_sets;
  }

  err = 0;
  for(ctr = 0; ctr < sizeof(unq); ctr += 2) {
    /* Filter out whitespace */
    if(!unq[ctr] || STRINGD->is_whitespace(unq[ctr])) {
      if(typeof(unq[ctr + 1]) != T_STRING
	 || !STRINGD->is_whitespace(unq[ctr + 1])) {
	LOGD->write_syslog("Error: not type T_STRING", LOG_ERR);
	err = 1;
	break;
      }
      continue;
    }
    if(typeof(unq[ctr + 1]) != T_STRING) {
      LOGD->write_syslog("Error: not type T_STRING: '"
			 + STRINGD->mixed_sprint(unq[ctr + 1]) + "'/"
			 + typeof(unq[ctr + 1]), LOG_ERR);
      err = 1;
      break;
    }

    /* Trim whitespace in remaining entries */
    unq[ctr] = STRINGD->trim_whitespace(unq[ctr]);
    unq[ctr+1] = STRINGD->trim_whitespace(unq[ctr+1]);

    loc = PHRASED->language_by_name(unq[ctr]);
    if(loc == -1) {
      LOGD->write_syslog("Error: unknown locale '" + unq[ctr] + "'", LOG_ERR);
      err = 1;
      break;
    }

    add_commands_to_set(tmp_cmd, loc, unq[ctr + 1]);
  }
  if(err) {
    LOGD->write_syslog("Error decoding UNQ, not updating command_sets!",
		       LOG_ERR);
    return command_sets;
  }

  return tmp_cmd;
}
