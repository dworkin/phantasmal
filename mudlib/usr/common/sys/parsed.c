/*
 * Command parser for parsing user input
 * Also does the work of the binder (as described in From the Dawn of Time
 * Skotos.net articles)
 */

#include <limits.h>
#include "config.h"

/* code for enabling super-verbose logging */
#ifdef LOGGING
#define LOG(x) write_file("~/parser.log", x);
#else
#define LOG(x)
#endif

string grammar;
mapping ones;
mapping tens;

static void upgraded(varargs int clone);

static void create(varargs int clone) {
  if (clone) {
    error("Cloning not allowed!");
  }

  upgraded();
}

static void upgraded(varargs int clone) {
  grammar = read_file(NL_PARSE_FILE);
 
  if (grammar == nil) {
    error("Error reading grammar from file " + NL_PARSE_FILE);
  }

  grammar = "whitespace = /[ \r\n\t\b\\,]+/\n" + grammar;

  ones = ([ "zero" : 0, "one" : 1,  "two" : 2, "three" : 3, "four" : 4, 
	  "five" : 5, "six" : 6, "seven" : 7, "eight" : 8, "nine" : 9 ]);
  tens = ([ "ten" : 10, "eleven" : 11, "twelve" : 12, "thirteen" : 13,
	  "fourteen" : 14, "fifteen" : 15, "sixteen" : 16, "seventeen": 17,
	  "eighteen" : 18, "nineteen" : 19, "twenty" : 20, "thirty" : 30,
          "fourty" : 40, "fifty" : 50, "sixty" : 60, "seventy" : 70,
          "eighty" : 80, "ninety" : 90 ]);

  LOG("Loaded grammar:\n");
  LOG(grammar);
  LOG("\n");
}

/* function for parsing commands */
mixed *parse_cmd(string cmd){
  LOG("Parsing string: " + cmd + "$\n");
  return parse_string(grammar, cmd);
}

/* function for binding noun phrases */
mixed *bind_noun(mixed *phrase, int how) {
  error("Binding nouns not yet implemented.");
  return nil;
}

private string bind_verb(mixed *phrase) {
  error("Binding verbs not yet implemented.");
  return nil;
}


/* functions for converting numbers strings (as parsed) into numbers */
static mixed *dig2num(mixed *token){
  return ({ (int)token[0] });
}

static mixed *cat_num(mixed *token) {
  return ({ (token[0]*(int)pow(10.0, (float)strlen(token[1]))) + (int)token[1] });
}

static mixed *num1(mixed *token) {
  return ({ 1 });
}

static mixed *num0(mixed *token) {
  return ({ 0 });
}

static mixed *all_but_num(mixed *token) {
  return ({ - token[0] });
}

static mixed *num_all(mixed *token) {
  return ({ INT_MAX });
}

static mixed *one2num(mixed *token) {
  return ({ ones[token[0]] });
}

static mixed *ten2num(mixed *token) {
  int size;
  size = sizeof(token);
  if (size == 1) {
    return ({ tens[token[0]] });
  } else {
    return ({ tens[token[0]] + token[size - 1] });
  }
}

static mixed *hun2num(mixed *token) {
  int size;
  size = sizeof(token);
  if (size == 2) {
    return ({ token[0]*100 });
  } else {
    return ({ token[0]*100 + token[size-1] });
  }
}

static mixed *thou2num(mixed *token) {
  int size;
  size = sizeof(token);
 
  if (token[0] >= 1000) {
    return nil;
  }

  if (size == 2) {
    return ({ token[0]*1000 });
  } else {
    return ({ token[0]*1000 + token[size-1] });
  }
}
