#include <config.h>
#include <phrase.h>
#include <limits.h>
#include <type.h>
#include <log.h>

/********************* Loading phrases from files **********************/

private mapping phrasepaths;

/* Local prototypes */
mapping load_filemanaged_file(string path);
object  get_filemanaged_phrase(string path, string ident);
void    save_filemanaged_file(string path);
void    new_filemanaged_file(string path);


static void create(varargs int clone) {
  if(!find_object(UNQ_PARSER))
    compile_object(UNQ_PARSER);

  phrasepaths = ([ ]);
}

void upgraded(varargs int clone) {
  mixed *files;
  int    ctr, any_failures;

  any_failures = 0;
  files = map_indices(phrasepaths);
  phrasepaths = ([ ]);
  for(ctr = 0; ctr < sizeof(files); ctr++) {
    if(!load_filemanaged_file(files[ctr]))
      any_failures = 1;
  }
  if(any_failures) {
    LOGD->write_syslog("Can't reload all files in phrase repository "
		       + object_name(this_object()), LOG_ERR);
  }
}

object file_phrase(string path, string ident) {
  mapping file;

  file = phrasepaths[path];
  if(!file) {
    /* Load phrases from file */
    file = load_filemanaged_file(path);
    if(!file) {
      LOGD->write_syslog("Requested phrase from nonexistent file " + path,
			 LOG_WARN);
      return nil;
    }
  }
  if(file[ident]) {
    return file[ident];
  }

  LOGD->write_syslog("Couldn't find phrase " + ident + " in file.",
		     LOG_WARN);
  return nil;
}

mapping load_filemanaged_file(string path) {
  string  contents;
  mixed*  unq_data;
  mapping filemap;
  int     iter;
  object  phrase;

  /* Even if MAX_STRING_SIZE is obsolete, this'll let us limit the size
     of the file we read, which is a good thing. */
  contents = read_file(path, 0, MAX_STRING_SIZE - 1);
  if(strlen(contents) > MAX_STRING_SIZE - 3) {
    /* File is too long... */
    LOGD->write_syslog("File " + path
		       + " is too large to load as a phrase file!",
		       LOG_ERR);
    return nil;
  }

  /* Load as UNQ */
  unq_data = UNQ_PARSER->basic_unq_parse(contents);
  if(!unq_data) {
    LOGD->write_syslog("Can't parse filemanaged phrase file!", LOG_ERR);
    return nil;
  }
  if(sizeof(unq_data) == 0) return nil;
  if(sizeof(unq_data) % 2) return nil;  /* Error - odd sized array */

  iter = 0;
  filemap = ([ ]);
  while(iter < sizeof(unq_data)) {
    if(!unq_data[iter] || unq_data[iter] == "") {
      if(STRINGD->is_whitespace(unq_data[iter + 1])) {
	/* Anonymous, top-level whitespace -- skip it */
	iter += 2;
	continue;
      }
      LOGD->write_syslog("Anonymous top-level data parsing UNQ!", LOG_ERR);
      return nil;
    }

    if(typeof(unq_data[iter + 1]) == T_STRING) {
      phrase = PHRASED->new_simple_english_phrase(unq_data[iter + 1]);
    } else if(typeof(unq_data[iter + 1]) == T_ARRAY) {
      phrase = PHRASED->unq_to_phrase(unq_data[iter + 1]);

      if(!phrase) {
	LOGD->write_syslog("Null phrase from unq_to_phrase!", LOG_ERR);
	return nil;
      }
    } else {
      LOGD->write_syslog("Unrecognized data type " + typeof(unq_data[iter + 1])
			 + " returned by UNQ", LOG_ERR);
      return nil;
    }
    if(filemap[unq_data[iter]]) {
      LOGD->write_syslog("Repeated label " + unq_data[iter] + " in file "
			 + path + ".  Ignoring.");
    } else {
      filemap[unq_data[iter]] = phrase;
    }
    iter += 2;
  }

  phrasepaths[path] = filemap;

  return filemap;
}
