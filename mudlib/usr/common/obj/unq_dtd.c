#include <config.h>
#include <type.h>
#include <trace.h>
#include <log.h>

#define LOG_FILE ("/usr/common/bob.txt")

private mapping dtd;
private int     is_clone;
private mapping builtins;
private string  accum_error;

private void write_log(string str) {
  /* Uncomment this line to start seeing the write_file debug
     messages in the LOG_FILE above. */
  /* write_file(LOG_FILE, str + "\n"); */
}

static int create(varargs int clone) {
  is_clone = clone;
  if(!find_object(UNQ_PARSER))
    compile_object(UNQ_PARSER);

  dtd = ([ ]);

  if(!is_clone) {
    /* Note: may eventually allow additions to the builtins array
       for user-defined types -- effectively that's what Phantasmal
       does with 'phrase'. */
    builtins = ([ "string" : 1,
		  "int" : 1,
		  "float" : 1,
		  "phrase" : 1 ]);
  }
}

/* Functions for creating UNQ text from DTD template data */
        string serialize_to_dtd(mixed* dtd_unq);
private string serialize_to_dtd_type(string label, mixed unq);
private string serialize_to_string_with_mods(mixed* type, mixed unq);
private string serialize_to_dtd_struct(string label, mixed unq);
private string serialize_to_builtin(string type, mixed unq);

/* Functions for parsing and verifying DTD UNQ input */
        mixed* parse_to_dtd(mixed* unq);
private mixed* parse_to_dtd_type(string label, mixed unq);
private mixed  parse_to_string_with_mods(mixed* type, mixed unq);
private mixed* parse_to_dtd_struct(string label, mixed unq);
private mixed  parse_to_builtin(string type, mixed unq);

/* Functions for loading a DTD */
private void   new_dtd_element(string label, mixed data);
private mixed* dtd_struct(string* array);
private mixed* dtd_string_with_mods(string str);

/* Random helper funcs */
private void set_up_fields_mapping(mapping fields, mixed* type);



/* This function should be called only as UNQ_DTD->is_builtin(...),
   never locally from a clone.  It won't work if called on a clone.
   That's mainly because there's only one builtins mapping, and it's
   stored in a central location.  To save the trouble of making this
   happen later, I'm doing it now -- if we start adding user-defined
   types to builtins, we'll need to have only a single instance of it.
*/
mixed is_builtin(string label) {
  if(is_clone)
    error("Call is_builtin only on shared issue of unq_dtd!");

  return builtins[label];
}

/*************************** Functions for Serializing ***************/

/* This function Takes UNQ input that could have been the result of
   this DTD and converts it to an UNQ string suitable for writing
   to a file.  Returns its error in accum_error, which may be
   queried with get_parse_error_stack. */
string serialize_to_dtd(mixed* unq) {
  string ret, data;

  ret = "";
  accum_error = "";

  if(sizeof(unq) % 2) {
    accum_error += "Odd-sized UNQ chunk passed to serialize_to_dtd!\n";
    return nil;
  }

  while(sizeof(unq) > 0) {
    if(sizeof(unq) == 1) {
      accum_error += "Internal error (odd chunk-size) in serialize_to_dtd!\n";
      return nil;
    }

    if(!unq[0]) {
      accum_error += "Label (nil) passed to serialize_to_dtd!";
      return nil;
    }

    if(dtd[unq[0]]) {
      data = serialize_to_dtd_type(unq[0], unq[1]);
      if(!data) {
	accum_error += "Error on label '" + unq[0] + "'.\n";
	return nil;
      }
      ret += data;
    } else {
      accum_error += "Unrecognized type '" + unq[0] + "'.\n";
      return nil;
    }

    /* Cut off leading UNQ tag and contents */
    unq = unq[2..];
  }

  return ret;
}


/* The label should be passed in as arg1, and the unq (excluding the
   label itself) as arg 2. */
private string serialize_to_dtd_type(string label, mixed unq) {
  string tmp;

  if(dtd[label][0] == "struct") {
    return serialize_to_dtd_struct(label, unq);
  }

  tmp = serialize_to_string_with_mods(dtd[label], unq);
  if(tmp && !UNQ_DTD->is_builtin(label)) {
    tmp = "~" + label + "{" + tmp + "}\n";
  }
  if(!tmp) {
    accum_error += "Couldn't serialize as type " + implode(dtd[label], "")
      + "\n";
    return nil;
  }
  return tmp;
}


/* This serializes the given type, but doesn't wrap it in its appropriate
   label.  The caller will need to do that, if appropriate. */
private string serialize_to_string_with_mods(mixed* type, mixed unq) {
  string ret;
  int    ctr, is_struct;

  if(sizeof(type) != 1 && sizeof(type) != 2)
    error("Illegal type given to serialize_to_string_with_mods!");

  if(sizeof(type) == 1) {
    if(UNQ_DTD->is_builtin(type[0]))
      return serialize_to_builtin(type[0], unq);

    if(dtd[type[0]])
      return serialize_to_dtd_type(type[0], unq);

    accum_error += "Unrecognized type '" + type[0]
      + "' in serialize_to_string_with_mods!\n";
    return nil;
  }

  /* Sizeof(type) == 2, so it's a type/mod combo */
  if(type[1] != "?" && type[1] != "+" && type[1] != "*") {
    accum_error += "Unrecognized type modifier " + type[1] + "!\n";
    return nil;
  }

  if(typeof(unq) == T_STRING) {
    /* TODO:  type checking here */
    return unq;
  }
  if(typeof(unq) != T_ARRAY) {
    accum_error += "Unreasonable type serializing UNQ as "
      + implode(type, ",") + "!\n";
    return nil;
  }

  /* Multiple entries */
  /* First, check to see that the number of entries is reasonable */
  if(type[1] == "+" && sizeof(unq) < 1) {
    accum_error += "Number of entries doesn't fit + mod\n";
    return nil;
  }
  if(type[1] == "?" && sizeof(unq) > 1) {
    accum_error += "Number of entries doesn't fit ? mod\n";
    return nil;
  }
  /* The "*" modifier doesn't need a check, any number's fine */

  /* Set is_struct flag for below... */
  if(UNQ_DTD->is_builtin(type[0])) {
    is_struct = 0;
  } else if(dtd[type[0]][0] == "struct") {
    is_struct = 1;
  } else {
    accum_error += "Type '" + type[0]
      + "' doesn't seem to be struct or builtin!\n";
    return nil;
  }

  ret = "";
  for(ctr = 0; ctr < sizeof(unq); ctr+=2) {
    string tmp;

    if(is_struct)
      tmp = serialize_to_dtd_struct(type[0], unq[ctr + 1]);
    else
      tmp = serialize_to_builtin(type[0], unq[ctr + 1]);

    if(tmp == nil)
      return nil;

    ret += tmp;
  }

  return ret;
}


/* The label should be passed in as arg1, and the unq (excluding the
   label itself) as arg 2. */
private string serialize_to_dtd_struct(string label, mixed unq) {
  string  ret;
  mixed*  type;
  mapping fields;
  int     ctr;

  ret = "~" + label + "{";
  type = dtd[label];
  if(!type || type[0] != "struct") {
    accum_error += "Non-struct passed to serialize_to_dtd_struct!\n";
    return nil;
  }

  /* Rather than an instance tracker, we're just going to serialize
     the fields in the order given.  That puts some constraints on
     the UNQ that gets passed in, but that's fine for now.  Like
     so much of this code, it'll be expanded later if we need the
     new functionality. */

  fields = ([ ]);
  set_up_fields_mapping(fields, type);

  if(typeof(unq) != T_ARRAY) {
    string tmp;

    if(sizeof(type) != 2
       || !UNQ_DTD->is_builtin(type[1][0])) {
      accum_error += "Can't serialize struct '" + label + "' from '"
	+ unq + "' (type " + typeof(unq) + ").\n";
      return nil;
    }

    tmp = serialize_to_string_with_mods(type[1], unq);
    if(tmp) {
      /* serialize_to_string_with_mods will supply its own curly
	 braces. */
      return "~" + label + tmp + "\n";
    }
    return nil;
  }

  for(ctr = 0; ctr < sizeof(unq); ctr += 2) {
    string tmp;

    if(!fields[unq[ctr]]) {
      accum_error += "Unrecognized field '" + unq[ctr] + "'.\n";
      return nil;
    }

    tmp = serialize_to_dtd_type(unq[ctr], unq[ctr + 1]);
    if(!tmp) {
      accum_error += "Error writing label '" + unq[ctr] + "' of struct.\n";
      return nil;
    }
    ret += "~" + unq[ctr] + tmp + "\n";
  }

  ret += "}";
  return ret;
}


private string serialize_to_builtin(string type, mixed unq) {
  if(type == "string") {
    if(typeof(unq) != T_STRING) {
      accum_error += "Type " + typeof(unq) + " is not a string!\n";
      return nil;
    }
    return "{" + unq + "}";
  }

  if(type == "int") {
    if(typeof(unq) != T_INT) {
      accum_error += "Type " + typeof(unq) + " is not an int!\n";
      return nil;
    }
    return "{" + unq + "}";
  }

  if(type == "float") {
    if(typeof(unq) != T_FLOAT) {
      accum_error += "Type " + typeof(unq) + " is not a float!\n";
    }
    return "{" + unq + "}";
  }

  if(type == "phrase") {
    if(typeof(unq) == T_STRING) {
      return "{" + unq + "}";
    }
    return "{" + unq->to_unq_text() + "}";
  }

  accum_error += "Don't recognize type " + type + " serializing builtins!\n";
  return nil;
}

/*************************** Functions for Parsing *******************/

/* Takes arbitrary parsed UNQ input and makes it conform to this DTD
   if possible.  Returns errors in accum_error, which may be queried
   with get_parse_error_stack. */
mixed* parse_to_dtd(mixed* unq) {
  int    ctr;
  string label;
  mixed* data, ret;

  accum_error = "";

  ret = ({ });
  unq = UNQ_PARSER->trim_empty_tags(unq);

  write_log("============================================================");
  write_log("Parsing to DTD: '" + STRINGD->mixed_sprint(unq) + "'\n-----");

  for(ctr = 0; ctr < sizeof(unq); ctr+=2) {
    label = STRINGD->trim_whitespace(unq[ctr]);
    if(dtd[label]) {
      data = parse_to_dtd_type(label, unq[ctr + 1]);
      if(data == nil) {
	accum_error += "Mismatch on label '" + label + "'\n";
	return nil;
      }
      write_log("Finished parsing type '" + label + "'.");
      ret += data;
      continue;
    }

    error("Unrecognized...");
  }

  return ret;
}

string get_parse_error_stack(void) {
  return accum_error;
}

/* Label is an entry in the DTD, unq is a chunk of input to parse
   assuming it conforms to that label.

   Returns a labelled array.
*/
private mixed* parse_to_dtd_type(string label, mixed unq) {
  mixed* type;
  mixed  tmp;

  if(typeof(unq) == T_ARRAY) {
    unq = UNQ_PARSER->trim_empty_tags(unq);
  }

  write_log("Parsing to DTD type '" + label + "'");

  type = dtd[label];
  if(typeof(type) != T_ARRAY && typeof(type) != T_STRING)
    error("Invalid type in DTD in parse_to_dtd_type!");

  /* If it's a struct */
  if(type[0] == "struct") {
    return parse_to_dtd_struct(label, unq);
  }

  /* Else, not a struct. */

  tmp = parse_to_string_with_mods(type, unq);
  if(tmp == nil) {
    accum_error += "Couldn't parse " + STRINGD->mixed_sprint(unq)
      + " as type " + implode(type,"") + "\n";
    return nil;
  }
  return ({ label, tmp });
}

/* Returns a typed chunk or (with mods) an array of typed chunks
   fitting the builtin or builtin with modifier.  The item returned
   will not be prefixed with the appropriate label.  If desired,
   that may be done by the caller. */
private mixed parse_to_string_with_mods(mixed* type, mixed unq) {
  if(sizeof(type) != 1 && sizeof(type) != 2)
    error("Illegal type given to parse_to_string_with_mods!");

  if(unq == nil)
    return nil;

  if(sizeof(type) == 1) {
    if(UNQ_DTD->is_builtin(type[0]))
      return parse_to_builtin(type[0], unq);

    if(dtd[type[0]])
      return parse_to_dtd_type(type[0], unq);

    accum_error += "Unrecognized type '" + type[0]
      + "' in parse_to_string_with_mods!\n";
    return nil;
  }

  /* Sizeof(type) == 2, so it's a type/mod combo */
  if(type[1] != "?" && type[1] != "+" && type[1] != "*") {
    accum_error += "Unrecognized type modifier " + type[1] + "!\n";
    return nil;
  }

  if(UNQ_DTD->is_builtin(type[0])) {
    mixed* ret;
    int    ctr;
    mixed  tmp;

    if(typeof(unq) == T_STRING) {
      tmp = parse_to_builtin(type[0], unq);
      if(tmp == nil) return nil;

      /* Only one obj, but since this has modifiers return it as
	 an array-of-one. */
      return ({ tmp });
    }

    if(typeof(unq) != T_ARRAY)
      error("Unreasonable type parsing UNQ!");

    /* Okay, multiple entries -- typeof(unq) is T_ARRAY */
    unq = UNQ_PARSER->trim_empty_tags(unq);
    ret = ({ });
    for(ctr = 0; ctr < sizeof(unq); ctr+=2) {
      mixed tmp;

      if(!STRINGD->is_whitespace(unq[ctr])) {
	accum_error += "Labelled data found parsing "
	  + type[0] + type[1] + "!\n";
	return nil;
      }

      tmp = parse_to_builtin(type[0], unq[ctr + 1]);
      if(tmp == nil) return nil;

      ret += ({ tmp });
    }

    /* "?" support 0 or 1 instance */
    if(type[1] == "?" && sizeof(ret) > 1) {
      accum_error += "Number of entries doesn't fit ? mod\n";
      return nil;
    }

    /* "+" supports 1 or more */
    if(type[1] == "+" && sizeof(ret) < 1) {
      accum_error += "Number of entries doesn't fit + mod\n";
      return nil;
    }

    /* "*" doesn't even need a check - any number's fine */

    return ret;
  }

  accum_error
    += "Don't yet support modifier characters on non-builtin types!\n";
  return nil;
}

/* Returns a typed chunk of data which is a string, an int, etc as
   appropriate.  The returned chunk will not be prefixed with a
   label. */
private mixed parse_to_builtin(string type, mixed unq) {
  if(!UNQ_DTD->is_builtin(type))
    error("Type " + type + " isn't builtin in parse_to_builtin!");

  /* Nil won't parse as anything -- save some checking code below */
  if(unq == nil)
    return nil;

  if(type == "string") {
    if(typeof(unq) != T_STRING) {
      accum_error += "Type " + typeof(unq) + " is not a string\n";
      return nil;
    }
    return STRINGD->trim_whitespace(unq);
  }

  if(type == "int") {
    int val;

    if(typeof(unq) != T_STRING) {
      accum_error += "Type " + typeof(unq) + " is not a string\n";
      return nil;
    }
    unq = STRINGD->trim_whitespace(unq);
    if(!sscanf(unq, "%d", val)) {
      accum_error += unq + " is not an integer.\n";
      return nil;
    }

    return val;
  }

  if(type == "float") {
    error("Type float not yet implemented");
  }

  if(type == "phrase") {
    object tmp;

    if(typeof(unq) == T_STRING)
      return PHRASED->new_simple_english_phrase(unq);

    if(typeof(unq) != T_ARRAY)
      error("Don't recognized parsed UNQ object in parse_to_builtin(phrase)!");

    catch {
      tmp = PHRASED->unq_to_phrase(unq);
    } : {
      accum_error += call_trace()[1][TRACE_FIRSTARG][1];
      return nil;
    }
    return tmp;
  }

  error("Builtins array modified without modifying parse_to_builtin!");
}

/* Parse_to_dtd_struct assumes some input preprocessing -- label is
   whitespace-trimmed and unq's top level is empty-tag-trimmed.  Label
   is also validated to point to an UNQ DTD structure.  UNQ may or may
   not be a valid structure, but has already had the tag corresponding
   to label trimmed from it.

   Returns an array consisting of a label followed by an array
   of labelled fields.
*/
private mixed* parse_to_dtd_struct(string t_label, mixed unq) {
  mixed*  type, *instance_tracker, *ret;
  mixed   tmp;
  int     ctr;
  mapping fields;
  string  label;

  write_log("Parsing to dtd struct '" + t_label + "', arg: '"
	    + STRINGD->mixed_sprint(unq) + "'");

  type = dtd[t_label];
  if(type[0] != "struct")
    error("Non-struct passed to parse_to_dtd_struct!");

  if(typeof(unq) == T_STRING
     || (sizeof(type) == 2
	 && UNQ_DTD->is_builtin(type[1][0]))) {
    /* Hrm.  The struct had better consist of a single built-in field
       in this case. */
    if(sizeof(type) > 2) {
      accum_error += "The string '" + unq
	+ "' cannot be parsed as multiple fields for struct '" + t_label
	+ "'.\n";
      return nil;
    }

    if(!UNQ_DTD->is_builtin(type[1][0])) {
      accum_error += "The string '" + unq
	+ "' cannot be parsed as non-builtin field '" + type[1][0]
	+ "' of DTD struct '" + t_label + "'\n";
      return nil;
    }

    tmp = parse_to_string_with_mods(type[1], unq);
    if(tmp == nil) return nil;

    return ({ t_label, tmp });
  }

  /* We need to track how many of each tag in the structure are parsed
     so we can check to make sure they match our modifiers or lack
     thereof.  We have one bucket for each type in the struct.  We
     leave in the spare bucket for the word "struct" to simplify
     indexing. */
  instance_tracker = allocate(sizeof(type));

  for(ctr = 0; ctr < sizeof(instance_tracker); ctr++) {
    instance_tracker[ctr] = ({ });
  }

  /* Set up fields array to know what bucket of instance_tracker
     different labelled fields belong in */ 
  fields = ([ ]);
  set_up_fields_mapping(fields, type);

  for(ctr = 0; ctr < sizeof(unq); ctr += 2) {
    int   index;
    mixed tmp;

    label = STRINGD->trim_whitespace(unq[ctr]);
    if(fields[label] == nil) {
      accum_error += "Unrecognized field " + label + " in structure\n";
      return nil;
    }
    index = fields[label];

    tmp = parse_to_dtd_type(label, unq[ctr + 1]);
    if(tmp == nil) {
      accum_error += "Error parsing label '" + label + "' of structure.\n";
      return nil;
    }
    instance_tracker[index] += ({ tmp });
    if(instance_tracker[index] == nil)
      return nil;

  }

  /* Verify we match the modifiers */
  for(ctr = 1; ctr < sizeof(type); ctr++) {
    int num;

    /* If no modifier... */
    if(typeof(type[ctr]) == T_STRING
       || sizeof(type[ctr]) == 1) {
      if(sizeof(instance_tracker[ctr]) != 1) {
	accum_error += "Wrong # of instances of " + type[ctr] + "\n";
	return nil;
      }
      continue;
    }

    if(type[ctr][1] == "*")
      continue;  /* Star modifier allows any number of insts */

    num = sizeof(instance_tracker[ctr]);
    if((type[ctr][1] == "?") && num > 1) {
      accum_error += "Wrong # of fields of type " + type[ctr] + " in struct\n";
      return nil;
    }
    if((type[ctr][1] == "+") && num == 0) {
      accum_error += "Wrong # of fields of type " + type[ctr] + " in struct\n";
      return nil;
    }
  }

  /* Reassemble instances into single array to return */
  ret = ({ });
  for(ctr = 1; ctr < sizeof(instance_tracker); ctr++) {
    ret += instance_tracker[ctr];
  }
  return ({ t_label, ret });
}


/*** To load in the DTD: ***/

void load(string new_dtd) {
  int    ctr;
  string str;
  mixed* new_unq;

  if(!is_clone)
    error("Can't use non-clone UNQ DTD!  Stop it!");

  new_unq = UNQ_PARSER->basic_unq_parse(new_dtd);

  if(!new_unq)
    error("Can't parse UNQ data passed to unq_dtd:load!");

  new_unq = UNQ_PARSER->trim_empty_tags(new_unq);

  str = STRINGD->trim_whitespace(new_unq[0]);
  if(STRINGD->stricmp(str, "dtd")) {
    error("Can't load file as DTD -- not tagged as DTD!");
  }

  if(typeof(new_unq[1]) != T_ARRAY)
    error("This doesn't look complex enough to be a real DTD!");

  if(sizeof(new_unq) > 2) {
    error("Don't yet support multiple DTDs per file: "
	  + STRINGD->mixed_sprint(new_unq));
  }

  new_unq[1] = UNQ_PARSER->trim_empty_tags(new_unq[1]);
  for(ctr = 0; ctr < sizeof(new_unq[1]); ctr+=2) {
    new_dtd_element(new_unq[1][ctr], new_unq[1][ctr + 1]);
  }

}

private void new_dtd_element(string label, mixed data) {
  string* tmp_arr;

  label = STRINGD->trim_whitespace(label);
  if(dtd[label] || label == "struct" || UNQ_DTD->is_builtin(label))
    error("Redefining label " + label + " in UNQ DTD!");

  if(typeof(data) == T_STRING) {
    data = STRINGD->trim_whitespace(data);

    tmp_arr = explode(data, ",");
    dtd[label] = dtd_struct(tmp_arr);

    return;
  } else if (typeof(data) == T_ARRAY) {
    error("complex subtypes not yet supported!");
  } else {
    error("Type error -- problem with UNQ parser?");
  }
}

private mixed* dtd_string_with_mods(string str) {
  string mod;

  str = STRINGD->trim_whitespace(str);
  if(str == nil)
    error("Nil passed to dtd_string_with_mods!");

  if(STRINGD->is_alpha(str))
    return ({ str });

  mod = str[strlen(str)-1..strlen(str)-1];
  str = str[..strlen(str)-2];
  if(mod == "?" || mod == "*" || mod == "+")
    return ({ str, mod });

  error("Don't recognize UNQ type " + str);
}

private mixed* dtd_struct(string* array) {
  mixed* ret, *tmp;
  int    ctr;

  ret = ({ "struct" });

  for(ctr = 0; ctr < sizeof(array); ctr++) {
    array[ctr] = STRINGD->trim_whitespace(array[ctr]);
    tmp = dtd_string_with_mods(array[ctr]);
    ret += ({ tmp });
  }

  return ret;
}


void clear(void) {
  dtd = ([ ]);
}


/****************** Helper funcs ***************************/

private void set_up_fields_mapping(mapping fields, mixed* type) {
  int ctr;

  /* Set up fields array to know what bucket of instance_tracker
     different labels belong in */
  for(ctr = 1; ctr < sizeof(type); ctr++) {
    if(typeof(type[ctr]) == T_STRING) {
      fields[type[ctr]] = ctr;
    } else if(typeof(type[ctr]) == T_ARRAY) {
      fields[type[ctr][0]] = ctr;
    } else
      error("Unknown type in DTD struct type!");
  }
}
