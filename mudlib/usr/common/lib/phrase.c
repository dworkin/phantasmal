#include <config.h>
#include <type.h>
#include <phrase.h>

/* Phrase functions */

static void create(varargs int clone)
{
}

static int resolve_lang(varargs mixed lang) {
  int language;

  if(typeof(lang) == T_STRING) {
    language = PHRASED->language_by_name(lang);
  } else if (typeof(lang) == T_INT) {
    language = lang;
  } else {
    return -1;
  }

  return language;
}

string to_string(object user) {
  return "--DEBUG--";
}
