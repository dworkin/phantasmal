#include <kernel/kernel.h>

#include <config.h>
#include <type.h>

static mapping mobile_tags;
static mapping object_tags;

static void create(void) {
  mobile_tags = ([ ]);
  object_tags = ([ ]);
}

void upgraded(void) {
  if(!COMMON() && !SYSTEM())
    return;
}

private void check_value_type(int type, mixed value) {
  if(typeof(value) == type)
    return;

  if(typeof(value) == T_NIL && (type != T_INT && type != T_FLOAT)) {
    return;
  }
  error("Type mismatch:  type " + typeof(value)
	+ " won't fit into a tag of type " + type + "!");
}

void new_mobile_tag(string name, int type,
		    varargs string get_function, string set_function) {
  if(!GAME() && !COMMON() && !SYSTEM())
    error("Only game code can create new mobile tag types!");

  if(mobile_tags[name])
    error("Mobile tag type '" + name + "' already exists!");

  mobile_tags[name] = ({ type, get_function, set_function });
}

void new_object_tag(string name, int type,
		    varargs string get_function, string set_function) {
  if(!GAME() && !COMMON() && !SYSTEM())
    error("Only game code can create new object tag types!");

  if(object_tags[name])
    error("Object tag type '" + name + "' already exists!");

  object_tags[name] = ({ type, get_function, set_function });
}

mixed mobile_get_tag_value(object mobile, string name) {
  mixed *tag_arr;

  if(!GAME() && !COMMON() && !SYSTEM())
    error("Only Game code can get mobile tag values!");

  if(function_object("set_number", mobile) != MOBILE)
    error("Can only call mobile_get_tag_value on mobiles!");

  tag_arr = mobile_tags[name];
  if(!tag_arr)
    error("No such mobile tag type as '" + name + "'!");

  return call_other(mobile, (tag_arr[1] ? tag_arr[1] : "get_tag"), name);
}

void mobile_set_tag_value(object mobile, string name, mixed value) {
  mixed *tag_arr;

  if(!GAME() && !COMMON() && !SYSTEM())
    error("Only Game code can set mobile tag values!");

  if(function_object("set_number", mobile) != MOBILE)
    error("Can only call mobile_set_tag_value on mobiles!");

  tag_arr = mobile_tags[name];
  if(!tag_arr)
    error("No such mobile tag type as '" + name + "'!");

  check_value_type(tag_arr[0], value);

  call_other(mobile, (tag_arr[1] ? tag_arr[1] : "get_tag"), name, value);
}

mixed object_get_tag_value(object obj, string name) {
  mixed *tag_arr;

  if(!GAME() && !COMMON() && !SYSTEM())
    error("Only Game code can get object tag values!");

  if(function_object("set_number", obj) != OBJECT)
    error("Can only call object_get_tag_value on objects!");

  tag_arr = object_tags[name];
  if(!tag_arr)
    error("No such object tag type as '" + name + "'!");

  return call_other(obj, (tag_arr[1] ? tag_arr[1] : "get_tag"), name);
}

void object_set_tag_value(object obj, string name, mixed value) {
  mixed *tag_arr;

  if(!GAME() && !COMMON() && !SYSTEM())
    error("Only Game code can set object tag values!");

  if(function_object("set_number", obj) != OBJECT)
    error("Can only call object_set_tag_value on objects!");

  tag_arr = object_tags[name];
  if(!tag_arr)
    error("No such object tag type as '" + name + "'!");

  check_value_type(tag_arr[0], value);

  call_other(obj, (tag_arr[2] ? tag_arr[2] : "set_tag"), name, value);
}

mixed get_tag_value(object tagged_object, string name) {
  if(!GAME() && !COMMON() && !SYSTEM())
    error("Only game code may call get_tag_value!");

  switch(function_object("set_number", tagged_object)) {
  case OBJECT:
    return object_get_tag_value(tagged_object, name);
  case MOBILE:
    return mobile_get_tag_value(tagged_object, name);
  default:
    error("Don't recognized tagged object type in get_tag_value!");
  }
}

void set_tag_value(object tagged_object, string name, mixed value) {
  if(!GAME() && !COMMON() && !SYSTEM())
    error("Only game code may call set_tag_value!");

  switch(function_object("set_number", tagged_object)) {
  case OBJECT:
    object_set_tag_value(tagged_object, name, value);
    return;
  case MOBILE:
    mobile_set_tag_value(tagged_object, name, value);
    return;
  default:
    error("Don't recognize tagged object type in set_tag_value!");
  }
}

string* mobile_tag_names(void) {
  if(!GAME() && !COMMON() && !SYSTEM())
    error("Only game code may call mobile_tag_names!");

  return map_indices(mobile_tags);
}

string* object_tag_names(void) { 
  if(!GAME() && !COMMON() && !SYSTEM())
    error("Only game code may call object_tag_names!");

  return map_indices(object_tags);
}
