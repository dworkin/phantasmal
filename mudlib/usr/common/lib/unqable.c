#include <config.h>

void from_unq(mixed* unq);

static void create(varargs int clone) {
  if(clone) {

  } else {
    if(!find_object(UNQ_PARSER)) { compile_object(UNQ_PARSER); }
    if(!find_object(UNQ_DTD)) { compile_object(UNQ_DTD); }
  }
}

void destructed(int clone) {
}

void upgraded(varargs int clone) {
}


/* This method serializes the object as UNQ text and returns the
   appropriate string, suitable for writing to an UNQ file. */
string to_unq_text(void) {
  error("You must override the default to_unq_text method for "
	+ object_name(this_object()));
}


/* This method takes a string of UNQ text as loads it into the
   object. */
void from_unq_text(string text) {
  from_unq(UNQ_PARSER->basic_unq_parse(text));
}


/* This method takes the result of parsing an UNQ text file
   as simple UNQ and loads it into the object. */
void from_unq(mixed* unq) {
  error("You must override the default from_unq method for "
	+ object_name(this_object()));
}
