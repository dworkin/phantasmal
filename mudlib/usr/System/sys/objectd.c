/* Released into the public domain Jan 2002 by Noah Gibbs.
   Everything in this file is in the public domain, and is
   released without warranty, guarantee, or even the implication
   of marketability or fitness for a particular purpose.
   If you start a war in Asia with this, don't come crying to
   me. */

/* Thanks go to Geir Harald Hansen, who released an objectd well
   before I knew anything about DGD.  Studying his work has been
   valuable, as has correspondence (his and others) on and with the
   DGD Mailing List.  This objectd has somewhat different goals than
   his, and operates in some very different (and some very similar)
   ways.  See also the doc/design/OBJECTD document for details. */

#include <config.h>
#include <status.h>
#include <type.h>
#include <log.h>
#include <trace.h>

#include <kernel/kernel.h>
#include <kernel/objreg.h>
#include <kernel/rsrc.h>


/* TODO:  (features)
   - Allow recompiles that destruct and rebuild all necessary stuff
     - By specifying a list of objects to (re)compile
     - By specifying a list of files that changed
       - Allow to add manually to dependency list?
       - Compile list automatically by time, or just specify time?
*/


inherit objreg API_OBJREG;
inherit rsrc API_RSRC;


/* With aggro_recompile when a parent is found that we don't have an
   issue for, we'll recompile it.  That'll keep all the issue IDs the
   same and stuff, but means we'll have an issue for it.
*/
private int     aggro_recompile;

/* Keep track of recomp_paths and all_libs to see what to recompile
   during init's aggressive recompile. */
private mapping recomp_paths, all_libs;

/* For compiling(), include(), compiled_failed(), etc we need to
   track files.  This mechanism attempts to track what's being
   compiled. */
private string last_file;
private mixed* comp_dep;

/* Used when initializing objectd */
private int setup_done;

/* Should be private so other objects can't get objects by issue --
   that'll be important for security reasons.  These heavy arrays should
   have absolutely no exported references anywhere outside this object,
   ever. */
private object obj_issues;

/* This mapping contains objects whose latest issue is destroyed.  It
   is indexed by object name. */
private mapping dest_issues;

/* Array of objects to upgrade -- normally there should be no more than
   one object here at a time unless a previous call_out has failed... */
private object* upgrade_clonables;
private int     upgrade_callout;

/* Mapping of directories and AUTO objects.  If an object has one of
   these directories as a prefix, it will use the specified AUTO
   object. */
private mapping auto_objects;

private void   unregister_inherit_data(object issue);
private void   register_inherit_data(object issue);
private void   call_upgraded(object obj);
private object add_clonable(string owner, object obj, string* inherited);
private object add_lib(string owner, string path, string* inherited);
        string destroyed_obj_list(void);
private void   suspend_system(void);
private void   release_system(void);


static void create(varargs int clone)
{
  object test;

  objreg::create();
  rsrc::create();

  if(clone) {
    error("Attempting to clone objectd!");
  }

  /* Make a new heavy array to hold the object issues */
  if(!find_object(HEAVY_ARRAY))
    compile_object(HEAVY_ARRAY);

  obj_issues = clone_object(HEAVY_ARRAY);
  dest_issues = ([ ]);
  setup_done = 0;

  aggro_recompile = 0;
  recomp_paths = ([ ]);
  all_libs = ([ ]);
  comp_dep = ({ });
  upgrade_clonables = ({ });
  auto_objects = ([ ]);

  if(!find_object(ISSUE_LWO))
    compile_object(ISSUE_LWO);
  if(!find_object(CLONABLE_LWO))
    compile_object(CLONABLE_LWO);
  if(!find_object(LIB_LWO))
    compile_object(LIB_LWO);
}

void destructed(int clone) {
  if(!SYSTEM())
    return;

  destruct_object(obj_issues);
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
      /* Make sure this gets an entry in the fixups array so we
	 know to fix up the reference to the string afterward. */
      if(!fixups[keys[ctr]]) fixups[keys[ctr]] = ({ });
      fixups[keys[ctr]] += recomp_paths[keys[ctr]];
      recomp_paths[keys[ctr]] = nil;

      /* Destroy & Recompile lib */
      destruct_object(keys[ctr]);
      compile_object(keys[ctr]);
    }
  }
}

/* Used during init.  We've recompiled all the libs, now we need to
   make sure that stuff compiled before had issue objects for the libs
   have the right things in their parent arrays. */
/* TODO: add fixup to child arrays?  Or make obsolete? */
private void fix_parent_arrays(mapping fixups) {
  mixed* keys;
  int    ctr;

  keys = map_indices(fixups);
  for(ctr = 0; ctr < sizeof(keys); ctr++) {
    int    ctr2;
    mixed* paths;
    int    lib_index;

    /* For each library to fix up... */
    lib_index = status(keys[ctr])[O_INDEX];

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
      if(!issue) {
	LOGD->write_syslog("Can't get parent issue for " + path, LOG_ERR);
	continue;
      }
      inh = issue->get_parents();
      for(ctr3 = 0; ctr3 < sizeof(inh); ctr3++) {
	if(inh[ctr3] == keys[ctr]) {
	  inh[ctr3] = lib_index;
	}
      }
    }
  }
}

/* Called during init to fix up child arrays and clear old issues out
   of prev pointers. */
private void fix_child_arrays(string* owners) {
  mixed* keys;
  int    ctr, status_idx;
  object issue, index, first;
  string owner;

  keys = map_indices(all_libs);
  for(ctr = 0; ctr < sizeof(keys); ctr++) {
    issue = obj_issues->index(all_libs[keys[ctr]]);
    if(!issue) {
      LOGD->write_syslog("Can't find issue for index " + all_libs[keys[ctr]],
			 LOG_ERR_FATAL);
      error("Can't find issue for index " + all_libs[keys[ctr]]);
    }
    issue->clear_prev();
    issue->clear_children();
  }
  for(ctr = 0; ctr < sizeof(keys); ctr++) {
    issue = obj_issues->index(all_libs[keys[ctr]]);
    if(!issue) {
      LOGD->write_syslog("Can't find issue for index " + all_libs[keys[ctr]],
			 LOG_ERR_FATAL);
      error("Can't find issue for index " + all_libs[keys[ctr]]);
    }
    register_inherit_data(issue);
  }

  /* Iterate through all owners to make sure prev fields are clear and
     to register their inherit data. */
  for(ctr = 0; ctr < sizeof(owners); ctr++) {
    owner = owners[ctr];

    first = objreg::first_link(owner);
    if(!first) continue;  /* No objects for this owner... */

    index = first;
    do {
      status_idx = status(index)[O_INDEX];
      issue = obj_issues->index(status_idx);
      if(!issue || issue->get_prev() != -1) {
	LOGD->write_syslog("Can't verify issue in get_child_arrays!",
			   LOG_ERR_FATAL);
	error("Internal error!");
      }
      register_inherit_data(issue);

      index = objreg::next_link(index);
    } while(index != first);
  }
}

/* Used during init.  Recompile every clonable object */
private void recompile_every_clonable(string* owners) {
  string  owner;
  int     ctr, ctr2;
  object  index, first;
  object* obj_arr;

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

      /* Skip all clones -- we're just getting dependencies */
      if(sscanf(name, "%*s#%*d"))
	continue;

      compile_object(name);
    }
  }
}

/* Used during init.  Recompile every clonable so we know the issue
   info for each one.  While we're doing it, collect inheritance info
   so we know what they inherit from.  Then, recompile all libraries
   to track them properly. */
private void recompile_every_object(string* owners) {
  mapping fixups;

  /* set aggressive recompiles -- need to find all libraries */
  aggro_recompile = 1;

  /* Recompile all the clonables so we can get a first set of libs */
  recompile_every_clonable(owners);

  /* Now recompile all the libs we found while compiling but haven't started
     tracking yet */
  fixups = ([ ]);
  recompile_to_track_libs(fixups);
  fix_parent_arrays(fixups);
  fix_child_arrays(owners); /* Uses all_libs var */

  /* Add faked AUTO object */
  add_lib("System", AUTO, ({ }));

  /* Set aggro_recompile mode to strong checking -- anything that
     would initiate an aggressive recompile before should be outlawed
     now since we should already know everything we need to know about
     all objects.  This also turns off recomp_paths registration, so
     we can get rid of it. */
  aggro_recompile = 2;
  recomp_paths = nil;
}

/* Used during init.  Objregd keeps track of all clones, so we can count
   them pretty easily. */
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
  mixed* parents;
  int    ctr, index;
  object inh_issue;

  index = issue->get_index();
  parents = issue->get_parents();
  for(ctr = 0; ctr < sizeof(parents); ctr++) {
    if(typeof(parents[ctr]) == T_STRING) {
      if(aggro_recompile > 1) {
	LOGD->write_syslog("Uncorrected parent string!", LOG_ERR);
      }
    } else {
      inh_issue = obj_issues->index(parents[ctr]);
      if(!inh_issue) {
	LOGD->write_syslog("Internal error (" + issue->get_name()
			   + "), Parent: " + parents[ctr]
			   + ", Child: " + index, LOG_ERR_FATAL);
	error("Internal error!");
      }

      inh_issue->remove_child(index);
    }
  }
}

/* This removes this issue from the parent lists of everybody that
   used to inherit from it.  Currently only used in remove_program. */
private void clear_child_data(object issue) {
  mixed* children;
  int    ctr, index;
  object inh_issue;

  index = issue->get_index();
  children = issue->get_children();
  if(!children) {
    LOGD->write_syslog("Should children be NULL for issue #" +
		       index + " in clear_child_data?",
		       LOG_WARNING);
    return;
  }
  for(ctr = 0; ctr < sizeof(children); ctr++) {
    inh_issue = obj_issues->index(children[ctr]);
    if(!inh_issue) {
      LOGD->write_syslog("Internal error, Child: " + children[ctr]
			 + ", Parent: " + index, LOG_ERR_FATAL);
      error("Internal error!");
    }

    inh_issue->remove_parent(index);
  }
}

/* This adds the issue to the child lists of everybody it now
   inherits from. */
private void register_inherit_data(object issue) {
  mixed* parents;
  int    ctr, index;
  object tmp;

  index = issue->get_index();
  parents = issue->get_parents();
  for(ctr = 0; ctr < sizeof(parents); ctr++) {
    if(typeof(parents[ctr]) == T_STRING) {
      if(aggro_recompile > 1) {
	LOGD->write_syslog("Uncorrected parent string!", LOG_ERR);
      }
    } else if(typeof(parents[ctr]) == T_NIL) {
      LOGD->write_syslog("Parents[" + ctr + "] is nil for issue #" + index
			 + "!", LOG_ERR);
    } else {
      tmp = obj_issues->index(parents[ctr]);
      if(!tmp) {
	LOGD->write_syslog("Can't get parent issue from index!", LOG_ERR);
      } else {
	tmp->add_child(index);
      }
    }
  }
}

/* Transfer clones from an old object issue to a new one */
private void transfer_clones(object old_issue, object new_issue) {
  if(old_issue->get_num_clones() > 0) {
    new_issue->clones_from(old_issue);
  }

  if(old_issue->get_prev() != -1) {
    transfer_clones(obj_issues->index(old_issue->get_prev()), new_issue);
  }
}

private void convert_inherited_str_to_mixed(string *inherited, mixed* inh_obj,
					    string path) {
  int    tmp_idx, ctr;
  object tmp_issue;

  /* Convert inherited (an array of strings) to inh_obj (an array of
     objects) */
  for(ctr = 0; ctr < sizeof(inherited); ctr++) {
    /* Get latest issue of obj inherited[ctr] */
    tmp_idx = status(inherited[ctr])[O_INDEX];
    tmp_issue = obj_issues->index(tmp_idx);

    if(tmp_issue) {
      inh_obj[ctr] = tmp_idx;

      /* During init or later */
      if(aggro_recompile == 1) {
	all_libs[inherited[ctr]] = tmp_idx;
      }
    } else {
      if(aggro_recompile > 1) {
	LOGD->write_syslog("Unrecognized lib " + inherited[ctr]
			   + " with strong recompile check on!",
			   LOG_ERR);
      }
      if(aggro_recompile == 1) {
	if(!recomp_paths[inherited[ctr]]) recomp_paths[inherited[ctr]] = ({ });
	recomp_paths[inherited[ctr]] += ({ path });
      }
      inh_obj[ctr] = inherited[ctr];
    }
  }
}

private object add_clonable(string owner, object obj, string* inherited) {
  object new_issue, old_version, tmp;
  int    idx, ctr, old_index;
  mixed* inh_obj;

  if(!obj) {
    LOGD->write_syslog("Nil object passed to add_clonable!", LOG_ERR);
    return nil;
  }

  if(sscanf(object_name(obj), "%*s#%*d")) {
    LOGD->write_syslog("Clone passed to add_clonable!", LOG_ERR);
    return nil;
  }

  inh_obj = allocate(sizeof(inherited));
  convert_inherited_str_to_mixed(inherited, inh_obj, object_name(obj));

  idx = status(obj)[O_INDEX];

  old_index = -1;
  old_version = obj_issues->index(idx);
  if(old_version) {
    old_index = idx;
  } else if(dest_issues[object_name(obj)]) {
    old_index = dest_issues[object_name(obj)];
    old_version = obj_issues->index(old_index);
    if(!old_version) {
      LOGD->write_syslog("Can't get issue# for destroyed version!", LOG_ERR);
    }
    if(old_version && !old_version->destroyed()) {
      LOGD->write_syslog("Old issue is in dest_issues but not destroyed!",
			 LOG_WARN);
    }
  }

  if(old_version && !old_version->destroyed()) {
    /* Recompile - owner, obj, idx, and old_version stay the
       same, but inheritance may have changed. */

    unregister_inherit_data(old_version);

    old_version->set_parents(inh_obj);
    register_inherit_data(old_version);

    LOGD->write_syslog("Upgrading object on recompile", LOG_VERBOSE);

    call_upgraded(find_object(object_name(obj)));
  } else {
    /* new object or old one was destructed */
    new_issue = new_object(CLONABLE_LWO);

    /* The clones will all be updated at the end of this thread, so
       we don't actually need a previous version around. */
    new_issue->set_vals(owner, obj, idx, inh_obj, comp_dep);
    comp_dep = nil;

    obj_issues->set_index(idx, new_issue);
    register_inherit_data(new_issue);

    /* When we recompile this, it'll upgrade the old clones to the new
       version -- so the destroyed one should go away. */
    if(old_version) {
      /* Transfer clones over */
      transfer_clones(old_version, new_issue);

      /* Remove from dest array */
      dest_issues[object_name(obj)] = nil;
      dest_issues[old_index] = nil;
    }

  }

  /* If we're making a new object, that means that this issue# shouldn't
     be in the destructed issues. */
  if(dest_issues[idx]) {
    LOGD->write_syslog("Clearing destructed issue# which should be clean!",
		       LOG_WARN);
  }
  dest_issues[idx] = nil;

  return new_issue;
}

private object add_lib(string owner, string path, string* inherited) {
  object new_issue, tmp_obj, old_version;
  int    idx, ctr, tmp_idx, old_index;
  mixed* inh_obj;

  inh_obj = allocate(sizeof(inherited));
  convert_inherited_str_to_mixed(inherited, inh_obj, path);

  idx = status(path)[O_INDEX];
  all_libs[path] = idx;
  old_index = -1;
  old_version = obj_issues->index(idx);
  if(old_version) {
    old_index = idx;
  } else if(dest_issues[path]) {
    old_index = dest_issues[path];
    old_version = obj_issues->index(old_index);
    if(!old_version) {
      LOGD->write_syslog("Can't get issue for old_version adding lib!",
			 LOG_ERR);
    }
  }

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
    new_issue->set_vals(owner, path, idx, inh_obj,
			comp_dep, old_index);
    comp_dep = nil;

    obj_issues->set_index(idx, new_issue);
    register_inherit_data(new_issue);

    /* If we have a prev version then it's kept track of by the new
       issue so we can remove it from dest_issues. */
    if(old_version) {
      dest_issues[path] = nil;
      dest_issues[old_index] = nil;
      dest_issues[idx] = nil;
    }
  }

  return new_issue;
}


private void call_upgraded(object obj) {

  if(!obj)
    LOGD->write_syslog("Calling call_upgraded on nil!", LOG_ERR);

  /* Originally we checked to see if the object had an "upgraded"
     function and if so, we scheduled a callout.  That doesn't work
     because the object hasn't been recompiled yet so it may have just
     gotten or lost an "upgraded" function.  So we just schedule the
     callout and *then* see if it has that function. */
  upgrade_clonables += ({ obj });

  if(!upgrade_callout) {
    suspend_system();
    upgrade_callout = call_out("do_upgrade", 0.0, obj);
    if(upgrade_callout <= 0) {
      release_system();
      LOGD->write_syslog("Error scheduling upgrade call_out for "
			 + object_name(obj) + "!");
    }
  }
}

static void do_upgrade(object obj) {
  int ctr;

  release_system();
  upgrade_callout = 0;

  for(ctr = 0; ctr < sizeof(upgrade_clonables); ctr++) {
    if(function_object("upgraded", upgrade_clonables[ctr])) {
      catch {

	/* We don't need to call_limited or anything similar here,
	   because it's already called in a call_out, so there
	   are already tick limits.  We'll want to change stuff
	   here if we ever want to "bill" admin characters for
	   upgrading their objects, but right now we don't
	   care. */

	/* upgrade_clonables[ctr]->call_limited("upgraded", 0 ); */

	call_other(upgrade_clonables[ctr], "upgraded", 0);
      } : {
	LOGD->write_syslog("Error in " + object_name(upgrade_clonables[ctr])
			   + "->upgraded()!",
			   LOG_ERR);
      }
    }
  }

  upgrade_clonables = ({ });
}


/*** Hook funcs called after this object is set_object_manager'd ***/

/* Non-lib object has just been compiled - called just before create() */
void compile(string owner, object obj, string source,
	     string inherited...)
{
  if(previous_program() == DRIVER) {
    LOGD->write_syslog("compile: " + object_name(obj)
		       + ", issue #" + status(obj)[O_INDEX], LOG_VERBOSE);

    add_clonable(owner, obj, inherited);
  }
}

/* Inheritable object has just been compiled */
void compile_lib(string owner, string path, string source,
		 string inherited...)
{
  if(previous_program() == DRIVER) {
    LOGD->write_syslog("compile_lib: " + path + " ("
		       + status(path)[O_INDEX] + ")", LOG_VERBOSE);

    add_lib(owner, path, inherited);
  }
}


/* Object has just been cloned - called just before create(1) */
void clone(string owner, object obj)
{
  if(previous_program() == DRIVER) {
    int index;
    object issue;

    LOGD->write_syslog("clone: " + object_name(obj), LOG_VERBOSE);

    index = status(obj)[O_INDEX];
    issue = obj_issues->index(index);
    issue->add_clone(obj);

    if(issue->destroyed() && aggro_recompile > 1)
      LOGD->write_syslog("Clone of destroyed object...  Odd!", LOG_WARN);
  }
}

/* Object (non-lib) is about to be destructed */
void destruct(string owner, object obj)
{
  if(previous_program() == DRIVER) { 
    int    index;
    object issue;
    string objname;

    LOGD->write_syslog("destruct: " + object_name(obj)
		       + ", issue #" + status(obj)[O_INDEX], LOG_VERBOSE);

    index = status(obj)[O_INDEX];
    issue = obj_issues->index(index);

    if(sscanf(object_name(obj), "%s#%*d", objname)) {
      if(issue)
	issue->destroy_clone(obj);

      if(function_object("destructed",obj)) {
	obj->destructed(1);
      }

      return;
    }

    objname = object_name(obj);

    /* Not a clone */
    if(dest_issues[objname] || dest_issues[index]) {
      if(dest_issues[objname])
	LOGD->write_syslog("Name '" + STRINGD->mixed_sprint(objname)
			   + "' already in dest_issues.",
			   LOG_VERBOSE);
      if(dest_issues[index])
	LOGD->write_syslog("Index " + index + " already in dest_issues.",
			   LOG_VERBOSE);

      LOGD->write_syslog("Object is already in dest_issues!", LOG_WARN);
    }

    /* Put into dest_issues */
    dest_issues[objname] = index;
    dest_issues[index] = index;

    /* Mark issue destroyed */
    if(issue)
      issue->destruct();

    if(function_object("destructed", obj)) {
      obj->destructed(0);
    }

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

    LOGD->write_syslog("destruct_lib: " + path + " (" + index + ")",
		       LOG_VERBOSE);

    all_libs[path] = nil;

    /* If the Obj is not registered, that's fine unless we've
       aggro_recompile'd and set stronger checking on. */
    if(!issue && aggro_recompile > 1) {
      LOGD->write_syslog("Can't get issue for lib "
				+ path + " in destruct_lib!", LOG_WARN);
    }
    if(!issue) return;

    if(dest_issues[path] || dest_issues[index]) {
      LOGD->write_syslog("Lib is already in dest_issues!", LOG_ERR);
    }

    dest_issues[path] = index;
    dest_issues[index] = index;

    if(issue)
      issue->destruct();
  }
}

/* Last ref to specific issue removed */
void remove_program(string owner, string path, int timestamp, int index)
{
  if(previous_program() == DRIVER) {
    object issue, cur;
    mixed* status;

    LOGD->write_syslog("remove: " + path + ", issue #" + index, LOG_VERBOSE);

    /* Get current version */
    cur = find_object(path);
    status = status(cur ? cur : path);
    if(status) {
      cur = obj_issues->index(status[O_INDEX]);
    } else {
      if(dest_issues[path]) {
	cur = obj_issues->index(dest_issues[path]);
	if(!cur)
	  LOGD->write_syslog("No current instance of " + path + "!", LOG_ERR);
      }
    }

    if(!cur) {
      LOGD->write_syslog("Removing issueless stray " + path, LOG_ERR);
    }

    issue = obj_issues->index(index);
    if(issue) {
      if(issue->destroyed()) {
	dest_issues[index] = nil;
	dest_issues[path] = nil;
      }
    } else if(dest_issues[path] || dest_issues[index]) {
      LOGD->write_syslog("Issue in dest_issues but not obj_issues!", LOG_ERR);
    }

    if(issue) {
      unregister_inherit_data(issue);

      /* For libraries only, clear child data */
      if(function_object("get_children", issue))
	clear_child_data(issue);
    } else if(aggro_recompile > 1) {
      LOGD->write_syslog("Removing unregistered issue of " + path, LOG_WARN);
    }

    obj_issues->set_index(index, nil);

    /* If no current issue is to be found, this no longer
       belongs in all_libs */
    if(!status(path)) {
      all_libs[path] = nil;
    }

  }
}


/* Object (possibly a lib) is about to be compiled */
void compiling(string path)
{
  if(previous_program() == DRIVER) {
    object obj, issue;
    int    recomp;

    LOGD->write_syslog("compiling: " + path, LOG_VERBOSE);

    /* Set up entry to keep track of objects being compiled */
    last_file = path;
    comp_dep = ({ });

    /* Call upgrading() func to let object know it's being compiled */
    obj = find_object(path);
    if(obj && function_object("upgrading", obj)) {
      catch {
	obj->upgrading();
      } : {
	LOGD->write_syslog("Error in " + object_name(obj) + "->upgrading()",
			   LOG_ERR);
      }
    }
  }
}

/* 'path' about to be included */
void include(string from, string path)
{
  if(previous_program() == DRIVER && aggro_recompile > 0) {
    string trimmed_from;

    LOGD->write_syslog("include " + path + " from " + from,
		       LOG_ULTRA_VERBOSE);

    if(path != "AUTO" && path != "/include/AUTO"
       && sscanf(path, "%*s\.h") == 0)
      LOGD->write_syslog("Including non-header file " + path, LOG_ERR);

    comp_dep += ({ path });
  }
}

/* An attempt to compile the given object (including libs) has failed */
void compile_failed(string owner, string path)
{
  if(previous_program() == DRIVER) {
    comp_dep = nil;

    LOGD->write_syslog("compile_failed: " + path, LOG_VERBOSE);
  }
}

/* Non-System files can inherit from a nonstandard AUTO object if the
   ObjectD returns one.  This is where that happens. */
string path_special(string compiled)
{


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
    LOGD->write_syslog("forbid_inherit: " + path + " from " + from,
		       LOG_ULTRA_VERBOSE);

    /* If we *did* actually forbid something, that would be logged with
       something other than LOG_ULTRA_VERBOSE... */
  }
  return 0;
}


/*** API Funcs called by the system at large ***/

/* Security is handled here by allowing the setup only once, and the
   fact that the operation gives information only to this object. */
void do_initial_obj_setup(void) {
  string* owners;

  /* For object iteration: */
  object  index, first;
  object* obj_arr;

  if(!SYSTEM())
    return;

  if(setup_done) return;
  setup_done = 1;

  owners = rsrc::query_owners();

  recompile_every_object(owners);
  count_clones(owners);
}

/* Gives a list of destroyed object names */
string destroyed_obj_list(void) {
  mixed*  keys;
  int     ctr, idx;
  object  issue;
  string  ret;

  if(!SYSTEM())
    return nil;

  ret = "Objects:\n";
  keys = map_indices(dest_issues);
  for(ctr = 0; ctr < sizeof(keys); ctr++) {
    if(typeof(keys[ctr]) == T_INT) {
      /* Index is num, not name */
      continue;
    }

    ret += "* ";
    idx = dest_issues[keys[ctr]];
    issue = obj_issues->index(idx);
    if(issue) {
      ret += issue->get_name();
    } else {
      ret += "(nil)";
    }
    ret += " (" + idx + ")" + "\n";
  }

  return ret;
}

/* This is secure because it exports only a string.  You can find out
   how many clones an object has, but you can't get their object
   pointers. */
string report_on_object(string spec) {
  object issue;
  string ret;
  int    ctr, lib;
  mixed* arr, status;
  mixed  index;

  if(!SYSTEM())
    return nil;

  if(sscanf(spec, "#%d", index) || sscanf(spec, "%d", index)) {
    issue = obj_issues->index(index);

    if(!issue && dest_issues[index]) {
      LOGD->write_syslog("Issue is in dest_issues but not obj_issues!",
			 LOG_ERR);
    }

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
	return "DGD recognizes obj " + spec + " but objectd does not.\n";
    } else if(!issue) {
      if(dest_issues[spec]) {
	issue = obj_issues->index(dest_issues[spec]);
      } else
	return "DGD doesn't recognize object " + spec + "\n";
    }
  }

  ret = "";

  if(issue->destroyed() && dest_issues[issue->get_index()])
    ret += "Destroyed ";
  else if (issue->destroyed() || dest_issues[issue->get_index()]) {
    if(dest_issues[issue->get_index()]) {
      ret += "(In Dest array)\n";
    } else {
      ret += "(Marked destroyed)\n";
    }
    ret += "Incorrectly destroyed ";
  }

  if(object_name(issue) == (LIB_LWO + "#-1")) {
    ret += "Inheritable\n";
    lib = 1;
  } else if(object_name(issue) == (CLONABLE_LWO + "#-1")) {
    ret += "Clonable\n";
    ret += "  " + issue->get_num_clones() + " clones exist\n";
  } else {
    return "Internal error!";
  }

  if(typeof(issue->get_name()) != T_STRING) {
    error("Issue name isn't a string...");
  }
  ret += "Name: " + issue->get_name() + "\n";
  ret += "Index: " + issue->get_index() + "\n";
  ret += "Comp_time: " + ctime(issue->get_comp_time()) + "\n";
  if(issue->get_prev() != -1) {
    ret += "Previous issue: " + issue->get_prev() + "\n";
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
      ret += "#" + index + " ";
  }
  ret += "\n";

  if(lib) {
    ret += "Its children are: ";
    arr = issue->get_children();
    if(!arr) arr = ({ });
    for(ctr = 0; ctr < sizeof(arr); ctr++) {
      index = arr[ctr];
      ret += "#" + index + " ";
    }
    ret += "\n";
  }

  ret += "Also depends on: ";
  arr = issue->get_dependencies();
  if(!arr) arr = ({ });
  for(ctr = 0; ctr < sizeof(arr); ctr++) {
    index = arr[ctr];
    if(typeof(index) == T_STRING)
      ret += index + " ";
    else
      error("Non-string dependency!");
  }
  ret += "\n";

  return ret;
}

/*
  Suspend_system suspends network input, new logins and callouts
  except in this object.  (idea stolen from Geir Harald Hansen's
  ObjectD).  This will need to be copied to any and every object
  that suspends callouts -- the RSRCD checks previous_object()
  to find out who *isn't* suspended.  TelnetD only suspends
  new incoming network activity.
*/
private void suspend_system() {
  if(SYSTEM()) {
    RSRCD->suspend_callouts();
    TELNETD->suspend_input(0);  /* 0 means "not shutdown" */
  } else
    error("Only privileged code can call suspend_system!");
}

/*
  Releases everything that suspend_system suspends.
*/
private void release_system() {
  if(SYSTEM()) {
    RSRCD->release_callouts();
    TELNETD->release_input();
  } else
    error("Only privileged code can call release_system!");
}


/* Returns a status report on an object (passed in directly or
   by path, object name or issue number).  The report is in
   the following format (offsets 0 to 6):

   ({ issue number (T_INT),
      object name (T_STRING),
      parent array (T_ARRAY) or nil,
      child array (T_ARRAY) or nil,
      num_clones (T_INT) or nil,
      previous issue number (T_INT) or nil,
      object destroyed or not (0 or 1, T_INT)
    })
*/
mixed* od_status(mixed obj_spec) {
  object issue;
  int    index;

  if(!SYSTEM())
    return nil;

  if(typeof(obj_spec) == T_INT) {
    index = obj_spec;
    issue = obj_issues->index(obj_spec);
  } else if(typeof(obj_spec) == T_OBJECT) {
    index = status(obj_spec)[O_INDEX];
    issue = obj_issues->index(obj_spec);
  } else if(typeof(obj_spec) == T_STRING) {
    if(find_object(obj_spec)) {
      index = status(find_object(obj_spec))[O_INDEX];
    } else {
      if(status(obj_spec)) {
	index = status(obj_spec)[O_INDEX];
      } else {
	return nil;
      }
    }
    issue = obj_issues->index(index);
  }

  if(!issue) return nil;

  return ({ index, issue->get_name(), issue->get_parents(),
	      issue->get_children(), issue->get_num_clones(),
	      issue->get_prev() != -1 ? issue->get_prev() : nil,
	      issue->destroyed() });
}

/* Recompiles all objects other than DRIVER.  Output is sent to object
  output using the "message" function.  The user object happens to work
  well with this :-) */
void recompile_auto_object(object output) {
  int    ctr, ctr2;
  mixed* keys, *didnt_dest;

  if(!SYSTEM())
    return;

  /* This will always be logged at this level */
  LOGD->write_syslog("Doing full rebuild...", LOG_NORMAL);

  if(!SYSTEM())
    error("Can't call recompile_auto_object unprivileged!");

  /* Destruct all libs... */
  keys = map_indices(all_libs);
  didnt_dest = ({ });
  ctr2 = 0;
  for(ctr = 0; ctr < sizeof(keys); ctr++) {
    if(status(keys[ctr])) {
      ctr2++;
      destruct_object(keys[ctr]);
    } else {
      didnt_dest += ({ keys[ctr] });
    }
  }
  ctr2++;
  destruct_object(AUTO);

  if(ctr2 != sizeof(keys) + 1) {
    /* In this case, some elements of all_libs are already destructed,
       which violates our assumptions.  Of course, the rebuild will
       probably fix it. */
    LOGD->write_syslog("Couldn't destruct " + sizeof(didnt_dest) + "/"
		       + (sizeof(keys) + 1) + " libs: "
		       + implode(didnt_dest, ", "), LOG_WARN);
  }

  /* Recompile all clonables */
  recompile_every_clonable(rsrc::query_owners());
}
