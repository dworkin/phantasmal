#include <config.h>

private mapping tags;

static void create(varargs int clone) {
  tags = ([ ]);
}

void upgraded(void) {

}

nomask mixed get_tag(string tag_name) {
  if(previous_program() == TAGD) {
    return tags[tag_name];
  }
  error("Only TagD can directly get tag values!");
}

nomask void set_tag(string tag_name, mixed value) {
  if(previous_program() == TAGD)
    tags[tag_name] = value;
  else
    error("Only TagD can directly get tag values!");
}
