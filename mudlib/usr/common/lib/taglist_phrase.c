#include <phantasmal/phrase.h>
#include <phantasmal/lpc_names.h>

#include <type.h>

inherit PHRASE;

mixed* content;

static void create(varargs int clone)
{
}

string as_xml(void) {
  return taglist_to_xml(content);
}

void from_xml(string xml_markup) {
  content = xml_to_taglist(xml_markup);
}

string as_unq(void) {
  return taglist_to_unq(content);
}

void from_unq(string unq_markup) {
  content = unq_to_taglist(unq_markup);
}

private string taglist_to_string(object user, mixed *tl) {
  int ctr;

  if(!tl) return "";

  for(ctr = 0; ctr < sizeof(tl); ctr += 2) {


    switch(tl[ctr]) {

    }
  }
}

string to_string(object user) {
  return taglist_to_string(user, content);
}

string to_unq_text(void) {
  string ret;

  ret = taglist_to_unq(content);
  return ret;
}
