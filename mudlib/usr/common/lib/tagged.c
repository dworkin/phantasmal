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

nomask mixed* get_all_tags(void) {
  string *tag_names;
  mixed  *ret;
  int     ctr;

  if(previous_program() != TAGD)
    error("Only TagD can directly get tag values!");

  ret = ({ });
  tag_names = map_indices(tags);
  for(ctr = 0; ctr < sizeof(tag_names); ctr++) {
    ret += ({ tag_names[ctr], tags[tag_names[ctr]] });
  }

  return ret;
}

nomask void set_tag(string tag_name, mixed value) {
  if(previous_program() == TAGD)
    tags[tag_name] = value;
  else
    error("Only TagD can directly get tag values!");
}
