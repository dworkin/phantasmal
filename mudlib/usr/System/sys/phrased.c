#include <kernel/kernel.h>

#include <phantasmal/phrase.h>

#include <config.h>
#include <type.h>
#include <status.h>

inherit rep PHRASE_REPOSITORY;

/* The Phrased handles the Phrase data structure in its many and
   varied forms.  This means that it handles localization for Player
   (usually OOC) languages.  It also interprets, filters and processes
   Phrases in various formats and handles all externally visible
   grammar processing, though this is all implemented internally with
   Phrase objects within the Phrased.
*/

/* Security notes: most functions in this object are unprotected.
   That's because most of them are basically standard library
   functions with no security implications.
*/


/* This is upgradable with a recompile, mostly.  I'd test that feature
   more before you assume it works, though.  Removing languages is
   possible, but much harder and you'll probably have to add a lot of
   code to Phantasmal. */
#define INTL_NUM_LANG 5

/**************** Dealing with locales ****************************/

static mapping languages;
static mixed*  langnames;
static mixed*  localenames;

static void create(varargs int clone)
{
  rep::create(clone);

  languages = ([
		"debug" :        LANG_debugUS,
		"english" :      LANG_englishUS,
		"espanol" :      LANG_espanolUS,
		"spanish" :      LANG_espanolUS,
		"spanglish" :    LANG_espanolUS,
		"DEBUG" :        LANG_debugUS,
		"ENGLISH" :      LANG_englishUS,
		"ESPANOL" :      LANG_espanolUS,
		"SPANISH" :      LANG_espanolUS,
		"SPANGLISH" :    LANG_espanolUS,
		"debugUS" :      LANG_debugUS,
		"englishUS" :    LANG_englishUS,
		"espanolUS" :    LANG_espanolUS,
		"spanishUS" :    LANG_espanolUS,
		"en" :           LANG_englishUS,
		"eng" :          LANG_englishUS,
		"es" :           LANG_espanolUS,
		"esp" :          LANG_espanolUS,
		"enUS" :         LANG_englishUS,
		"engUS" :        LANG_englishUS,
		"esUS" :         LANG_espanolUS,
		"espUS" :        LANG_espanolUS,
		"US" :           LANG_englishUS,
  ]);

  langnames = ({ "debug", "none", "none", "US english",
		   "broken spanish" });
  localenames = ({ "dbUS", "noNO", "noNO", "enUS",
		     "esUS" });

  /* Set up base repository of phrases for localized MUD strings outside
     of any zone or world such as login greetings, menus and error
     messages. */
  if(!load_filemanaged_file(SYSTEM_PHRASES)) {
    error("Can't load system phrase file!");
  }
}

void upgraded(varargs int clone) {
  if(SYSTEM()) {
    rlimits ( status()[ST_STACKDEPTH]; -1 ) {
      rep::upgraded();
    }
  }
}

/* Query current number of locales */
int num_locales(void) {
  return INTL_NUM_LANG;
}

/* Query by human-settable name what locale number is being used */
int language_by_name(string name) {
  if(languages[name] != nil)
    return languages[name];

  return -1;
}

/* Get a human-readable name for a locale number */
string name_for_language(int lang) {
  return langnames[lang];
}

/* Get a locale name (suitable for file formats or formal displays) for
   a locale number */
string locale_name_for_language(int lang) {
  return localenames[lang];
}

object new_simple_english_phrase(string eng_str) {
  object phr;

  if(!find_object(LWO_PHRASE))
    { compile_object(LWO_PHRASE); }
  phr = new_object(LWO_PHRASE);
  phr->set_content_by_lang(LANG_englishUS, eng_str);

  return phr;
}

/*
  object unq_to_phrase(mixed* unq)

  Convert an UNQ-parsed array of labels and content to a Phrase
  data structure.  Each label should correspond to a recognized
  locale.
*/
object unq_to_phrase(mixed unq) {
  int    iter, lang;
  object phrase;
  mixed  uitem;
  mixed* tmp;

  if(typeof(unq) == T_STRING)
    return new_simple_english_phrase(unq);

  if(sizeof(unq) % 2) {
    error("Odd-sized array passed to unq_to_phrase");
  }

  /* Generate empty phrase structure */
  if(!find_object(LWO_PHRASE))
    { compile_object(LWO_PHRASE); }
  phrase = new_object(LWO_PHRASE);

  unq = UNQ_PARSER->trim_empty_tags(unq);

  iter = 0;
  while(iter < sizeof(unq)) {
    lang = PHRASED->language_by_name(unq[iter]);
    if(lang == -1) {
      error("Unknown locale/lang '" + unq[iter] + "' in unq_to_phrase!");
    }
    phrase->set_content_by_lang(lang, unq[iter + 1]);

    iter += 2;
  }

  /* Set Debug locale automatically, if needed & possible */
  if(phrase->get_content_by_lang(LANG_debugUS) == "Debug") {
    mixed tmp;

    tmp = phrase->get_content_by_lang(LANG_englishUS);
    if(tmp) {
      phrase->set_content_by_lang(LANG_debugUS, tmp);
    } else {
      tmp = phrase->get_content_by_lang(LANG_espanolUS);
      if(tmp) {
	phrase->set_content_by_lang(LANG_debugUS, tmp);
      }
    }
  } /* end set Debug locale */

  return phrase;
}
