#include <config.h>

private object dtd;

static void create(varargs int clone) {
  if(clone) {

  } else {
    if(!find_object(UNQ_PARSER)) { compile_object(UNQ_PARSER); }
    if(!find_object(UNQ_DTD)) { compile_object(UNQ_DTD); }
  }
}

void destructed(int clone) {
  if(dtd)
    destruct_object(dtd);
}

void upgraded(varargs int clone) {

}

/* This sets the DTD object given as this object's DTD.  That means
   the object may destruct the DTD at its leisure, among other things. */
void set_dtd(object new_dtd) {
  dtd = new_dtd;
}

/* This supplies the text of a DTD file to be this object's DTD. */
void set_dtd_text(string dtd_text) {
  error("Unimplemented!");
}

/* This supplies a filename to load the object's DTD from. */
void set_dtd_file(string filename) {
  error("Unimplemented!");
}

/* This method serializes the object as UNQ text and returns the
   appropriate string, suitable for writing to an UNQ file. */
string to_unq_text(void) {
  error("You must override the default to_unq_text method for "
	+ object_name(this_object()));
}

/* This method converts the object to the same sort of template
   that parsing an UNQ file with a DTD yields.  This template
   can then be converted to text. */
mixed* to_dtd_unq(void) {
  error("You must override the default to_dtd_unq method for "
	+ object_name(this_object()));
}

/* This method takes a string of UNQ text as loads it into the
   object. */
void from_unq_text(string text) {
  error("You must override the default from_unq_text method for "
	+ object_name(this_object()));
}

/* This method takes the result of parsing an UNQ text file
   as simple UNQ and loads it into the object. */
void from_unq(mixed* unq) {
  error("You must override the default from_unq method for "
	+ object_name(this_object()));
}

/* This method takes the result of parsing the object according to
   its DTD and loads it into the object. */
void from_dtd_unq(mixed* unq) {
  error("You must override the default from_dtd_unq method for "
	+ object_name(this_object()));
}
