#include <kernel/kernel.h>

#include <phantasmal/log.h>

#include <config.h>
#include <type.h>

inherit unq DTD_UNQABLE;

/* Prototypes */
void upgraded(varargs int clone);


/* Constants */
#define SOULD_ARRAY_SIZE       5

/* Offsets into arrays */
#define SOULD_SELF_ONLY        0
#define SOULD_SELF_TARGET      1
#define SOULD_TARGET           2
#define SOULD_OTHER_ONLY       3
#define SOULD_OTHER_TARGET     4


/* Data from SoulD file*/
mapping sould_strings;
int     num_soc;


static void create(varargs int clone) {
  if(clone) {
    error("Can't clone CONFIGD!");
  }

  unq::create(clone);
  upgraded();
}

void upgraded(varargs int clone) {
  if(!SYSTEM() && !COMMON())
    return;

  set_dtd_file(SOULD_DTD);
  unq::upgraded();

  /* Clear out Soul array before reloading */
  sould_strings = ([ ]);
  num_soc = 0;

  /* We'll need to load the file's contents... */
  load_from_file(SOULD_FILE);
  num_soc = map_sizeof(sould_strings);

  /* And now we need to update USER's command set */
  SYSTEM_USER->set_social_commands();
}

mixed* to_dtd_unq(void) {
  error("Not implemented yet");
}

void from_dtd_unq(mixed* unq) {
  if(!SYSTEM() && !COMMON())
    return;

  while(sizeof(unq) > 0) {
    if(unq[0] == "social") {
      mixed* entry, *soc_unq;

      entry = allocate(SOULD_ARRAY_SIZE);
      soc_unq = unq[1];

      while(sizeof(soc_unq) > 0) {
	if(soc_unq[0][0] == "verb") {
	  sould_strings[soc_unq[0][1]] = entry;
	} else if (soc_unq[0][0] == "self-only") {
	  entry[SOULD_SELF_ONLY] = soc_unq[0][1];
	} else if (soc_unq[0][0] == "self-target") {
	  entry[SOULD_SELF_TARGET] = soc_unq[0][1];
	} else if (soc_unq[0][0] == "target") {
	  entry[SOULD_TARGET] = soc_unq[0][1];
	} else if (soc_unq[0][0] == "other-only") {
	  entry[SOULD_OTHER_ONLY] = soc_unq[0][1];
	} else if (soc_unq[0][0] == "other-target") {
	  entry[SOULD_OTHER_TARGET] = soc_unq[0][1];
	} else {
	  error("Unrecognized tag '" + STRINGD->mixed_sprint(soc_unq[0])
		+ "' in social entry parsing SoulD file!");
	}
	soc_unq = soc_unq[1..];
      }
    }
    unq = unq[2..];
  }
}

int is_valid(string verb) {
  if(SYSTEM() || COMMON() || GAME())
    return !!sould_strings[verb];

  return -1;
}

int num_socials(void) {
  if(SYSTEM() || COMMON() || GAME())
    return num_soc;

  return -1;
}

string* all_socials(void) {
  mixed* keys;

  if(!SYSTEM() && !COMMON() && !GAME())
    return nil;

  keys = map_indices(sould_strings);
  return keys[..];
}

string get_social_string(object user, object body,
			 object target_body, string verb) {
  mixed *entry, *unq;
  string result;
  object phr;

  if(!SYSTEM() && !COMMON() && !GAME())
    return nil;

  entry = sould_strings[verb];
  if(!entry) return nil;

  if(user->get_body() == body) {
    /* Looks like this is done by us... */
    if(target_body) {
      unq = entry[SOULD_SELF_TARGET];
    } else {
      unq = entry[SOULD_SELF_ONLY];
    }
  } else if (user->get_body() == target_body) {
    /* Looks like this is done *to* us */
    unq = entry[SOULD_TARGET];
  } else {
    if(target_body) {
      unq = entry[SOULD_OTHER_TARGET];
    } else {
      unq = entry[SOULD_OTHER_ONLY];
    }
  }

  if(!unq)
    error("Can't resolve UNQ for social verb '" + verb + "'!");

  /* Go through and replace tags */
  result = "";
  while(unq && sizeof(unq)) {
    if(!unq[0] || unq[0] == "") {
      result += unq[1];
    } else if (unq[0] == "target") {
      phr = target_body->get_brief();
      result += phr->to_string(user);
    } else if (unq[0] == "actor") {
      phr = body->get_brief();
      result += phr->to_string(user);      
    } else {
      error("Unrecognized tag " + STRINGD->mixed_sprint(unq[0])
	    + "/" + STRINGD->mixed_sprint(unq[1])
	    + " in string substitution for social verb '" + verb + "'");
    }
    unq = unq[2..];
  }

  return result;
}
