/*
 * First-line command parser for parsing user input
 */

#include <kernel/kernel.h>

#include <phantasmal/parser.h>
#include <phantasmal/lpc_names.h>

#include <limits.h>
#include <type.h>

/* #define LOGGING */

/* Parser concatenation rules are stuck into another file so that this
   one is more readable. */
inherit concat "/usr/common/lib/parsed/ps_rules";
inherit reg    "/usr/common/lib/parsed/register";

/* code for enabling super-verbose logging */
#ifdef LOGGING
#define LOG(x) write_file("~/parser.log", x);
#else
#define LOG(x)
#endif

/* Where to find parser files */
#define NL_TOKEN_FILE    "/usr/common/sys/nl_tokens.dpd"
#define NL_GRAMMAR_FILE  "/usr/common/sys/nl_parser.dpd"

/* number of ambiguities to keep */
#define NUM_AMBIGUOUS 5

string grammar_file;
string token_file;
string grammar;

void upgraded(varargs int clone);
static string uncomment_file(string file);


static void create(varargs int clone) {
  concat::create();
  reg::create();
  upgraded();
}

void upgraded(varargs int clone) {
  /* Expect call only from self and ObjectD */
  if(!SYSTEM() && !COMMON())
    return;

  concat::upgraded(clone);
  reg::upgraded(clone);

  token_file = read_file(NL_TOKEN_FILE);
  grammar_file = read_file(NL_GRAMMAR_FILE);

  if (grammar_file == nil) {
    error("Error reading grammar from file " + NL_GRAMMAR_FILE);
  }
  if (token_file == nil) {
    error("Error reading token grammar from file " + NL_TOKEN_FILE);
  }

  LOG("Loaded token file:\n");
  LOG(token_file);
  LOG("\n");
  LOG("******************************************************\n");
  LOG("Loaded grammar file:\n");
  LOG(grammar_file);
  LOG("\n");
}


private string** divide_into_lines(string *words, int linelen, int divchar) {
  string **ret;
  string  *line;
  int      curlen, ctr;

  ret = ({ });
  ctr = 0;
  while(sizeof(words) > ctr) {
    curlen = 0;
    line = ({ });

    do {
      /* Move a word into 'line' */
      line += ({ words[ctr] });
      curlen += strlen(words[ctr]) + divchar;
      ctr++;
    } while((curlen < linelen) && (sizeof(words) > ctr));

    ret += ({ line });
  }

  return ret;
}


/* This function writes the part-of-speech token grammar to a string
   and returns it.  This depends on things like the list of nouns and
   adjectives that are currently registered. */
private string* pos_grammars(void) {
  string*  words;
  string** linelist;
  string   output, nontoken;
  int      ctr, line;
  int*     cat_list;
  mapping* categories, *pos_categories;
  mapping  word_type_map;

  /* Make sure we have a valid wordmap */
  init_wordmap();

  /* Allocate and initialize all the category mappings */
  categories = allocate(1 << sizeof(parts_of_speech));
  for(ctr = 0; ctr < (1 << sizeof(parts_of_speech)); ctr++)
    categories[ctr] = ([ ]);

  /* For each word, place it in its appropriate category */
  word_type_map = pvt_get_wordmap();
  words = map_indices(word_type_map);
  for(ctr = 0; ctr < sizeof(words); ctr++) {
    categories[word_type_map[words[ctr]]][words[ctr]] = 1;
  }

  /* Make the token grammar from the categories */
  output = "";
  for(ctr = 1; ctr < sizeof(categories); ctr++) {
    words = map_indices(categories[ctr]);

    if(words && sizeof(words)) {
      /* Divide into 50+-char lines, with 3-char separators */
      linelist = divide_into_lines(words, 50, 3);

      for(line = 0; line < sizeof(linelist); line++) {
	output += make_string_from_pos_bits(ctr) + " = /(";
	output += implode(linelist[line], ")|(");
	output += ")/\n";
      }
    } else {
      output += "# Skipping " + make_string_from_pos_bits(ctr) + "\n";
    }
  }

  /* Make new categories */
  pos_categories = allocate(sizeof(parts_of_speech));
  for(ctr = 0; ctr < sizeof(parts_of_speech); ctr++)
    pos_categories[ctr] = ([ ]);

  /* Sift category information into pos_categories */
  for(ctr = 1; ctr < sizeof(categories); ctr++) {

    /* For each part of speech (counted off by 'line'), check the bit
       and put the category into the pos_categories entry.  The
       category is assigned to the pos_categories entry by inserting into the
       pos_category entry's hash table with a key of the category number and
       a value of 1. */
    for(line = 0; line < sizeof(parts_of_speech); line++) {
      if((ctr & (1 << line)) && map_sizeof(categories[ctr])) {
	pos_categories[line][ctr] = 1;
      }
    }
  }

  /* Make the non-token grammar from the pos_categories */
  nontoken = "";
  for(ctr = 0; ctr < sizeof(parts_of_speech); ctr++) {
    nontoken += "# All different kinds of " + parts_of_speech[ctr] + "\n";

    cat_list = map_indices(pos_categories[ctr]);
    for(line = 0; line < sizeof(cat_list); line++) {
      nontoken += parts_of_speech[ctr] + ": "
	+ make_string_from_pos_bits(cat_list[line]) + "\n";
    }

    nontoken += "\n";
  }

  return ({ output, nontoken });
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

private void set_grammar(void) {
  /* This function is told when it's time to regenerate the grammar
     from words and files.  This is generally after a recompile, or
     after OLC has added a new word to one of the part-of-speech
     tables.  You never know when the user will want to type one of
     those new words, so we just regenerate the grammar to be sure. */

  /* The grammar is made of a whitespace token (contains ctrl chars,
     must be inserted from LPC), an autogenerated chunk of
     part-of-speech tokens, a token-parsing grammar from one file, a
     bad-token token (also contains ctrl chars), and a non-token
     grammar from a second file.  These are assembled in the order
     listed, using simple string concatenation. */

  /* If we ever start overrunning the DGD string limit on this concat,
     we can uncomment_file() earlier in the process on the subfiles
     individually. */

  if(regenerate_grammar) {
    string  whitespace_string, bad_token_string;
    string  token_autogen, grammar_autogen;
    string *gram;

    whitespace_string = "whitespace = /[ \r\n\t\b,]+/\n";
    bad_token_string = "bad_token = /[^ \r\n\t\b\\\\!,.?:;]+/\n";
    gram = pos_grammars();
    token_autogen = gram[0];
    grammar_autogen = gram[1];

    grammar = whitespace_string + token_file + token_autogen + "\n"
      + bad_token_string + grammar_file + grammar_autogen;

    grammar = uncomment_file(grammar);
    regenerate_grammar = 0;

    LOG("*********************************************\n");
    LOG("*********************************************\n");
    LOG("*********************************************\n");
    LOG("Setting grammar:\n" + grammar + "\n");
  }
}


/* function for parsing commands */
mixed *parse_cmd(string cmd){
  mixed *ret;

  set_grammar();

  LOG("*********************************************\n");
  LOG("Parsing string: " + cmd + "$\n");

  if(!cmd || STRINGD->is_whitespace(cmd))
    return ({ });

  catch {
    ret = parse_string(grammar, cmd, NUM_AMBIGUOUS);

    return ret;
  } : {
    error("Parsing failed.  Command is '" + cmd + "'.");
    return nil;
  }
}
