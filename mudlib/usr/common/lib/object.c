#include <config.h>

#include <phrase.h>
#include <grammar.h>

/*
 * /lib/object.c
 *
 * A straightforward MUD object with the normal, expected facilities.
 * Not, by itself, a container.
 *
 */

/* These allow the object to be contained, and the setters for this
   are all access-controlled so only the CONTAINER object can play
   with them. */
object location;  /* Same as 'environment' in many MUDLibs */

/* Descriptions, available to the player when examining the object */
object bdesc;  /* "Brief" description, such as "a sword" or
		  "John Kricfalusi" or "some bacon" */
object gdesc;  /* "Glance" description, a short phrase such as
		  "John Kricfalusi, the guy behind Ren and Stimpy",
		  "a red, flashing toy gun", or "about a pound of bacon" */
object ldesc;  /* A longer standard "look" description */
object edesc;  /* An "examine" description, meant to convey details only
		  available when searched for.  Defaults to ldesc. */

/* Specifiers, determining how the player may refer to the object.
   These replace the name field (obsolete) with something complex and
   localized. */
mixed** nouns;   /* An array of phrases for the various nouns which can
		    specify this object */
mixed** adjectives;  /* An array of phrases for allowable adjectives that
			specify this object */

int     desc_article; /* This is the article type which may be optionally
			 prepended to brief and glance descriptions */

/* Tracking number for OBJNUMD */
int    tr_num;

/* Mobile, if any, perceiving this object as its body */
object mobile;


/* Prototypes */
void clear_nouns(void);
void clear_adjectives(void);


static void create(varargs int clone) {
  ::create(clone);

  if(clone) {
    tr_num = -1;
    mobile = nil;

    clear_nouns();
    clear_adjectives();
  }
}

void destructed(int clone) {
  if(clone && location) {
    location->remove_from_container(this_object());
  }
}

void upgraded(varargs int clone) {

}

object get_location(void) {
  return location;
}

/* This private routine takes a locale and a phrase struct and tries
   to deduce the appropriate article to use for it. */
private string choose_article(int locale, object desc) {
  if(locale == LANG_espanolUS) {
    return "el";
  } else if(locale == LANG_englishUS) {
    switch(desc_article) {
    case ART_DEFINITE:
      return "the";
    case ART_PROPER:
      return "";
    case ART_INDEFINITE: {
      if(STRINGD->should_use_an(desc->get_content_by_lang(LANG_englishUS))) {
	return "an";
      }
      return "a";
    }
    default:
      return "some";
    }
  }

  return "---";
}

string get_brief_article(int locale) {
  return choose_article(locale, bdesc);
}

string get_glance_article(int locale) {
  return choose_article(locale, gdesc);
}

int get_article_type(void) {
  return desc_article;
}

void set_article_type(int new_type) {
  desc_article = new_type;
}

object get_mobile(void) {
  return mobile;
}

object get_brief(void) {
  return bdesc;
}

void set_brief(object brief) {
  bdesc = brief;
}

object get_glance(void) {
  return gdesc;
}

void set_glance(object glance) {
  gdesc = glance;
}

object get_look(void) {
  return ldesc;
}

void set_look(object look) {
  ldesc = look;
}

object get_examine(void) {
  if(!edesc) return ldesc;
  return edesc;
}

void set_examine(object examine) {
  edesc = examine;
}

void clear_examine(void) {
  edesc = nil;
}

int get_number(void) {
  return tr_num;
}

void add_noun(object phr) {
  int    locale, ctr2;
  string tmp;
  mixed* words;

  if(PHRASED->num_locales() > sizeof(nouns)) {
    error("Fix objects to support dynamic adding of locales!");
  }

  for(locale = 0; locale < PHRASED->num_locales(); locale++) {
    tmp = phr->get_content_by_lang(locale);

    if(tmp) {
      words = explode(tmp, ",");
      for(ctr2 = 0; ctr2 < sizeof(words); ctr2++) {
	words[ctr2] = STRINGD->trim_whitespace(words[ctr2]);
	if(words[ctr2] && words[ctr2] != "")
	  nouns[locale] += ({ words[ctr2] });
      }
    }
  }
}


void clear_nouns(void) {
  int ctr, num_loc;

  num_loc = PHRASED->num_locales();
  nouns = allocate(num_loc);

  for(ctr = 0; ctr < num_loc; ctr++) {
    nouns[ctr] = ({ });
  }
}


mixed* get_nouns(int locale) {
  if(locale < 0 || locale >= sizeof(nouns))
    return nil;

  return nouns[locale][..];
}

mixed* get_adjectives(int locale) {
  if(locale < 0 || locale >= sizeof(adjectives))
    return nil;

  return adjectives[locale][..];
}


void add_adjective(object phr) {
  int    locale, ctr2;
  string tmp;
  mixed* words;

  if(PHRASED->num_locales() > sizeof(nouns)) {
    error("Fix objects to support dynamic adding of locales!");
  }

  for(locale = 0; locale < PHRASED->num_locales(); locale++) {
    tmp = phr->get_content_by_lang(locale);

    if(tmp) {
      words = explode(tmp, ",");
      for(ctr2 = 0; ctr2 < sizeof(words); ctr2++) {
	words[ctr2] = STRINGD->trim_whitespace(words[ctr2]);
	if(words[ctr2] != "")
	  adjectives[locale] += ({ words[ctr2] });
      }
    }
  }
}


void clear_adjectives(void) {
  int ctr, num_loc;

  num_loc = PHRASED->num_locales();
  adjectives = allocate(num_loc);

  for(ctr = 0; ctr < num_loc; ctr++) {
    adjectives[ctr] = ({ });
  }
}


private int match_str_array(string* words, string* array) {
  int ctr, ctr2, match;

  for(ctr = 0; ctr < sizeof(words); ctr++) {
    match = 0;

    for(ctr2 = 0; ctr2 < sizeof(array); ctr2++) {
      if(STRINGD->prefix_string(words[ctr], array[ctr2])) {
	match = 1;
	break;
      }
    }

    if(!match) {
      return 0;
    }
  }

  return 1;
}

int match_description(object user, string *cmp_adjectives, string* cmp_nouns) {
  int locale, match;

  locale = user->get_locale();
  match = match_str_array(cmp_nouns, nouns[locale]);

  if(!match && locale != LANG_englishUS) {
    match = match_str_array(cmp_nouns, nouns[LANG_englishUS]);
  }

  if(!match) return 0;

  match = match_str_array(cmp_adjectives, adjectives[locale]);
  if(!match && locale != LANG_englishUS) {
    match = match_str_array(cmp_adjectives, adjectives[LANG_englishUS]);
  }

  return match;
}


/* Access-protected functions */


/* Functions for CONTAINER use */

/* set_location sets the location variable directly.  Mostly, this shouldn't
   happen.  Instead it should be set with the assorted container commands. */
void set_location(object new_loc) {
  if(previous_program() == CONTAINER) {
    location = new_loc;
  } else {
    error("You can't set that from there!");
  }
}


/* Functions for MOBILE use */

void set_mobile(object new_mob) {
  if(previous_program() == MOBILE) {
    mobile = new_mob;
  } else {
    error("You can't set that from there!");
  }
}


/* Functions for MAPD, EXITD, etc use - overridden in child classes */

void set_number(int num) {
  tr_num = num;
}
