#include <config.h>
#include <limits.h>
#include <type.h>
#include <phrase.h>
#include <trace.h>
#include <status.h>
#include <log.h>

#include <kernel/kernel.h>

/* Reference to Soundexd */
private object  soundex;

/* UNQ DTD for help entries */
private object  help_dtd;

/* Number of locales */
private int     num_loc;

/* Help entries */
private mixed*  exact_entries;
private mapping sdx_entries;

/* Known help directories/files */
private mixed*  path_stack;

/* Paths to exclude */
private mapping path_exc;

/* Vars for loading in call_outs */
string *files_to_load;
int     load_callout;

/* Prototypes */
void load_unq_help(string path, mixed* data);
void upgraded(void);
void reread_help_files(void);
void clear_help_entries(void);
void new_help_file(string path);
void new_help_directory(string path);
static void load_helpfiles(void);


#define FILES_PER_ITER 1


static void create(varargs int clone) {
  if(clone)
    error("Can't clone helpd!");

  if(!find_object(UNQ_PARSER))
    compile_object(UNQ_PARSER);
  if(!find_object(UNQ_DTD))
    compile_object(UNQ_DTD);

  if(!find_object(SOUNDEXD)) { compile_object(SOUNDEXD); }
  soundex = find_object(SOUNDEXD);

  path_stack = ({ });

  files_to_load = ({ });
  load_callout = -1;

  clear_help_entries();
}

void destructed(int clone) {
  if(help_dtd)
    destruct_object(help_dtd);
}

void upgraded(void) {
  files_to_load = ({ });
  load_callout = -1;

  clear_help_entries();
  rlimits(status()[ST_STACKDEPTH];-1) {
    reread_help_files();
  }
}


/* This function takes a list of strings like that returned by explode()
   and calls explode on the individual members of it.  Used to explode
   wordlist around more than one delimiter. */
private string* reexplode_wordlist(string *wordlist, string delim) {
  string *newwords;
  int     ctr;

  newwords = ({ });
  for(ctr = 0; ctr < sizeof(wordlist); ctr++) {
    if(wordlist[ctr] && strlen(wordlist[ctr])) {
      newwords += explode(wordlist[ctr], delim);
    }
  }

  return newwords;
}

private string normalize_help_query(string query) {
  int     ctr, ctr2;
  string *words, *querylist;
  string  word;

  query = STRINGD->to_lower(STRINGD->trim_whitespace(query));

  words = explode(query, " ");

  /* Re-explode the list around "-" and "_", just like space */
  words = reexplode_wordlist(words, "-");
  words = reexplode_wordlist(words, "_");

  querylist = ({ });

  for(ctr = 0; ctr < sizeof(words); ctr++) {
    if(words[ctr]) {
      word = "";
      for(ctr2 = 0; ctr2 < strlen(words[ctr]); ctr2++) {
	if((words[ctr][ctr2] >= "A"[0] && words[ctr][ctr2] <= "Z"[0])
	   || (words[ctr][ctr2] >= "a"[0] && words[ctr][ctr2] <= "z"[0])
	   || (words[ctr][ctr2] >= "0"[0] && words[ctr][ctr2] <= "9"[0])) {
	  word += words[ctr][ctr2..ctr2];
	}
      }

      if(strlen(word)) {
	querylist += ({ word });
      }
    }
  }

  /* Alphabetize the query list */
  querylist = STRINGD->alpha_sort_list(querylist);

  query = implode(querylist, " ");

  return query;
}

void clear_help_entries(void) {
  int iter;

  num_loc = PHRASED->num_locales();

  exact_entries = allocate(num_loc);
  for(iter = 0; iter < num_loc; iter++) {
    exact_entries[iter] = ([ ]);
  }
  sdx_entries = ([ ]);
}

void reread_help_files(void) {
  int iter;
  int ret;

  for(iter = 0; iter < sizeof(path_stack); iter++) {
    new_help_directory(path_stack[iter]);
    if(iter > 10) {
      error("Error in iteration...\n");
    }
  }
}

/* Customize this for your MUD... */
private int exclude_path(string path) {
  /* Don't include SCCS directories */
  if(sscanf(path, "%*s/SCCS%*s") == 2) {
    return 1;
  }
  /* Don't include RCS directories */
  if(sscanf(path, "%*s/RCS%*s") == 2) {
    return 1;
  }
  /* Don't include CVS directories */
  if(sscanf(path, "%*s/CVS%*s") == 2) {
    return 1;
  }

  return 0;
}

void load_help_dtd(string dtd) {
  if(help_dtd)
    error("Help DTD already exists!");

  help_dtd = clone_object(UNQ_DTD);
  help_dtd->load(dtd);
}

void new_help_file(string path) {
  string  contents, word, arg;
  string* lines;
  int     ctr, argcount, len;
  mixed*  keys;
  mixed*  ent;
  mixed*  exp_arr;
  mixed*  unq_data;
  string err;

  contents = read_file(path);
  if(strlen(contents) > MAX_STRING_SIZE - 3) {
    error("Helpfile " + path + " too long!");
  }

  err = catch (unq_data
	       = UNQ_PARSER->unq_parse_with_dtd(contents, help_dtd, path));

  if (err != nil) {
    LOGD->write_syslog("Helpd got parse error parsing " + path,
		       LOG_ERR);
    error(err);
  }
  if(!unq_data || !typeof(unq_data) == T_ARRAY)
    error("Couldn't load file " + path + " as UNQ helpfile!");

  if(unq_data && typeof(unq_data) == T_ARRAY && unq_data[0] &&
     unq_data[0] != "") {
    load_unq_help(path, unq_data);
    return;
  }

  error("Couldn't load UNQ file " + path);
}

void new_help_directory(string path) {
  mixed** dir;
  int     ctr;
  string left;

  if(previous_program() != HELPD) {
    path_stack += ({ path });
  }

  dir = get_dir(path + "/*");
  left = "";
  for(ctr = 0; ctr < sizeof(dir[0]); ctr++) {
    if(dir[1][ctr] == -2) {
      if(!exclude_path(path + "/" + dir[0][ctr])) {
	new_help_directory(path + "/" + dir[0][ctr]);
      }
    } else if(sscanf(dir[0][ctr], "%*s.hlp%s", left) == 2) {
      if(left == "") {
	files_to_load += ({ path + "/" + dir[0][ctr] });

	if(load_callout < 0) {
#if 0
	  load_callout = call_out("load_helpfiles", 0);
	  if(load_callout < 0)
	    LOGD->write_syslog("Couldn't load all helpfiles?", LOG_ERR);
#else
	  /* Temporary hack to keep from using the call_out loading */
	  load_helpfiles();
#endif
	}
      }
    }
  }
}


/* Note: The mapping (tmp) handles uniqueness of description by
   hashing the descriptions (Phrase LWOs) and taking the map_values.
   The if/for combo actually checks for a match on keywords.  An entry
   matches if all of its keywords are also in kw.  If it has no
   keywords, it matches no matter what kw is. */
private mixed* filter_for_keywords(mixed* entries, mixed* kw) {
  mixed*  ret;
  int     ctr;
  mapping tmp;

  if(!entries) return nil;

  tmp = ([ ]);

  /* To make later checking easier */
  if(!kw) kw = ({ });

  for(ctr = 0; ctr < sizeof(entries); ctr++) {
    if(!tmp[entries[ctr][1]]) {
      /* Not a repeat of an existing entry */

      if(sizeof(kw & entries[ctr][4]) == sizeof(entries[ctr][4])) {
	/* If all keywords in the entry are also in kw, we'll
	   put it into tmp. */
	tmp[entries[ctr][1]] = entries[ctr];
      }
    }
  }

  ret = map_values(tmp);
  return sizeof(ret) ? ret : nil;
}

mixed* query_exact(string key, object user) {
  int locale;
  mixed* ent;

  locale = user->get_locale();

  key = normalize_help_query(key);

  ent = exact_entries[locale][key];
  if(!ent) {
    ent = exact_entries[LANG_englishUS][key];
  }

  return ent;
}

mixed* query_exact_with_keywords(string key, object user, string* kw) {
  return filter_for_keywords(query_exact(key, user), kw);
}

mixed* query_soundex(string sdx_key, object user) {
  int locale;
  locale = user->get_locale();

  if(locale != LANG_englishUS)
    return nil;

  return sdx_entries[sdx_key];
}

mixed* query_soundex_with_keywords(string sdx_key, object user, string* kw) {
  mixed* entries;

  entries = query_soundex(sdx_key, user);
  return filter_for_keywords(entries, kw);
}

private void new_unq_entry(string path, object names, object desc,
			   string keywords) {
  int    locale, len, ctr;
  mixed* keys, arr, kw_arr;
  string sdx;
  object phr;

  if(!names || !desc) {
    error("Required help component(s) missing in file: " + path);
  }

  if(keywords && keywords != "" && !STRINGD->is_whitespace(keywords)) {
    kw_arr = explode(keywords, ",");
    for(ctr = 0; ctr < sizeof(kw_arr); ctr++) {
      kw_arr[ctr] = STRINGD->trim_whitespace(kw_arr[ctr]);

      if(kw_arr[ctr] != "admin") {
	LOGD->write_syslog("Unknown help keyword '" + kw_arr[ctr]
			   + "' loading helpfiles!", LOG_WARN);
      }
    }
  } else {
    kw_arr = ({ });
  }

  /* Trim leading and trailing whitespace from the description */
  desc->trim_whitespace();

  for(locale = 0; locale < num_loc; locale++) {
    string tmp;

    tmp = names->get_content_by_lang(locale);
    if(!tmp || tmp == "")
      continue;

    keys = explode(names->get_content_by_lang(locale), ",");
    len = sizeof(keys);

    for(ctr = 0; ctr < len; ctr++) {
      keys[ctr] = normalize_help_query(keys[ctr]);
      if(STRINGD->string_has_char('\n', keys[ctr])) {
	LOGD->write_syslog("Embedded newline in name(s) of help entry \""
				  + keys[0] + "\"", LOG_ERR);
      }

      /* If no entry, init array */
      if(!exact_entries[locale][keys[ctr]])
	exact_entries[locale][keys[ctr]] = ({ });

      if(locale == LANG_englishUS) {
	/* Generate Soundex key */
	sdx = soundex->get_key(keys[ctr]);
      } else {
	/* If it's not English, don't bother with Soundex */
	sdx = "";
      }

      /* Compose entry */
      arr = ({keys[ctr], desc, path, sdx, kw_arr });

      if(locale == LANG_englishUS) {
	if(!sdx_entries[sdx])
	  sdx_entries[sdx] = ({ });

	/* If it's English, put its Soundex into the sdx_entries array */
	sdx_entries[sdx] += ({ arr });
      }

      /* Put exact entry into specified locale */
      exact_entries[locale][keys[ctr]] += ({ arr });
    }
  }
}

void load_unq_help(string path, mixed* data) {
  int    len, ctr;
  string keywords;
  object names, desc;

  len = sizeof(data);
  if(len % 2)
    error("Odd-sized array passed to load_unq_help (" + path + ")!");

  for(ctr = 0; ctr < len; ctr+=2) {
    if(data[ctr] == "name") {
      if(names) {
	new_unq_entry(path, names, desc, keywords);

	names = desc = nil;
	keywords = nil;
      }

      names = data[ctr + 1];
      if(!names)
	error("Bad name data in " + path);
    } else if(data[ctr] == "desc") {
      if(desc)
	error("Description entry already present in help file " + path);

      desc = data[ctr + 1];
      if(!desc)
	error("Bad description data in " + path);
    } else if(data[ctr] == "keywords") {
      if(keywords)
	error("Keywords entry already present in help file " + path);

      keywords = data[ctr + 1];
      if(!keywords)
	error("Bad keyword data in " + path);
    } else {
      error("Unrecognized tag " + STRINGD->mixed_sprint(data[ctr])
	    + " parsing help file...");
    }
  }

  if(names)
    new_unq_entry(path, names, desc, keywords);
}

static void load_helpfiles(void) {
  int     ctr;
  string* files_this_time;

  files_this_time = files_to_load[..(FILES_PER_ITER-1)];
  files_to_load = files_to_load[FILES_PER_ITER..];

  load_callout = -1;
  if(sizeof(files_to_load) - FILES_PER_ITER > 0) {
    load_callout = call_out("load_helpfiles", 0);
    if(load_callout < 0)
      LOGD->write_syslog("Couldn't schedule call_out for helpfile loading!",
			 LOG_ERR);
  }

  for(ctr = 0; ctr < FILES_PER_ITER; ctr++) {
    new_help_file(files_this_time[ctr]);
  }

}
