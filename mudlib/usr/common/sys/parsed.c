/*
 * Command parser for parsing user input
 * Also does the work of the binder (as described in From the Dawn of Time
 * Skotos.net articles)
 */

#include "config.h"

/* code for enabling super-verbose logging */
#ifdef LOGGING
#define LOG(x) write_file("~/parser.log", x);
#else
#define LOG(x)
#endif

string grammar;

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
