/*
 * Command parser for parsing user input
 * Also does the work of the binder (as described in From the Dawn of Time
 * Skotos.net articles)
 */

#include <phantasmal/parser.h>

#include <config.h>
#include <limits.h>
#include <type.h>

/* code for enabling super-verbose logging */
#ifdef LOGGING
#define LOG(x) write_file("~/parser.log", x);
#else
#define LOG(x)
#endif

/* number of ambiguities to keep */
#define NUM_KEEP 5

string grammar;
mapping ones;
mapping tens;

static void upgraded(varargs int clone);
static string uncomment_file(string file);

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
          "forty" : 40, "fourty" : 40, "fifty" : 50, "sixty" : 60,
	  "seventy" : 70, "eighty" : 80, "ninety" : 90 ]);

  grammar = uncomment_file(grammar);

  LOG("Loaded grammar:\n");
  LOG(grammar);
  LOG("\n");
}

/* Function for removing comments from the grammar file.  A comment is
 * anything which starts with the pound ('#') sign.
 */

static string uncomment_file(string file) {
  string *lines;
  string result;
  int i, j;

  result = "";

  lines = explode(file, "\n");
  for(i = 0; i < sizeof(lines); ++i) {
    if (strlen(lines[i]) > 0 && lines[i][0] != '#') {
      result += lines[i] + "\n";
    }
  }

  return result;
}

/* function for parsing commands */
mixed *parse_cmd(string cmd){
  LOG("Parsing string: " + cmd + "$\n");
  return parse_string(grammar, cmd, NUM_KEEP);
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
  return ({ (token[0] * (int)pow(10.0,
				 (float)strlen(token[1]))) + (int)token[1] });
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

/* Functions for "and" and "or" joins */
static mixed *and_join(mixed *token) {
  return ({ ({ JOIN_AND, token[0], token[2] }) });
}

static mixed *sub_join(mixed *token) {
  return ({ ({ JOIN_SUB, token[0], token[2] }) });
}

/* or_join -- this phrase should be interpreted one or the other way, but not
 * both.  Not sure that this function is useful, should be able to deal with
 * this, since this is how parse_string() will represent ambiguities 
 * (I think).
 */
static mixed *or_join(mixed *token) {
  return ({ ({ ({ token[0] }) , ({ token[2] }) }) });
}

/* noun depthifying functions */
/* noun phrases have the following format:
 * ({ PHR_NOUN, <descriptor>, <number>, <owner>, <adj1>, <adj2>, ..., <adjN> })
 * 
 * In addition there are the following special nouns:
 * Everything: ({ PHR_NOUN, "all", INT_MAX, nil })
 * Pronouns:
 *    He: ({ PHR_NOUN, "he", 1, nil })
 *    She: ({ PHR_NOUN, "she", 1, nil })
 *    It: ({ PHR_NOUN, "it", 1, nil })
 */
static mixed* noun_all(mixed *token) {
  return ({ ({ PHR_NOUN, "all", INT_MAX, nil }) });
}

static mixed *noun(mixed *token) {
  /* very last token is the noun -- all previous tokens are adjectives */
  return ({ ({ PHR_NOUN, token[sizeof(token)-1], 1, nil }) + token[0..sizeof(token)-2] });
}

static mixed *noun_pronoun(mixed *token) {
  return ({ ({ PHR_NOUN, token[0], 1, nil }) });
}

static mixed *noun_repeat(mixed *token) {
  return ({ ({ PHR_NOUN, token[sizeof(token)-1], token[0], nil }) 
	      + token[1..sizeof(token)-2] });
}

static mixed *noun_owned(mixed *token) {
  return ({ ({ PHR_NOUN, token[sizeof(token)-1], 1, token[0] }) 
	      + token[2..sizeof(token)-2] });
}

static mixed *noun_owned_repeat(mixed *token) {
  mixed *noun_phrase;
  /* toklen is the number of adj/owner/num tokens (exclues the noun). */
  int i, toklen;
  
  /* set up the noun phrase.  Assume last word is the noun, but don't know
   * much else about the noun phrase yet, so put in bogus values.
   */
  toklen = sizeof(token) - 1;
  noun_phrase = ({ PHR_NOUN, token[toklen], 1, nil });

  for (i = 0; i < toklen; ++i) {
    switch (typeof(token[i])) {
    case T_STRING:
      /* adjective, add to end of noun phrase */
      noun_phrase += ({ token[i] });
      break;
    case T_INT:
      /* number of objects */
      noun_phrase[NPR_NUMBER] = token[i];
      break;
    case T_ARRAY:
      if (typeof(token[i][NPR_TYPE]) == T_INT && token[i][NPR_TYPE] == PHR_NOUN) {
	/* this token is the owner */
	noun_phrase[NPR_OWNER] = token[i];

	/* kill the next token, this indicates possesive */
	++i;
      } else {
	LOGD->write_syslog("Got a phrase I'm not quite sure what to do with.\r\n");
	LOGD->write_syslog(STRINGD->mixed_sprint(token[i]) + "\r\n");
      }
      break;
    default:
      error("Unexpected type of argument in noun_owned_repeat()");
    }
  }

  return noun_phrase;
}

/* function for handing prepositional phrases */
/* all prepositional phrases are preposition, noun phrase */

static mixed *prep(mixed *tokens) {
  return ({ ({ PHR_PREP, tokens[0], tokens[1] }) });
}

/* function for handling verb phrases */
/* all verb phrases are a verb, followed by one or more noun or prepositional
 * phrase */

static mixed *verb(mixed *tokens) {
  return ({ ({ PHR_VERB }) + tokens });
}

/* function for parsing to an adjective phrase */
static mixed *adject(mixed *tokens) {
  int i;
  mixed *phr;
  mixed noun;

  phr = ({ });
  noun = tokens[sizeof(tokens) - 1];

  for (i = sizeof(tokens); i-- > 0; ) {
    switch(typeof(tokens[i])) {
    case T_STRING:
      if (tokens[i] == "of") { 
	--i;
	/* the noun is the string right before the "of" */
	/* This code is dead at the moment, pending better handling of "of"
	 * type constructs */
	noun = tokens[i];
	break;
      } else {
	phr = ({ tokens[i] }) + phr;
      }
      break;
    case T_ARRAY:
      if (tokens[i][NPR_TYPE] == ADJ_NPR) {
	tokens[i][NPR_TYPE] = ADJ_NPR;
      }
      phr = ({ tokens[i] }) + phr;
      break;
    }
  }

  /* add the noun to the end of the phrase, if it's not already there */
  if (phr[sizeof(phr)-1] != noun) {
    phr += ({ noun });
  }

  return phr;
}


