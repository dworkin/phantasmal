#include <phantasmal/phrase.h>
#include <phantasmal/lpc_names.h>

#include <type.h>

inherit PHRASE;

mixed* content;

static void create(varargs int clone)
{
  if(clone) {
    content = allocate(PHRASED->num_locales());
  }
}

string to_string(object user) {
  int locale;

  locale = user->get_locale();
  if(locale >= 0) {
    if(locale < sizeof(content) && content[locale])
      return content[locale];
    if(content[LANG_englishUS])
      return content[LANG_englishUS];
    if(content[LANG_debugUS])
      return content[LANG_debugUS];
    return "(nil)";
  }
  return "(Illegal locale)";
}

mixed get_content_by_lang(int lang) {
  if(lang < 0 || lang >= sizeof(content))
    return nil;

  return content[lang];
}

void set_content_by_lang(int lang, mixed cont) {
  if(lang >= 0 && lang < sizeof(content))
    content[lang] = cont;
  else if (lang < PHRASED->num_locale()) {
    /* New locale added -- realloc the array */
    content += allocate(PHRASED->num_locale() - sizeof(content));
    content[lang] = cont;
  } else
    error("Invalid lang passed to content_by_lang!");
}

/* This phrase is a blatant nod to how we've been failing to
   internationalize.  If this is true, there is a nontrivial
   non-English translation for this phrase.  If false, there isn't. */

int not_english(void) {
  int ctr;

  for(ctr = 0; ctr < PHRASED->num_locale(); ctr++) {
    if(ctr == LANG_englishUS) ctr++;
    if(content[ctr] && content[ctr] != content[LANG_englishUS]) return TRUE;
  }

  return FALSE;
}

#if 0
void trim_whitespace(void) {
  int iter;

  error("Nobody seems to use this...  Good.");

  for(iter = 0; iter < sizeof(content); iter++) {
    if(content[iter]) {
      if(typeof(content[iter]) == T_STRING)
	content[iter] = STRINGD->trim_whitespace(content[iter]);
      else {
	/* Just trim the beginning of the first string, and the
	   end of the last one. */
	error("Trim_whitespace not yet implemented for all phrases!");
      }
    }
  }
}
#endif

string to_unq_text(void) {
  string ret;
  int    ctr;

  ret = "";
  /* Start with 1, not 0 -- 0 is debug, and is special */
  for(ctr = 1; ctr < sizeof(content); ctr++) {
    if(content[ctr]) {
      ret += "\n    ~" + PHRASED->locale_name_for_language(ctr);
      ret += "{" + STRINGD->unq_escape(content[ctr]) + "}";
    }
  }

  if(content[0] && content[0] != content[LANG_englishUS]) {
    ret += "\n    ~debugUS{" + STRINGD->unq_escape(content[0]) + "}";
  }

  return ret;
}
