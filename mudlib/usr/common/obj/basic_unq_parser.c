#include <config.h>
#include <limits.h>
#include <type.h>
#include <trace.h>

string parser;

static void create(varargs int clone) {
  parser = read_file("/data/parser/unq_parser.dpd", 0, MAX_STRING_SIZE - 1);
  if(strlen(parser) > MAX_STRING_SIZE - 3) {
    error("Parser description too big loading file in unq_parser::create!");
  }
}

private mixed do_backslash_escaping(mixed tmp);

mixed* basic_unq_parse(string block) {
  mixed* tmp;
  catch {
    tmp = parse_string(parser, block);
    /* replace a nil return with a return of an empty array */
    /* -- Tentatively changed this to just log if tmp == nil. */
    if (tmp == nil) {
      /* tmp = ({ }); */
      LOGD->write_syslog("Warning: parse_string() returned nil in basic_unq_parse()");
    }
  } : {
    if (block == nil) {
      LOGD->write_syslog("Error: no block passed to basic_unq_parse");
    } else {
      LOGD->write_syslog("Error parsing block: " + block);
    }
    tmp = nil;
  }

  tmp = do_backslash_escaping(tmp);

  return tmp;
}

mixed* unq_parse_with_dtd(string block, object dtd, varargs string filename) {
  mixed* unq;
  mixed* struct;

  unq = basic_unq_parse(block);
  if(!unq) {
    if(filename) {
      error ("Can't tokenize/match file '" + filename
	     + "' as UNQ in UNQ_PARSER::unq_parse_with_dtd!");
    } else {
      error ("Can't tokenize/match as UNQ in UNQ_PARSER::unq_parse_with_dtd!");
    }
  }

  struct = dtd->parse_to_dtd(unq);
  if(!struct) {
    if(filename) {
      error("*** UNQ from file '" + filename
	    + "' doesn't fit DTD in unq_parse_with_dtd:\n"
	    + dtd->get_parse_error_stack());
    } else {
      error("*** UNQ input doesn't fit DTD in unq_parse_with_dtd:\n"
	    + dtd->get_parse_error_stack());
    }
  }

  return struct;
}

private string backslash_escape_string(string tmp) {
  string ret;
  int    ctr;

  if(!tmp) return nil;

  ret = "";
  for(ctr = 0; ctr < strlen(tmp); ctr++) {
    if(tmp[ctr] == '\\') {
      ctr++;

      /* Backslash at end of input... */
      if(ctr >= strlen(tmp)) {
	ret += "\\";
	break;
      }

      if(tmp[ctr] == '\\'
	 || tmp[ctr] == '~'
	 || tmp[ctr] == '{'
	 || tmp[ctr] == '}') {
	ret += tmp[ctr..ctr];
	continue;
      }

      if(tmp[ctr] == 'n') {
	ret += "\n";
	continue;
      }

      if(tmp[ctr] == 'b') {
	ret += "\b";
	continue;
      }

      if(tmp[ctr] == 't') {
	ret += "\t";
	continue;
      }

      if(tmp[ctr] == 'r') {
	ret += "\r";
	continue;
      }

      if(tmp[ctr] == 'a') {
	ret += "\a";
	continue;
      }

      if(tmp[ctr] == 'f') {
	ret += "\f";
	continue;
      }

      if(tmp[ctr] == 'v') {
	ret += "\v";
	continue;
      }
    } else {
      ret += tmp[ctr..ctr];
    }
  }

  return ret;
}


private mixed do_backslash_escaping(mixed tmp) {
  int ctr;

  if(tmp == nil) return nil;

  if(typeof(tmp) == T_STRING)
    return backslash_escape_string(tmp);

  if(typeof(tmp) == T_ARRAY) {
    for(ctr = 0; ctr < sizeof(tmp); ctr++) {
      tmp[ctr] = do_backslash_escaping(tmp[ctr]);
    }

    return tmp;
  }

  error("Unrecognized type " + typeof(tmp) + " in do_backslash_escaping!");
}


mixed* trim_empty_tags(mixed* unq) {
  mixed* ret;
  int    ctr;

  if(!unq) return unq;
  ret = ({ });

  if(sizeof(unq) % 2)
    error("Odd-sized UNQ array passed to trim_empty_tags: "
	  + STRINGD->mixed_sprint(unq));

  for(ctr = 0; ctr < sizeof(unq); ctr += 2) {
    if((unq[ctr] && !STRINGD->is_whitespace(unq[ctr]))
       || typeof(unq[ctr + 1]) == T_ARRAY
       || !STRINGD->is_whitespace(unq[ctr + 1])) {
      ret += ({ unq[ctr], unq[ctr + 1] });
    }
  }

  return ret;
}



/*** Functions callable by parse_string during parsing... ***/

/* From:
unq_string: substring ? simple_anon_tag
*/
static mixed* simple_anon_tag(mixed* tokens) {
  return ({ nil, tokens[0] });
}

/* From:
unq_tag: '{' unq_string '}' ? anon_tag
*/
static mixed* anon_tag(mixed* tokens) {
  mixed* tag_val;
  int    size;

  size = sizeof(tokens);
  if(size < 3) {
    error("Incorrect-sized array passed to anon_tag!");
  } else if (size == 3) {
    error("Should never happen?");
    return ({ "", tokens[1] });
  } else if (size == 4 && tokens[2] == nil) {
    /* Previous was string returned by simple_anon_tag */
    return ({ "", tokens[2] });
  } else {
    tag_val = tokens[1..(size - 2)];
    return ({ "", tag_val });
  }
}

/* From:
unq_tag: '{' '}' ? empty_anon_tag
*/
static mixed* empty_anon_tag(mixed* tokens) {
  return ({ "", ({ }) });
}

/* From:
unq_tag: '~' regularstring '{' '}' ? empty_named_tag
*/
static mixed* empty_named_tag(mixed* tokens) {
  return ({ tokens[1], ({ }) });
}

/* From:
unq_tag: '~' regularstring '{' unq_string '}' ? named_tag
*/
static mixed* named_tag(mixed* tokens) {
  string tag_name;
  mixed* tag_val;
  int    size;

  size = sizeof(tokens);
  tag_name = tokens[1];

  if(size < 5) {
    error("Incorrect-sized array passed to named_tag!");
  } else if(size == 5) {
    error("Should never happen? [2]");
    return ({ tag_name, tokens[3] });
  } else if (size == 6 && tokens[3] == nil) {
    /* Previous was string returned by simple_anon_tag */
    return ({ tag_name, tokens[4] });
  } else {
    tag_val = tokens[3..(size-2)];
    return ({ tag_name, tag_val });
  }
}

/* From:
substring: substring regularstring ? concat_strings
*/
static mixed* concat_strings(mixed* tokens) {
  int ctr;
  string tmp;

  tmp = "";
  for(ctr = 0; ctr < sizeof(tokens); ctr++) {
    if(typeof(tokens[ctr] == T_STRING)) {
      tmp += tokens[ctr];
    } else {
      error("Unknown type concatenating strings");
    }
  }

  return ({ tmp });
}

/* From:
unq_string: unq_string substring ? concat_ustring_string
*/
static mixed* concat_ustring_string(mixed* tokens) {
  string tmp;
  int len;

  len = sizeof(tokens);
  tmp = tokens[len - 1];

  return tokens[0..(len - 2)] + ({ nil, tmp });
}

static mixed* package_args(mixed* tokens) {
  return ({ tokens });
}
