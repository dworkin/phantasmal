#include <phantasmal/phrase.h>
#include <phantasmal/lpc_names.h>

#include <type.h>

inherit PHRASE;

mixed* content;

static void create(varargs int clone)
{
}

string *as_taglist(void) {
  if(!content) return nil;
  return content[..];
}

void from_taglist(string *new_content) {
  content = new_content;
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

string as_markup(int markup_type) {
  switch(markup_type) {
  case MARKUP_UNQ:
    return as_unq();
  case MARKUP_XML:
    return as_xml();
  default:
    error("Illegal markup type in as_markup!");
  }
}

void from_unq(string unq_markup) {
  content = unq_to_taglist(unq_markup);
}

void from_unq_data(mixed unq) {
  if(typeof(unq) == T_STRING) {
    from_taglist( ({ "", unq }) );
  } else {
    content = unq_data_to_taglist(unq);
  }
}

string to_string(object user) {
  return user->taglist_to_string(content);
}

string to_unq_text(void) {
  return taglist_to_unq(content);
}
