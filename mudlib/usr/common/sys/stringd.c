#include <config.h>
#include <type.h>

static void create(varargs int clone) {
}

int char_is_whitespace(int char) {
  if((char == '\n') || (char == '\t')
     || char == ' ')
    return 1;

  return 0;
}

int char_to_lower (int char)
{
        if ((char <= 'Z' && char >= 'A'))
                char |= 0x20;

        return char;
}

int char_to_upper (int char)
{
        if ((char <= 'z' && char >= 'a'))
                char &= ~0x20;

        return char;
}

int is_whitespace(string str) {
  int len;
  int iter;

  len = strlen(str);
  for(iter = 0; iter < len; iter++) {
    if(!char_is_whitespace(str[iter]))
      return 0;
  }
  return 1;
}

int is_alpha(string str) {
  int ctr;

  return !!parse_string("regstring = /[a-zA-Z]+/\n"
			+ "notreg = /[^a-zA-Z]+/\n"
			+ "full : regstring", str);
}

int is_alphanum(string str) {
  int ctr;

  return !!parse_string("regstring = /[a-zA-Z0-9]+/\n"
			+ "notreg = /[^a-zA-Z0-9]+/\n"
			+ "full : regstring", str);
}

int string_has_char(int char, string str) {
  int len, iter;

  len = strlen(str);
  for(iter = 0; iter < len; iter++) {
    if(str[iter] == char)
      return 1;
  }
  return 0;
}

string trim_whitespace(string str) {
  int start, end;

  if(!str || str == "") return str;
  start = 0;
  end = strlen(str) - 1;
  while((start <= end) && char_is_whitespace(str[start]))
    start ++;
  while((start <= end) && char_is_whitespace(str[end]))
    end--;

  return str[start..end];
}

string to_lower(string text) {
  int ctr;
  int len;
  string newword;

  newword = text;
  len = strlen(newword);
  for(ctr=0; ctr<len; ctr++) {
    newword[ctr] = char_to_lower(newword[ctr]);
  }
  return newword;
}

string to_upper(string text) {
  int ctr;
  int len;
  string newword;

  newword = text;
  len = strlen(newword);
  for(ctr=0; ctr<len; ctr++) {
    newword[ctr] = char_to_upper(newword[ctr]);
  }
  return newword;
}

int stricmp(string s1, string s2) {
  int tmp1, tmp2, len1, len2;
  int len, iter;

  len1 = strlen(s1);
  len2 = strlen(s2);
  len = len1 > len2 ? len2 : len1;

  for(iter = 0; iter < len; iter++) {
    tmp1 = s1[iter]; tmp2 = s2[iter];
    if(tmp1 <= 'Z' && tmp1 >= 'A')
      tmp1 += 'a' - 'A';
    if(tmp2 <= 'Z' && tmp2 >= 'A')
      tmp2 += 'a' - 'A';

    if(tmp1 > tmp2)
      return 1;
    if(tmp2 > tmp1)
      return -1;
  }

  if(len1 == len2) return 0;
  if(len1 > len2) return 1;
  return -1;
}

string mixed_sprint(mixed data) {
  int    iter;
  string tmp;
  mixed* arr;

  switch(typeof(data)) {
  case T_NIL:
    return "nil";

  case T_STRING:
    return "\"" + data + "\"";

  case T_INT:
    return "" + data;

  case T_FLOAT:
    return "" + data;

  case T_ARRAY:
    if(data == nil) return "array/nil";
    if(sizeof(data) == 0) return "({ })";

    tmp = "({ ";
    for(iter = 0; iter < sizeof(data); iter++) {
      tmp += mixed_sprint(data[iter]);
      if(iter < sizeof(data) - 1) {
	tmp += ", ";
      }
    }
    return tmp + " })";

  case T_MAPPING:
    arr = map_indices(data);
    tmp = "([ ";
    for(iter = 0; iter < sizeof(arr); iter++) {
      tmp += arr[iter] + " => " + mixed_sprint(data[arr[iter]]) + ",";
    }
    return tmp + "])";

  case T_OBJECT:
    return "<Object (" + object_name(data) + ")>";

  default:
    error("Unrecognized DGD type in mixed_sprint");
  }
}

string unq_escape(string str) {
  string ret;
  int    index;

  if(!str) return nil;

  ret = "";
  for(index = 0; index < strlen(str); index++) {
    switch(str[index]) {
    case '{':
    case '}':
    case '~':
    case '\\':
      ret += "\\" + str[index..index];
    default:
      ret += str[index..index];
      break;
    }
  }
  return ret;
}

int prefix_string(string prefix, string str) {
  if(strlen(str) < strlen(prefix))
    return 0;

  if(str[0..strlen(prefix)-1] == prefix) {
    return 1;
  }

  return 0;
}
