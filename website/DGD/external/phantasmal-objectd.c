/* Thanks go to Geir Harald Hansen, who released an object manager well
   before I knew anything about DGD.  Studying his work has been valuable,
   as has correspondence (his and others') on and with the DGD Mailing
   List.  This object manager has somewhat different goals than his, and
   operates in some very different (and some very similar) ways.  See
   also the doc/design/OBJECTD document for details. */

#include <config.h>
#include <status.h>
#include <type.h>

#include <kernel/kernel.h>
#include <kernel/objreg.h>
#include <kernel/rsrc.h>

/* TODO:
   - Add child lists to inheritables
   - Start paying proper attention to file dependencies (like includes)
   - Make the compiled-from-string operations different somehow,
     including allowing manually registering file dependencies
   - Allow recompiles that destruct and rebuild all necessary stuff
     - By specifying an object to compile
     - By specifying a list of objects to compile
     - By specifying a list of files that changed
   - Hardcode recompile of AUTO object
   - Add upgrading() and upgraded() methods
   - Add "notify" file dependencies and perhaps check upgraded return
     val to let an object veto being upgraded.
*/

inherit objreg API_OBJREG;
inherit rsrc API_RSRC;

/* The ISSUE_LWO is a structure which contains the info we care about
   for each issue.  That means it stores things like object name,
   issue compile time, and so on.  LWOs may link to each other since
   the group of them exist entirely within the single HEAVY_ARRAY
   (see below).  The ISSUE_LWO is never used directly, but is instead
   used as one of LIB_LWO or CLONABLE_LWO, both of which inherit
   from ISSUE_LWO. */

#define ISSUE_LWO "/lib/lwo/issue"
#define LIB_LWO "/data/lwo/lib_issue"
#define CLONABLE_LWO "/data/lwo/clonable_issue"


/* The HEAVY_ARRAY type is a single heavyweight object which will wind up
   containing all the lightweight objects.  If we kept references to any
   of the LWOs in this object also they'd be copied here at the end of
   thread execution - that means we need to keep all such references
   confined to staying inside the issues object (the HEAVY_ARRAY). */

#define HEAVY_ARRAY "/obj/heavy_array"


/* With aggro_recompile when a parent is found that we don't have an
   issue for, we'll recompile it.  That'll keep all the issue IDs the
   same and stuff, but means we'll have an issue for it.
*/
private int     aggro_recompile;
private mapping recomp_paths;

/* Used when initializing manager */
private int setup_done;

private object log;

/* Should be private so other objects can't get objects by issue --
   that'll be important for security reasons.  These heavy arrays should
   have absolutely no exported references anywhere outside this object,
   ever. */
private object obj_issues;

/* This mapping contains objects whose latest issue is destroyed.  It
   is indexed by object name. */
private mapping dest_issues;

static void create(varargs int clone)
{
  object test;

  objreg::create();
  rsrc::create();

  log = find_object(LOG_MANAGER);

  if(clone) {
    error("Attempting to clone object manager!");
  }

  /* Make a new heavy array to hold the object issues */
  if(!find_object(HEAVY_ARRAY))
    compile_object(HEAVY_ARRAY);

  obj_issues = clone_object(HEAVY_ARRAY);
  dest_issues = ([ ]);
  setup_done = 0;

  aggro_recompile = 0;
  recomp_paths = ([ ]);

  if(!find_object(ISSUE_LWO))
    compile_object(ISSUE_LWO);
  if(!find_object(CLONABLE_LWO))
    compile_object(CLONABLE_LWO);
  if(!find_object(LIB_LWO))
    compile_object(LIB_LWO);
}


/*** Private funcs ***/

/* Used during init.  Recompile all libs we know only by path (not issue
   struct) to get an issue for them. */
private void recompile_to_track_libs(mapping fixups) {
  while(map_sizeof(recomp_paths) > 0) {
    int    ctr;
    mixed* keys;

    keys = map_indices(recomp_paths);
    for(ctr = 0; ctr < sizeof(keys); ctr++) {
      /* Recompile lib */
      if(!fixups[keys[ctr]]) fixups[keys[ctr]] = ({ });
      fixups[keys[ctr]] += recomp_paths[keys[ctr]];
      recomp_paths[keys[ctr]] = nil;

      destruct_object(keys[ctr]);
      compile_object(keys[ctr]);
    }
  }
}

/* Used during init.  We've recompiled all the libs, now we need to
   make sure that stuff compiled before had issue objects for the libs
   have the right things in their parent arrays. */
/* TODO: add fixup to child arrays */
private void fix_parent_arrays(mapping fixups) {
  mixed* keys;
  int    ctr;

  keys = map_indices(fixups);
  for(ctr = 0; ctr < sizeof(keys); ctr++) {
    int ctr2;
    mixed* paths;
    object lib_issue;

    /* For each library to fix up... */
    lib_issue = obj_issues->index(status(keys[ctr])[O_INDEX]);
    if(!lib_issue) error("Can't get issue for lib " + keys[ctr]);
    paths = fixups[keys[ctr]];
    for(ctr2 = 0; ctr2 < sizeof(paths); ctr2++) {
      string path;
      object obj, issue;
      int    ctr3;
      mixed* inh;

      /* For each file to fix the lib for... */
      path = paths[ctr2];
      obj = find_object(path);
      if(obj) {
	issue = obj_issues->index(status(obj)[O_INDEX]);
      } else {
	issue = obj_issues->index(status(path)[O_INDEX]);
      }

      /* Replace the string with the issue in the parent array */
      if(!issue) error("Can't get parent issue!");
      inh = issue->get_parents();
      for(ctr3 = 0; ctr3 < sizeof(inh); ctr3++) {
	if(inh[ctr3] == keys[ctr]) {
	  inh[ctr3] = ({ lib_issue->get_index(), lib_issue->get_mod_count() });
	}
      }
    }
  }

}

/* Used during init.  Recompile every clonable so we know the issue info
   for each one.  While we're doing it, collect inheritance info so we
   know what they inherit from. */
private void recompile_every_clonable(string* owners) {
  string  owner;
  int     ctr, ctr2;
  object  index, first;
  object* obj_arr;
  mapping fixups;

  /* set aggressive recompiles -- need to find all libraries */
  aggro_recompile = 1;

  /* First, for all owners recompile all owned (non-lib) objects. */
  for(ctr = 0; ctr < sizeof(owners); ctr++) {
    owner = owners[ctr];

    first = objreg::first_link(owner);
    if(!first) continue;  /* No objects for this owner... */

    /* Build simple linear object array from circular list in
       objregd */
    obj_arr = ({ first });
    index = objreg::next_link(first);
    while(index != first) {
      obj_arr += ({ index });
      index = objreg::next_link(index);
    }

    /* For this owner, recompile all his objects so we can get their
       dependencies */
    for(ctr2 = 0; ctr2 < sizeof(obj_arr); ctr2++) {
      string name, path;

      name = object_name(obj_arr[ctr2]);

      /* Skip the object manager itself for the nonce
      if(name == OBJECT_MANAGER)
	continue;
      */

      /* Skip all clones -- we're just getting dependencies */
      if(sscanf(name, "%*s#%*d"))
	continue;

      if(sscanf(name, "%s#", path))
	compile_object(path);
      else
	compile_object(name);
    }
  }

  /* Now recompile all the libs we found while compiling but haven't started
     tracking yet */
  fixups = ([ ]);
  recompile_to_track_libs(fixups);

  fix_parent_arrays(fixups);
}

private void count_clones(string* owners) {
  string  owner;
  int     ctr, ctr2, ctr3;
  object  index, first;
  object* obj_arr;
  mapping lib_issues;

  lib_issues = ([ ]);

  /* Now, for all owners go through all objects */
  for(ctr = 0; ctr < sizeof(owners); ctr++) {
    owner = owners[ctr];

    first = objreg::first_link(owner);
    if(!first) continue;  /* No objects for this owner... */

    /* Build simple linear object array from circular list in
       objregd */
    obj_arr = ({ first });
    index = objreg::next_link(first);
    while(index != first) {
      obj_arr += ({ index });
      index = objreg::next_link(index);
    }

    /* For this owner, enumerate all objects */
    for(ctr2 = 0; ctr2 < sizeof(obj_arr); ctr2++) {
      string name, path;
      int    index;
      object issue;
      mixed* inh;

      name = object_name(obj_arr[ctr2]);
      if(sscanf(name, "%s#%*d", path)) {

	/* For clones, add to clonables */
	index = status(find_object(path))[O_INDEX];
	issue = obj_issues->index(index);
	issue->add_clone(obj_arr[ctr2]);
      }
    }
  }
}

/* This removes this issue from the child lists of everybody it
   used to inherit from */
private void unregister_inherit_data(object issue) {
  /* Does nothing until we have the "child" lists... */

}

/* This adds the issue to the child lists of everybody it now
   inherits from. */
private void register_inherit_data(object issue) {
  /* Does nothing until we have the "child" lists... */

}

private void transfer_clones(object old_issue, object new_issue) {
  if(old_issue->get_num_clones() > 0) {
    new_issue->clones_from(old_issue);
  }

  if(old_issue->get_prev()) {
    transfer_clones(old_issue->get_prev(), new_issue);
  }
}

private object find_index_in_dest(int index) {
  mixed* keys;
  int    ctr;

  keys = map_indices(dest_issues);
  for(ctr=0; ctr<sizeof(keys); ctr++) {
    if(dest_issues[keys[ctr]]->get_index() == index)
      return dest_issues[keys[ctr]];
  }
  return nil;
}

private convert_inherited_str_to_mixed(string *inherited, mixed* inh_obj,
				       string path) {
  int    tmp_idx, tmp_mod, ctr;
  object tmp_issue;

  /* Convert inherited (an array of strings) to inh_obj (an array of
     objects) */
  for(ctr = 0; ctr < sizeof(inherited); ctr++) {
    /* Get latest issue of obj inherited[ctr] */
    tmp_idx = status(inherited[ctr])[O_INDEX];
    tmp_issue = obj_issues->index(tmp_idx);

    if(tmp_issue) {
      tmp_mod = tmp_issue->get_mod_count();
      inh_obj[ctr] = ({ tmp_idx, tmp_mod });
    } else
      if(aggro_recompile) {
	if(!recomp_paths[inherited[ctr]]) recomp_paths[inherited[ctr]] = ({ });
	recomp_paths[inherited[ctr]] += ({ path });
	if(aggro_recompile > 1)
	  error("Unrecognized lib " + inherited[ctr]
		+ " with strong recompile check on!");
      }
      inh_obj[ctr] = inherited[ctr];
  }
}

private object add_clonable(string owner, object obj, string* inherited) {
  object new_issue, old_version, tmp;
  int    idx, ctr, old_index;
  mixed* inh_obj;

  if(!obj)
    error("Nil object passed to add_clonable!");

  inh_obj = allocate(sizeof(inherited));
  convert_inherited_str_to_mixed(inherited, inh_obj, object_name(obj));

  idx = status(obj)[O_INDEX];
  old_version = obj_issues->index(idx);
  if(!old_version)
    old_version = dest_issues[object_name(obj)];

  if(old_version && !old_version->destroyed()) {
    /* Recompile - owner, obj, idx, old_version and mod_count stay the
       same, but inheritance may have changed. */
    unregister_inherit_data(old_version);
    old_version->set_parents(inh_obj);
    register_inherit_data(old_version);
  } else {
    /* new object or old one was destructed */
    new_issue = new_object(CLONABLE_LWO);

    /* The clones will all be updated at the end of this thread, so
       we don't actually need a previous version around. */
    new_issue->set_vals(owner, obj, idx, inh_obj, nil, 0);

    obj_issues->set_index(idx, new_issue);
    register_inherit_data(new_issue);

    /* When we recompile this, it'll upgrade the old clones to the new
       version -- so the destroyed one should go away. */
    if(old_version) {
      /* Transfer clones over */
      transfer_clones(old_version, new_issue);

      /* Remove from heavy_array */
      dest_issues[object_name(obj)] = nil;
    }

  }

  return new_issue;
}

private object add_lib(string owner, string path, string* inherited) {
  object new_issue, tmp_obj, old_version;
  int    idx, ctr, tmp_idx, old_index;
  mixed* inh_obj;

  inh_obj = allocate(sizeof(inherited));
  convert_inherited_str_to_mixed(inherited, inh_obj, path);

  idx = status(path)[O_INDEX];
  old_version = obj_issues->index(idx);
  if(!old_version)
    old_version = dest_issues[path];

  if(old_version && !old_version->destroyed()) {
    /* Recompile - owner, path, idx, old_version and mod_count stay the
       same, but inheritance may have changed. */
    unregister_inherit_data(old_version);
    old_version->set_parents(inh_obj);
    register_inherit_data(old_version);
  } else {
    /* Brand new, or old issue was destructed */
    new_issue = new_object(LIB_LWO);
    /* If old one was destructed then give it as the prev version and
       set the mod count one higher than the old one */
    new_issue->set_vals(owner, path, idx, inh_obj, old_version,
			old_version ? old_version->get_mod_count() + 1 : 0);

    obj_issues->set_index(idx, new_issue);
    register_inherit_data(new_issue);

    /* If we have a prev version then it's kept track of by the new
       issue so we can remove it from dest_issues. */
    if(old_version) {
      dest_issues[path] = nil;
    }
  }

  return new_issue;
}


/*** Hook funcs called after this object is set_object_manager'd ***/

/* Non-lib object has just been compiled - called just before create(0) */
void compile(string owner, object obj, string inherited...)
{
  if(previous_program() == DRIVER) {
    add_clonable(owner, obj, inherited);
  }
}

/* Non-lib object is about to be compiled from source string */
void compile_string(string owner, object obj, string source,
		    string inherited...)
{
  if(previous_program() == DRIVER) {
    error("Duplicate code for compile() when it works!");
  }
}

/* Inheritable object has just been compiled */
void compile_lib(string owner, string path, string inherited...)
{
  if(previous_program() == DRIVER) {
    /* Handle dest_issues */
    add_lib(owner, path, inherited);
  }
}

/* Inheritable object has just been compiled from source string */
void compile_lib_string(string owner, string path, string source,
			string inherited...)
{
  if(previous_program() == DRIVER) {
    error("Duplicate code for compile_lib() when it works!");
  }
}

/* Object has just been cloned - called just before create(1) */
void clone(string owner, object obj)
{
  if(previous_program() == DRIVER) {
    int index;
    object issue;

    index = status(obj)[O_INDEX];
    issue = obj_issues->index(index);
    issue->add_clone(obj);
  }
}

/* Object (non-lib) is about to be destructed */
void destruct(string owner, object obj)
{
  if(previous_program() == DRIVER) { 
    int    index;
    object issue;

    LOG_MANAGER->write_syslog("Destruct " + object_name(obj));
    index = status(obj)[O_INDEX];
    issue = obj_issues->index(index);

    if(sscanf(object_name(obj), "%*s#%*d")) {
      if(issue)
	issue->destroy_clone(obj);
      return;
    }

    /* Not a clone */
    if(dest_issues[obj])
      error("Object in both obj_issues and dest_issues!");

    /* Move from obj_issues to dest_issues */
    dest_issues[obj] = issue;
    obj_issues->set_index(index, nil);

    /* Mark issue destroyed */
    if(issue)
      issue->destruct();
  }
}

/* Inheritable object is about to be destructed */
void destruct_lib(string owner, string path)
{
  if(previous_program() == DRIVER) {
    object issue;
    int    index;

    index = status(path)[O_INDEX];
    issue = obj_issues->index(index);

    /* Obj not registered -- that's fine unless we've aggro_recompile'd */
    if(aggro_recompile > 1) {
      LOG_MANAGER->write_syslog("Can't get issue for lib "
				+ path + " in destruct_lib!");
    }
    if(!issue) return;

    if(dest_issues[path])
      error("Lib in both obj_issues and dest_issues!");

    dest_issues[path] = issue;

    if(issue)
      issue->destruct();
  }
}

/* Last ref to program removed */
void remove_program(string owner, string path, int timestamp, int index)
{
  if(previous_program() == DRIVER) {
    object issue;

    LOG_MANAGER->write_syslog("Last ref to " + path + "("
			      + index + ") removed.");

    issue = obj_issues->get_index(index);
    obj_issues->set_index(index, nil);

    if(!issue) {
      issue = dest_issues[path];
      dest_issues[path] = nil;
    } else if(dest_issues[path]) {
      error("Shouldn't have an object both around and destroyed!");
    }

    if(issue) {
      unregister_inherit_data(issue);
    }
  }
}

/* Object (possibly a lib) is about to be compiled */
void compiling(string path)
{
  if(previous_program() == DRIVER) {

  }
}

/* 'path' about to be included */
void include(string from, string path)
{
  if(previous_program() == DRIVER) {

  }
}

/* An attempt to compile the given object (including libs) has failed */
void compile_failed(string owner, string path)
{
  if(previous_program() == DRIVER) {

  }
}

/* #include "AUTO" special case */
string path_special(string compiled)
{
  if(previous_program() == DRIVER) {

  }
  return "";
}

/* Return true if path is not a legal first arg for call_other */
int forbid_call(string path)
{
  if(previous_program() == DRIVER) {

  }
  return 0;
}

/* Return true to forbid 'from' from inheriting 'path', privately or no
   according to 'priv' */
int forbid_inherit(string from, string path, int priv)
{
  if(previous_program() == DRIVER) {

  }
  return 0;
}


/*** Funcs called by the system for information (and helpers) ***/

/* Security is handled here by allowing the setup only once, and the
   fact that the operation gives information only to this object. */
void do_initial_obj_setup(void) {
  string* owners;

  /* For object iteration: */
  object  index, first;
  object* obj_arr;

  if(setup_done) return;
  setup_done = 1;

  owners = rsrc::query_owners();

  recompile_every_clonable(owners);
  count_clones(owners);
}

private object issue_for_string(string spec) {
  object obj, issue;
  mixed* status;
  int    index;

  status = status(spec);
  if(status) {
    index = status[O_INDEX];
    issue = obj_issues->index(index);
    if(issue)
      return issue;
  }

  if(find_object(spec)) {
    obj = find_object(spec);
    index = status(obj)[O_INDEX];
    issue = obj_issues->index(index);
    return issue;
  }

  return nil;
}

string destroyed_obj_list(void) {
  string* keys;
  int     ctr;
  string  ret;

  ret = "Objects:\n";
  keys = map_indices(dest_issues);
  for(ctr = 0; ctr < sizeof(keys); ctr++) {
    ret += "* " + keys[ctr] + "\n";
  }

  return ret;
}

/* This is secure because it exports only a string.  You can find out
   how many clones an object has, but you can't get their object
   pointers. */
string report_on_object(string spec) {
  object issue;
  string ret;
  int    ctr;
  mixed* arr, status;
  mixed  index;

  if(sscanf(spec, "#%d", index)) {
    issue = obj_issues->index(index);

    if(!issue)
      issue = find_index_in_dest(index);

    if(!issue)
      return "Issue #" + index + " isn't tracked by objectd\n";
  } else {
    status = status(spec);
    if(status) {
      index = status[O_INDEX];
      issue = obj_issues->index(index);
      if(!issue && !find_object(spec))
	return "DGD tracks " + spec + " (#" + index
	  + ") but objectd does not.\n";
    }
    if(find_object(spec)) {
      index = status(find_object(spec))[O_INDEX];
      issue = obj_issues->index(index);
      if(!issue)
	return "DGD recognizes obj " + spec + " *2* but objectd does not.\n";
    } else if(!issue) {
      if(dest_issues[spec])
	issue = dest_issues[spec];
      else
	return "DGD doesn't recognize object " + spec + "\n";
    }
  }

  ret = "";

  if(issue->destroyed())
    ret += "Destroyed ";

  if(object_name(issue) == (LIB_LWO + "#-1")) {
    ret += "Inheritable\n";

  } else if(object_name(issue) == (CLONABLE_LWO + "#-1")) {
    ret += "Clonable\n";
    ret += "  " + issue->get_num_clones() + " clones exist\n";
  } else {
    return "Internal error!";
  }

  if(typeof(issue->get_name()) != T_STRING) {
    error("Name isn't a string...");
  }
  ret += "Name: " + issue->get_name() + "\n";
  ret += "Index: " + issue->get_index() + "\n";
  ret += "Comp_time: " + ctime(issue->get_comp_time()) + "\n";
  if(issue->get_prev()) {
    ret += "Previous issue: " + issue->get_prev()->get_index() + "\n";
  } else {
    ret += "No previous issue\n";
  }

  ret += "Inherits from: ";
  arr = issue->get_parents();
  if(!arr) arr = ({ });
  for(ctr = 0; ctr < sizeof(arr); ctr++) {
    index = arr[ctr];
    if(typeof(index) == T_STRING)
      ret += index + " ";
    else
      ret += index[0] + "/" + index[1] + " ";
  }
  ret += "\n";

  ret += "Also depends on: ";
  arr = issue->get_dependencies();
  if(!arr) arr = ({ });
  for(ctr = 0; ctr < sizeof(arr); ctr++) {
    index = arr[ctr];
    if(typeof(index) == T_STRING)
      ret += index + " ";
    else
      error("Non-string dep!");
  }
  ret += "\n";

  return ret;
}
