#include <kernel/kernel.h>

#include <phantasmal/log.h>

#include <config.h>
#include <type.h>

/* Commandset processing */
private mixed* command_sets;

/* Prototypes */
private mixed* load_command_sets_file(string filename);


static void create(void) {
  command_sets = nil;
}

void upgraded(varargs int clone) {
  if(SYSTEM()) {
    command_sets = load_command_sets_file(USER_COMMANDS_FILE);
    if(!command_sets) {
      LOGD->write_syslog("Command_sets is Nil!", LOG_FATAL);
    }
  } else
    error("Non-System code called upgraded!");
}

int num_command_sets(int loc) {
  if(!SYSTEM())
    return 0;

  if(!command_sets[loc])
    return 0;

  return sizeof(command_sets[loc]);
}

mixed* query_command_sets(int loc, int num, string cmd) {
  mixed* ret;

  if(!SYSTEM())
    return nil;

  ret = command_sets[loc][num][cmd];

  if(ret) {
    return ret[..];
  } else {
    return nil;
  }
}

/* Used when loading commands from an UNQ file.  Arg tmp_cmd is a set
   to add commands to.  Loc is the locale.  Cmds is a string with
   commands and corresponding function names in a specific format
   delimited by commas and slashes --

          cmd_name1 / func1,
          cmd_name2 / func2,
          cmd_name3 / func3,
          etc.
*/
private void add_commands_from_file(mixed* tmp_cmd, int loc, int which,
				    string cmds) {
  mixed* entries;
  int    ctr;
  string cmd, func;

  /* Make sure tmp_cmd[loc] consists of an array with at least a single
     empty mapping... */
  if(!tmp_cmd[loc]) {
    tmp_cmd[loc] = ({ ([ ]) });
  }
  if(!sizeof(tmp_cmd[loc])) {
    tmp_cmd[loc] += ({ ([ ]) });
  }

  /* If the mapping in that array is uninitialized, initialize it */
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
    tmp_cmd[loc][which][cmd] = ({ func });
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
  LOGD->write_syslog("Allocating command_sets", LOG_VERBOSE);
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

    /* Add commands to set #0 */
    add_commands_from_file(tmp_cmd, loc, 0, unq[ctr + 1]);
  }
  if(err) {
    LOGD->write_syslog("Error decoding UNQ, not updating command_sets!",
		       LOG_ERR);
    return command_sets;
  }

  return tmp_cmd;
}
