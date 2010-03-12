#include <kernel/kernel.h>

#include <phantasmal/phrase.h>
#include <phantasmal/grammar.h>
#include <phantasmal/log.h>
#include <phantasmal/obj_flags.h>
#include <phantasmal/phrase.h>
#include <phantasmal/lpc_names.h>

inherit tag TAGGED;

/*
 * /lib/object.c
 *
 * A straightforward MUD object with the normal, expected facilities.
 *
 */

/* These allow the object to be contained, and the setters for this
   are all access-controlled so only the CONTAINER object can play
   with them. */
private object location;  /* Same as 'environment' in many MUDLibs */

/* Descriptions, available to the player when examining the object */
object bdesc;  /* "Brief" description, such as "a sword" or
                  "John Kricfalusi" or "some bacon" */
object ldesc;  /* A longer standard "look" description */
object edesc;  /* An "examine" description, meant to convey details only
                  available when searched for.  Defaults to ldesc. */

/* Specifiers, determining how the player may refer to the object. */
private string** nouns;   /* An array (by locale) of arrays of phrases
                             for the various nouns which can specify
                             this object */
private string** adjectives;  /* An array (by locale) of arrays of
                                 phrases for allowable adjectives that
                                 specify this object */

/* These are arrays of removed nouns and verbs from parent objects.
   Usually a parent's nouns and verbs are inherited automatically by
   each child object type, but these are specifically excepted. */
string **removed_nouns;
string **removed_adjectives;

/* Tracking number for OBJNUMD */
static int    tr_num;

/* Mobile, if any, perceiving this object as its body */
private object mobile;

/* Parent/archetype for data inheritance */
private object* archetypes;

/* Details -- sub-objects that are part of this one */

/* "details" lists immediate details of this object.  "removed_details"
   lists details that the object's parent has but that this object
   does not.  "detail_of" is an object that this object is a detail of,
   or nil. */
private object* details;
private object* removed_details;
private object  detail_of;

/* Objects contained in this one */
private mixed*  objects;
/* Mobiles of objects contained in this one */
private mixed*  mobiles;


/* Prototypes */
void prepend_to_container(object obj);
void append_to_container(object obj);
void clear_nouns(void);
void clear_adjectives(void);
string* get_nouns(int locale);
string* get_adjectives(int locale);
private void register_my_nouns(void);
private void unregister_my_nouns(void);
private void register_my_adjs(void);
private void unregister_my_adjs(void);

static void create(varargs int clone) {
  tag::create(clone);

  if(clone) {
    tr_num = -1;
    mobile = nil;

    clear_nouns();
    clear_adjectives();

    details = ({ });
    objects = ({ });
    mobiles = ({ });
    archetypes = ({ });

    removed_details = ({ });
  }
}

void destructed(int clone) {
  int index;

  if(!SYSTEM())
    return;

  if(clone && location) {
    location->remove_from_container(this_object());
  }
  if(clone) {
    for(index = 0; index < sizeof(objects); index++) {
      objects[index]->set_location(nil);
    }
  }
}

void upgraded(varargs int clone) {
  if(SYSTEM() || COMMON()) {

  }
}

object get_location(void) {
  if(!SYSTEM() && !COMMON() && !GAME())
    return nil;

  return location;
}

object get_mobile(void) {
  return mobile;
}

/**** Get and Set textual descriptions of the object ****/

object get_brief(void) {
  if(!bdesc && sizeof(archetypes))
    return archetypes[0]->get_brief();

  return bdesc;
}

void set_brief(object brief) {
  bdesc = brief;
}

object get_look(void) {
  if(!ldesc && sizeof(archetypes))
    return archetypes[0]->get_look();

  return ldesc;
}

void set_look(object look) {
  ldesc = look;
}

object get_examine(void) {
  if(!edesc && !ldesc && sizeof(archetypes))
    return archetypes[0]->get_look();

  if(!edesc) return get_look();
  return edesc;
}

void set_examine(object examine) {
  edesc = examine;
}

void clear_examine(void) {
  edesc = nil;
}


int get_number(void) {
  return tr_num;
}

object* get_archetypes(void) {
  return archetypes[..];
}

/* This function is called with a new set of archetypes to use as
   the list of the object's parents.  The list is ordered, with the
   primary archetype listed first and later archetypes listed in
   descending order of priority.  The primary archetype is used
   for purposes such as inheriting a primary description. */
void set_archetypes(object* new_archetypes) {
  if(!SYSTEM() && !COMMON())
    error("Only SYSTEM and COMMON objects may set archetypes!");

  if(!new_archetypes)
    new_archetypes = ({ });

  /* If the two lists contain the same elements, the xor of them
     will be empty.  Otherwise, it won't, and the list of parents
     has changed, not just in order but in content. */
  if(sizeof(new_archetypes ^ archetypes)) {
    removed_details = ({ });
    removed_nouns = ({ });
    removed_adjectives = ({ });
  }

  archetypes = new_archetypes[..];
}

void add_archetype(object new_arch) {
  if(!SYSTEM() && !COMMON())
    error("Only SYSTEM and COMMON objects may set archetypes!");

  archetypes += ({ new_arch });
}

void remove_archetype(object arch_to_remove) {
  error("Not yet implemented!");
}

private atomic void add_remove_noun_adj(object phr, int do_add, int is_noun) {
  int     locale, ctr, ctr2;
  mixed*  taglist;
  string* words;

  if(PHRASED->num_locales() > sizeof(nouns)) {
    error("Fix objects to support dynamic adding of locales!");
  }

  taglist = phr->as_taglist();
  if(!taglist) return;

  if(is_noun)
    unregister_my_nouns();
  else
    unregister_my_adjs();

  locale = LANG_enUS;

  for(ctr = 0; ctr < sizeof(taglist); ctr += 2) {
    string *cur_words;

    if(taglist[ctr] != "") {
      switch(taglist[ctr][0]) {
      case '{':
        locale = PHRASED->language_by_name(taglist[ctr][1..]);
        if(locale == -1)
          error("Bad language tag " + taglist[ctr][1..]
                + " passed when modifying nouns/adjs!");
        break;
      case '}':
        locale = LANG_enUS;
        break;
      default:
        error("Illegal phrase passed when modifying nouns/adjs!");
      }
    }

    /* Now modify nouns or adjs */
    if(is_noun)
      cur_words = get_nouns(locale);
    else
      cur_words = get_adjectives(locale);

    words = explode(taglist[ctr + 1], ",");

    for(ctr2 = 0; ctr2 < sizeof(words); ctr2++) {
      words[ctr2] = STRINGD->trim_whitespace(words[ctr2]);

      if(words[ctr2] && words[ctr2] != "") {
        if(do_add) {
          if(is_noun)
            removed_nouns[locale] -= ({ words[ctr2] });
          else
            removed_adjectives[locale] -= ({ words[ctr2] });

          /* If no parent defines this already, add it to this
             object */
          if(is_noun && !sizeof(cur_words & ({ words[ctr2] }))) {
            nouns[locale] += ({ words[ctr2] });
          } else if(!is_noun && !sizeof(cur_words & ({ words[ctr2] }))) {
            adjectives[locale] += ({ words[ctr2] });
          }
        } else {
          if(is_noun)
            nouns[locale] -= ({ words[ctr2] });
          else
            adjectives[locale] -= ({ words[ctr2] });

          /* If a parent defines this, put it into the 'removed' list */
          if(is_noun && sizeof(cur_words & ({ words[ctr2] }))) {
            removed_nouns[locale] += ({ words[ctr2] });
          } else if(!is_noun && sizeof(cur_words & ({ words[ctr2] }))) {
            removed_adjectives[locale] += ({ words[ctr2] });
          }
        }
      }
    }
  }

  if(is_noun)
    register_my_nouns();
  else
    register_my_adjs();
}

void add_noun(object phr) {
  add_remove_noun_adj(phr, 1, 1);
}

void remove_noun(object phr) {
  add_remove_noun_adj(phr, 0, 1);
}


void clear_nouns(void) {
  int ctr, num_loc;

  if(nouns)
    unregister_my_nouns();

  num_loc = PHRASED->num_locales();
  nouns = allocate(num_loc);
  removed_nouns = allocate(num_loc);

  for(ctr = 0; ctr < num_loc; ctr++) {
    nouns[ctr] = ({ });
    removed_nouns[ctr] = ({ });
  }
}


string* get_nouns(int locale) {
  string *ret;
  int     ctr;

  if(locale < 0 || locale >= sizeof(nouns))
    return nil;

  ret = ({ });
  if(sizeof(archetypes)) {
    for(ctr = 0; ctr < sizeof(archetypes); ctr++) {
      ret += archetypes[ctr]->get_nouns(locale);
    }
  }

  return (nouns[locale] + ret) - removed_nouns[locale];
}

string** get_immediate_nouns(void) {
  if(!COMMON() && !SYSTEM())
    return nil;

  return nouns;
}

string* get_removed_nouns(int locale) {
  return removed_nouns[locale][..];
}

string* get_removed_adjectives(int locale) {
  return removed_adjectives[locale][..];
}

string* get_adjectives(int locale) {
  string *ret;
  int     ctr;

  if(locale < 0 || locale >= sizeof(adjectives))
    return nil;

  ret = ({ });
  if(sizeof(archetypes)) {
    for(ctr = 0; ctr < sizeof(archetypes); ctr++) {
      ret += archetypes[ctr]->get_adjectives(locale);
    }
  }

  return (adjectives[locale] + ret) - removed_adjectives[locale];
}

string** get_immediate_adjectives(void) {
  if(!COMMON() && !SYSTEM())
    return nil;

  return adjectives;
}

void add_adjective(object phr) {
  add_remove_noun_adj(phr, 1, 0);
}

void remove_adjective(object phr) {
  add_remove_noun_adj(phr, 0, 0);
}


void clear_adjectives(void) {
  int ctr, num_loc;

  if(adjectives)
    unregister_my_adjs();

  num_loc = PHRASED->num_locales();
  adjectives = allocate(num_loc);
  removed_adjectives = allocate(num_loc);

  for(ctr = 0; ctr < num_loc; ctr++) {
    adjectives[ctr] = ({ });
    removed_adjectives[ctr] = ({ });
  }
}


/* This function takes two string arrays.  The first, called words,
   usually corresponds to user input.  The second, array, usually
   corresponds to nouns or adjectives of an object, or other set
   of words to be matched.  This function returns true if every
   string in "words" is a prefix of a string in "array".  The same
   word in "array" may be matched by multiple entries of "words"
   with no problems, and potentially vice-versa. */
private int match_str_array(string* words, string* array) {
  int ctr, ctr2, match;

  for(ctr = 0; ctr < sizeof(words); ctr++) {
    match = 0;

    for(ctr2 = 0; ctr2 < sizeof(array); ctr2++) {
      if(STRINGD->prefix_string(words[ctr], array[ctr2])) {
        match = 1;
        break;
      }
    }

    if(!match) {
      return 0;
    }
  }

  return 1;
}


/* This function takes a user object, a list of adjectives and a
   list of nouns.  The user object is used to get the locale for
   the word matching.  The function returns true if the given list
   of nouns matches nouns from this object, and the same is true of
   the adjectives.  The matching criteria are according to
   match_str_array, documented above. */
private int match_adj_and_nouns(object user, string *cmp_adjectives,
                                string* cmp_nouns) {
  int locale, match;

  locale = user->get_locale();

  match = match_str_array(cmp_nouns, nouns[locale]);
  if(!match && locale != LANG_englishUS) {
    match = match_str_array(cmp_nouns, nouns[LANG_englishUS]);
  }

  if(!match) return 0;

  match = match_str_array(cmp_adjectives, adjectives[locale]);
  if(!match && locale != LANG_englishUS) {
    match = match_str_array(cmp_adjectives, adjectives[LANG_englishUS]);
  }

  return match;
}

/* Match_words takes a user object (used to get the locale for word
   matching) and a list of words.  All words before the last one are
   assumed to be adjectives, and the last is assumed to be a noun.
   Then the matching is checked with match_adj_and_nouns, above. */
int match_words(object user, string *words) {
  string noun;

  noun = words[sizeof(words) - 1];

  return match_adj_and_nouns(user, words[..sizeof(words) - 2], ({ noun }));
}

/* Match_string separates the given string into words, trims
   appropriately, makes all the words lowercase, and passes the
   results to match_words, above. */
int match_string(object user, string name) {
  string* words;
  int     ctr;

  words = explode(name, " ");

  /* Trim */
  for(ctr = 0; ctr < sizeof(words); ctr++) {
    if(!words[ctr] || STRINGD->is_whitespace(words[ctr])) {
      words = words[..ctr-1] + words[ctr+1..];
    } else {
      words[ctr] = STRINGD->to_lower(STRINGD->trim_whitespace(words[ctr]));
    }
  }

  return match_words(user, words);
}

object* find_contained_objects(object user, string namestr) {
  string* words;
  object* ret;
  int     ctr;

  words = explode(namestr, " ");

  /* Trim */
  for(ctr = 0; ctr < sizeof(words); ctr++) {
    if(!words[ctr] || STRINGD->is_whitespace(words[ctr])) {
      words = words[..ctr-1] + words[ctr+1..];
    } else {
      words[ctr] = STRINGD->to_lower(STRINGD->trim_whitespace(words[ctr]));
    }
  }

  if(sizeof(words) == 0)
    return nil;

  ret = ({ });

  for(ctr = 0; ctr < sizeof(objects); ctr++) {
    if(objects[ctr]->match_words(user, words)) {
      ret += ({ objects[ctr] });
    }
  }

  if(sizeof(ret))
    return ret;

  return nil;
}


/**** Object Description Functions ****/
/* TODO:  locale-ify this */

/* Returns true if the given string describes a single instance of this
   object type. */
int doesDescribeOne(object user, string desc) {
  return match_string(user, desc);
}

/* Returns true if the given string describes several instances of this
   object type. */
int doesDescribeMany(object user, string desc) {

}

/* Returns a phrase describing one instance of this object type, not
   including any articles. */
object describeOne() {

}

/* Returns a phrase describing several instances of this object type.
   The function may choose whether to prepend the exact number or a
   more general quantifier like "several". */
object describeMany(object obj, int num) {

}


/**** Access-Protected Functions ****/

/* kludgy check to make sure we don't detail/contain ourselves */
int superset_of(object sample)
{
	while (sample) {
		if (this_object() == sample) {
			return 1;
		}
	
		sample = sample->get_location();
	}
	
	return 0;
}
/* Container Functions */

/* set_location sets the location variable directly.  Mostly, this shouldn't
   happen.  Instead it should be set with the assorted container commands. */
void set_location(object new_loc) {
  if(previous_program() == OBJECT) {
    /* kludgy check */
    
    if (superset_of(new_loc)) {
    	error("Circular containment attempted");
    }

    if(detail_of && detail_of != new_loc) {
      LOGD->write_syslog("Setting location of obj #" + tr_num
                         + " despite detail_of being set!", LOG_ERROR);
    }
    
    location = new_loc;
  }
}


void add_to_container(object obj) {
  append_to_container(obj);
}

void prepend_to_container(object obj) {
  if(!obj)
    error("Can't add (nil) to a container!");

  if(obj->get_location())
    error("Remove from previous container before adding!");

  if(obj->get_detail_of())
    error("Can't add a detail to a container!");

  obj->set_location(this_object());
  if(obj->get_mobile()) {
    mobiles += ({ obj->get_mobile() });
    obj->get_mobile()->notify_moved(obj);
  }

  objects = ({ obj }) + objects;
}

void append_to_container(object obj) {
  if(!obj)
    error("Can't add (nil) to a container!");

  if(obj->get_location())
    error("Remove from previous container before adding!");

  if(obj->get_detail_of())
    error("Can't add a detail to a container!");

  obj->set_location(this_object());
  if(obj->get_mobile()) {
    mobiles += ({ obj->get_mobile() });
    obj->get_mobile()->notify_moved(obj);
  }

  objects += ({ obj });
}

void remove_from_container(object obj) {
  if(obj->get_location() != this_object()) {
    error("Trying to remove object from wrong container!");
  }

  if(obj->get_detail_of()) {
    error("Trying to remove detail with remove_from_container!");
  }

  if(objects & ({ obj }) ) {
    objects -= ({ obj });
  } else {
    LOGD->write_syslog("Can't remove object from container!", LOG_ERR);
  }

  obj->set_location(nil);
  if(obj->get_mobile()) {
    mobiles -= ({ obj->get_mobile() });
    obj->get_mobile()->notify_moved(obj);
  }
}

object object_num_in_container(int num) {
  if(num < 0 || num >= sizeof(objects))
    return nil;

  return objects[num];
}

mixed* objects_in_container(void) {
  return objects[..];
}

mixed* mobiles_in_container(void) {
  return mobiles[..];
}

int num_objects_in_container(void) {
  return sizeof(objects);
}



/* Functions for MOBILE use */

nomask void set_mobile(object new_mob) {
  if(previous_program() == MOBILE || previous_program() == MOBILED) {
    if(detail_of) {
      error("A mobile can't (yet?) inhabit a detail!");
    }

    if(mobile && location) {
      /* Remove the mobile from its container, if any */
      location->remove_mobile(mobile);
    }
    mobile = new_mob;
    if(new_mob && location) {
      /* Add the mobile to its container, if any */
      location->add_mobile(new_mob);
    }
  } else {
    error("You can't set that from there!");
  }
}


/* Detail functions */

object* get_details(void) {
  object* parent_details;
  int     ctr;

  parent_details = ({ });

  if(archetypes) {
    for(ctr = 0; ctr < sizeof(archetypes); ctr++) {
      parent_details += archetypes[ctr]->get_details();
    }
  }

  parent_details -= removed_details;

  return parent_details + details;
}

/* This returns only the details that are specific to this
   object, not any that are inherited. */
object* get_immediate_details(void) {
  if(!COMMON() && !SYSTEM() && !GAME())
    error("Only privileged code can see immediate_details for an object!");

  if(details && sizeof(details)) {
    return details[..];
  } else {
    return nil;
  }
}

object *get_removed_details(void) {
  if(removed_details && sizeof(removed_details)) {
    return removed_details[..];
  } else {
    return nil;
  }
}

void set_removed_details(object *new_removed_details) {
  if(!SYSTEM() && !COMMON())
    error("Only SYSTEM or COMMON objects can set removed_details!");

  if(!new_removed_details)
    new_removed_details = ({ });

  removed_details = new_removed_details[..];
}

object get_detail_of(void) {
  return detail_of;
}

object set_detail_of(object obj) {
  if(previous_program() != OBJECT)
    error("Only OBJECTs can set details with set_detail_of!");

  if(location && obj)
    error("Remove from container before using set_detail_of!");

  if(obj)
    set_location(obj);
  
  detail_of = obj;
}

void add_detail(object obj) {
  if(!SYSTEM() && !COMMON())
    error("Only SYSTEM and COMMON objects may add details!");

  if(sizeof(removed_details & ({ obj }))) {
    /* Our parent has this detail and we overrode it.  So we'll
       remove the override. */
    removed_details -= ({ obj });

    if(obj->get_detail_of()) {
      /* This is still a detail of somebody else, probably our
         parent (directly or indirectly).  We already removed the
         override, it's not legal to make it our own detail when
         it's somebody else's, so let's just return. */
      return;
    }
  }

  obj->set_detail_of(this_object());
  details += ({ obj });
}


void remove_detail(object obj) {
  if(!SYSTEM() && !COMMON())
    error("Only SYSTEM and COMMON objects may remove details!");

  /* TODO:  go through all children (in the archetype sense) and
     remove this detail from their removed_detail lists? */

  if( !sizeof(details & ({ obj })) ) {
    object *full_details;

    /* This isn't one of our immediate details.  Check to see if
       we should add it to the removed_details array. */

    if(sizeof(archetypes))
      full_details = get_details();

    if(full_details && sizeof(full_details & ({ obj })) > 0) {
      removed_details += ({ obj });
      return;
    }

    error("You can't remove a detail that isn't in this object!");
  }

  obj->set_detail_of(nil);
  details -= ({ obj });
}


/* Registration of nouns and adjectives with ParseD */

/*
 * ParseD needs to know what words are valid nouns and adjectives in
 * the system, and that requires knowing what nouns and adjectives
 * apply to objects in the system.  We can't directly register and
 * unregister nouns and adjectives because multiple objects will use
 * them (think 'green' or 'rock' or 'sword').  So ParseD will refcount
 * them for us, but we have to ref or unref them when we add or
 * remove nouns and adjectives.
 *
 * Objects with parents are responsible for only their *own* nouns and
 * adjectives, not those of their parent objects.  Since the parent
 * must exist to be valid, the parent nouns and adjectives must
 * already be referenced.  Similarly, the removed_XXX arrays have no
 * effect on referencing of nouns and adjectives.
 *
 * Currently, ParseD is only in English.  These ref and unref
 * functions can update non-English just fine, but we'll need to make
 * them do it when/if we add a non-English language that requires it.
 *
 */

private void register_my_nouns(void) {
  int ctr;

  for(ctr = 0; ctr < sizeof(nouns[LANG_englishUS]); ctr++) {
    PARSED->ref_noun(nouns[LANG_englishUS][ctr]);
  }
}

private void unregister_my_nouns(void) {
  int ctr;

  for(ctr = 0; ctr < sizeof(nouns[LANG_englishUS]); ctr++) {
    PARSED->unref_noun(nouns[LANG_englishUS][ctr]);
  }
}

private void register_my_adjs(void) {
  int ctr;

  for(ctr = 0; ctr < sizeof(adjectives[LANG_englishUS]); ctr++) {
    PARSED->ref_adj(adjectives[LANG_englishUS][ctr]);
  }
}

private void unregister_my_adjs(void) {
  int ctr;

  for(ctr = 0; ctr < sizeof(adjectives[LANG_englishUS]); ctr++) {
    PARSED->unref_adj(adjectives[LANG_englishUS][ctr]);
  }
}


/* Functions for use by MAPD, EXITD, etc - overridden in child classes */

nomask void set_number(int num) {
  string prog;

  prog = previous_program();
  if(prog == MAPD || prog == EXITD) {
    tr_num = num;
  } else {
    error("Program " + prog + " not authorized to set object numbers!");
  }
}


/* Function for use by other OBJECTs */

void remove_mobile(object rem_mob) {
  if(previous_program() == OBJECT) {
    if(mobiles & ({ rem_mob }) ) {
      mobiles -= ({ rem_mob });
    }
  } else {
    error("Only another OBJECT may remove mobiles directly!");
  }
}

void add_mobile(object add_mob) {
  if(previous_program() == OBJECT) {
    mobiles += ({ add_mob });
  } else {
    error("Only another OBJECT may remove mobiles directly!");
  }
}
