#include <config.h>
#include <limits.h>

inherit UNQABLE;

private object dtd;
private string dtd_text;
private string dtd_filename;

private void update_dtd_vars(void);

mixed* to_dtd_unq  (void);
void   from_dtd_unq(mixed* unq);

static void create(varargs int clone) {
  ::create(clone);
}

void destructed(int clone) {
  if(dtd)
    destruct_object(dtd);

  ::destructed(clone);
}

void upgraded(varargs int clone) {
  ::upgraded(clone);

  if(dtd_filename) {
    update_dtd_vars();
  }
}

/* This sets the DTD object given as this object's DTD.  That means
   the object may destruct the DTD at its leisure, among other things. */
void set_dtd(object new_dtd) {
  dtd = new_dtd;
}

/* This supplies the text of a DTD file to be this object's DTD. */
void set_dtd_text(string new_dtd_text) {
  dtd_text = new_dtd_text;
  update_dtd_vars();
}

/* This supplies a filename to load the object's DTD from. */
void set_dtd_file(string new_dtd_filename) {
  dtd_filename = new_dtd_filename;
  update_dtd_vars();
}

/* The updates dtd and dtd_text from themselves and dtd_filename. */
private void update_dtd_vars(void) {
  if(dtd_filename) {
    string buf;

    buf = read_file(dtd_filename);
    if(strlen(buf) > MAX_STRING_SIZE - 3) {
      error("DTD file is too large in update_dtd_vars!");
    }

    dtd_text = buf;
  }
  if(dtd_text) {
    if(dtd) {
      dtd->clear();
    } else {
      dtd = clone_object(UNQ_DTD);
    }

    dtd->load(dtd_text);
  }
}

/* This method serializes the object as UNQ text and returns the
   appropriate string, suitable for writing to an UNQ file. */
string to_unq_text(void) {
  if(!dtd) {
    error("You must override the default to_unq_text method for "
	  + object_name(this_object()) + " or supply a DTD!");
  }

  return dtd->serialize_to_dtd(to_dtd_unq());
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
void from_unq(mixed* unq) {
  if(!dtd) {
    error("You must override the default from_unq_text method for "
	  + object_name(this_object()) + " or supply a DTD!");
  }

}


/* This method takes the result of parsing the object according to
   its DTD and loads it into the object. */
void from_dtd_unq(mixed* unq) {
  error("You must override the default from_dtd_unq method for "
	+ object_name(this_object()));
}
