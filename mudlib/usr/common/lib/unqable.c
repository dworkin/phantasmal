#include <config.h>

private object dtd;

static void create(varargs int clone) {

}

void destructed(int clone) {

}

void set_dtd(object new_dtd) {
  dtd = new_dtd;
}

string to_unq_text(void) {
  error("Override default to_unq_text method for "
	+ object_name(this_object()));
}

mixed* to_dtd_unq(void) {
  error("Override default to_dtd_unq method for "
	+ object_name(this_object()));
}

void from_unq(mixed* unq) {
  error("Override default from_unq method for "
	+ object_name(this_object()));
}

void from_dtd_unq(mixed* unq) {
  error("Override default from_dtd_unq method for "
	+ object_name(this_object()));
}
