/*
 * ~System/sys/objectd.c - object manager
 *
 *        Copyright (C) 1999, 2000 Geir Harald Hansen.
 *        Free for non-commercial use.
 *        For commercial use, contact me.
 *
 *        Send suggestions and bug reports to: geirhans@ifi.uio.no
 *        Many thanks to the helpful people of the DGD mailing list.
 *
 *        This object manager should work on any system based on the DGD
 *        kernel library.  But there are some important things to take note of:
 *
 *        1: The DGD configuration file should set array_size >= objects / 256
 *        (rounded up).
 *
 *        2: Compiling this object should be one of the very first things
 *        done in the initialization daemon ("~System/initd").
 *        More specifically, objectd must know of all objects in existence,
 *        but only searches for inheritables in /kernel/lib/ and
 *        /kernel/lib/api/ when starting.  Therefore objectd must in the least
 *        be compiled before non-kernel inheritables, or errors will result.
 *        Upon creation, objectd will call the driver object to register
 *        itself as the object manager.
 *
 *        3: Read the CONFIGURATION section below and set the defines
 *        as appropriate.
 *
 * CONFIGURATION:
 *        * ONE_VERSION:
 *
 *        If ONE_VERSION is defined, multiple issues of the same object will
 *        not be allowed.  Objects that are inherited or cloned cannot be
 *        destructed before all inheriting objects or clones are.  The
 *        exception to this is the upgrade operation.
 *        This setting can even be changed on a running system:  Define or
 *        undef ONE_VERSION and upgrade the objectd.  This, of course, does
 *        not affect existing objects which may have multiple issues.
 *        ONE_VERSION is not foolproof, see BUGS below.
 *
 *        * UPGRADE_CHUNK:
 *
 *        UPGRADE_CHUNK defines how many objects the objectd may upgrade
 *        in a single thread before giving DGD a chance to breathe and swap
 *        out some objects again.  If there are more objects than this to
 *        upgrade, they are upgraded over several threads, using a callout.
 *        The problem with this is that a failed compilation may cause there
 *        to be multiple issues of an object, even with ONE_VERSION defined.
 *
 *        By undefining UPGRADE_CHUNK, the entire upgrade procedure is made
 *        atomic, which fixes that problem.  This is not recommended, however,
 *        as a system with a swapfile larger than physical memory would run out
 *        of memory when swapping in all objects to upgrade them, for example
 *        during an upgrade of the auto object.  On a system where rooms are
 *        own objects, not clones, upgrading the base room object is another
 *        example of when too many objects would be swapped in during an
 *        upgrade.
 *
 *        Again, it is possible to change the definition of UPGRADE_CHUNK or
 *        remove/add it on a running system and upgrading objectd.
 *
 * BUGS:  Can store at most (status()[ST_ARRAYSIZE] * 256) issues.
 *        This is not a problem as long as the DGD configuration file sets
 *        array_size >= objects / 256.
 *
 *        If there are compilation errors during an upgrade, multiple
 *        versions of an object may exist, even if ONE_VERSION is defined.
 *
 *        When starting up, existing objects will be registered (except
 *        inheritables outside /kernel/, which is one reason to load
 *        objectd early).  But the inheritance structure of these objects
 *        is not known until an upgrade of the auto object is done.  It may
 *        therefore be a good idea to upgrade the auto object after booting
 *        the system fresh (without a statedump), letting objectd discover
 *        the entire inheritance hierarchy correctly.
 *
 */

/* TODO: Use macros for insertion and removal from linked list */
/* TODO: suspend system: also suspend input and logins */
/* TODO: possible to call a function in every clone of every inheritor */
/* TODO: same function call functionality in upgrading */

# define UPGRADE_CHUNK 50 /* max no. of objects to recompile per thread */
# define ONE_VERSION /* allow only one issue of an object to exist at a time */
              /* destruct works only when object has no inheritors or clones */

# include <status.h>
# include <type.h>

# include <kernel/kernel.h>
# include <kernel/objreg.h>
# include <kernel/rsrc.h>

inherit objreg API_OBJREG;
inherit rsrc API_RSRC;

object driver;
int upgrading;      /* upgrade taking place? TRUE or FALSE */
mixed **issues;     /* issue_data = issues[id>>8][id%0xff] */
mapping owned_libs; /* resource owner : (mixed *) linked list, inheritables */
mixed *dested; /* linked list: latest issues of programs without master object.
                  Issues are kept here from the time their master object is
                  destructed until their programs are removed (previous
                  version will be moved to this list) or a new issue
                  is compiled (and links to this old issue). */

/* issue data: */
# define I_PREV           0 /* mixed *:  linked list: older existing issues */
# define I_ID             1 /* int:      issue ID */
# define I_NAME           2 /* string:   object name */
# define I_INHERITS_FROM  3 /* mixed **: issue data of inherited objects */
  /* libs only: */
# define IL_L_INHERITORS  4 /* mixed *:  linked list: inheritable inheritors */
# define IL_O_INHERITORS  5 /* mixed *:  linked list: other inheritors */
# define IL_SIZE          6
  /* non-libs only: */
# define IO_CLONES        4 /* int:      number of existing clones */
# define IO_SIZE          5

/* linked lists: */
# define LL_NEXT   0 /* mixed *: next node in list, nil is terminator */
# define LL_DATA   1 /* mixed: node data */
# define NEW_NODE(next, data) ({ next, data })

# define ISSUE(id)                   issues[id>>8][id%0xff]
# define IS_INHERITABLE(issue)       (sizeof(issue) == IL_SIZE)
# define IS_NON_INHERITABLE(issue)   (sizeof(issue) == IO_SIZE)
# define NEW_INHERITABLE(prev, id, name, inherited) \
                                      ({ prev, id, name, inherited, nil, nil })
# define NEW_NON_INHERITABLE(prev, id, name, inherited) \
                                             ({ prev, id, name, inherited, 0 })

private void new_inheritable(string owner, string objname, string *inherited);
private void new_noninheritable(string owner, object obj, string *inherited);

/*
 * NAME:        create - initialize
 * DESCRIPTION: Initialize object daemon.
 */
static create()
{
  mixed **files;
  string master_name;
  object next, last;
  int i;

  objreg::create();
  rsrc::create();

  issues = ({ });
  owned_libs = ([ ]);
  dested = nil;

  /* we cannot know what existing objects inherit, but register what we know */

  files = get_dir("/kernel/lib/api/*");     /* /kernel/lib/api/ inheritables */
  for (i = sizeof(files[0]); i--; )
    if (files[3][i])
      new_inheritable("System", "/kernel/lib/api/" +
		      files[0][i][ .. strlen(files[0][i]) - 3], ({ }) );

  files = get_dir("/kernel/lib/*");             /* /kernel/lib/ inheritables */
  for (i = sizeof(files[0]); i--; )
    if (files[3][i])
      new_inheritable("System", "/kernel/lib/" +
		      files[0][i][ .. strlen(files[0][i]) - 3], ({ }) );

  next = last = first_link("System");             /* System non-inheritables */
  do {
    if (!sscanf(object_name(next), "%*s#"))
      new_noninheritable("System", next, ({ }));
  } while ((next = next_link(next)) != last);

  new_noninheritable("System", find_object(DRIVER), ({ })); /* driver object */

  next = last = first_link("System");                 /* count System clones */
  do {
    if (sscanf(object_name(next), "%s#", master_name))
      ISSUE(status(next)[O_INDEX])[IO_CLONES]++;
  } while ((next = next_link(next)) != last);

  driver = find_object(DRIVER);
  driver->set_object_manager(this_object());        /* become object manager */

  /* If we are unable to operate safely, let it be known */
  if (status()[ST_ARRAYSIZE] * 256 < status()[ST_OTABSIZE])
    error("Object manager requires " +
	  "array_size >= objects / 256 in DGD configfile");
}

/*
 * NAME:        rem_issue - remove all data about issue
 * DESCRIPTION: Completely unregisters issue <id> owned by <owner>.
 */
private void rem_issue(int id, string owner)
{
  mixed *issue, *latest, node, next;
  int list, i, was_latest;

  /* find and remove from issues array */
  issue = ISSUE(id);
  ISSUE(id) = nil;

  /* if there, remove from dested list */
  if (node = dested)
    if (node[LL_DATA] == issue) {
      dested = node[LL_NEXT];
      was_latest = TRUE;
    } else {
      while ((next = node[LL_NEXT]) && next[LL_DATA] != issue)
	node = next;
      if (next) {
	node[LL_NEXT] = next[LL_NEXT];
	was_latest = TRUE;
      }
    }

  /* keep linked list of versions consistent */
  if (was_latest) { /* found and removed from list */
    if (issue[I_PREV]) /* replace with previous version, if there is one */
      dested = NEW_NODE(dested, issue[I_PREV]);
  } else { /* we are an old version; find the newest and remove us from list */
    mixed *st;

    if (st = status(issue[I_NAME]))
      latest = ISSUE(st[O_INDEX]);
    else {
      node = dested;
      while (node && node[LL_DATA][I_NAME] != issue[I_NAME])
	node = node[LL_NEXT];
      if (node)
	latest = node[LL_DATA];
    }

    /* found latest version, now remove this issue from version list */
    while (latest && latest[I_PREV] != issue)
      latest = latest[I_PREV];
    if (latest)
      latest[I_PREV] = issue[I_PREV];
  }

  /* if inheritable, remove from list of owned inheritables */
  if (IS_INHERITABLE(issue) && (node = owned_libs[owner])) {
    if (node[LL_DATA] == issue)
      owned_libs[owner] = node[LL_NEXT];
    else {
      while ((next = node[LL_NEXT]) && next[LL_DATA] != issue)
	node = next;
      if (next)
	node[LL_NEXT] = next[LL_NEXT];
    }
  }

  /* remove from inheritor lists */
  list = IS_INHERITABLE(issue) ? IL_L_INHERITORS : IL_O_INHERITORS;
  for (i = sizeof(issue[I_INHERITS_FROM]); i--; )
    if (node = issue[I_INHERITS_FROM][i][list]) {
      if (node[LL_DATA] == issue)
	issue[I_INHERITS_FROM][i][list] = node[LL_NEXT];
      else {
	while ((next = node[LL_NEXT]) && next[LL_DATA] != issue)
	  node = next;
	if (next)
	  node[LL_NEXT] = next[LL_NEXT];
      }
    }
}

/*
 * NAME:        get_if_exists - get issue data if it exists
 * DESCRIPTION: Returns the issue data of issue <id>, or nil if it does not
 *              exist.  This is used in place of the ISSUE() macro at times
 *              when it is not known whether an issue exists.
 */
private mixed *get_if_exists(int id)
{
  int id1, id2;

  id1 = id >> 8;
  id2 = id % 0xff;

  if (id1 < sizeof(issues) && id2 < sizeof(issues[id1]))
    return issues[id1][id2];
  return nil;
}

/*
 * NAME:        dested_version - find and removed from dested list
 * DESCRIPTION: Finds the latest issue of object <objname> in the dested list
 *              and removes it from the list before returning the issue data.
 *              Returns nil if the object is not in the list.
 */
private mixed *dested_version(string objname)
{
  mixed *node, next;

  if (node = dested) {
    if (node[LL_DATA][I_NAME] == objname) {
      dested = node[LL_NEXT];
      return node[LL_DATA];
    }
    while ((next = node[LL_NEXT]) && next[LL_DATA][I_NAME] != objname)
      node = next;
    if (next) {
      node[LL_NEXT] = next[LL_NEXT];
      return next[LL_DATA];
    }
  }
  return nil;
}

/*
 * NAME:        new_issue - store new issue data
 * DESCRIPTION: Stores the issue data <issue> in the database.
 */
private void new_issue(mixed *issue)
{
  int i, id1, id2;

  id1 = issue[I_ID] >> 8;
  id2 = issue[I_ID] % 0xff;

  /* resize arrays as needed */
  if ((i = id1 - sizeof(issues)) >= 0) {
    issues = issues + allocate(++i);
    i = sizeof(issues);
    while (i--)
      issues[i] = ({ });
  }
  if ((i = id2 - sizeof(issues[id1])) >= 0)
    issues[id1] += allocate(++i);

  /* register issue */
  issues[id1][id2] = issue;
}

/*
 * NAME:        new_inheritable - register a new inheritable
 * DESCRIPTION: Registers the new inheritable object <objname> owned by
 *              resource owner <owner> and inheriting <inherited>.
 */
private void new_inheritable(string owner, string objname, string *inherited)
{
  mixed **inherits_from;
  mixed *issue;
  int i, id, tmp;

  id = status(objname)[O_INDEX];
  if (issue = get_if_exists(id)) /* was a recompile, get old issue data */
    rem_issue(id, owner); /* have to remove; get rid old inheritance data */

  inherits_from = allocate(i = sizeof(inherited));
  if (issue) /* recompile: only inheritance data changes */
    issue[I_INHERITS_FROM] = inherits_from;
  else /* fresh compile: make a new issue node */
    issue = NEW_INHERITABLE(dested_version(objname), id,
			    objname, inherits_from);
  /* find issues of inherited objects */
  while (i--) {
    tmp = status(inherited[i])[O_INDEX];
    inherits_from[i] = ISSUE(tmp);
    /* register inheritance */
    inherits_from[i][IL_L_INHERITORS] =
		            NEW_NODE(inherits_from[i][IL_L_INHERITORS], issue);
  }

  /* register issue */
  new_issue(issue);

  /* register owned inheritable */
  owned_libs[owner] = NEW_NODE(owned_libs[owner], issue);
}

/*
 * NAME:        new_noninheritable - register a new non-inheritable
 * DESCRIPTION: Registers the new non-inheritable object <obj> owned by
 *              resource owner <owner> and inheriting <inherited>.
 */
private void new_noninheritable(string owner, object obj, string *inherited)
{
  mixed **inherits_from;
  string objname;
  mixed *issue;
  int i, id, tmp;

  objname = object_name(obj);
  id = status(obj)[O_INDEX];
  if (issue = get_if_exists(id)) /* was a recompile, get old issue data */
    rem_issue(id, owner); /* have to remove; get rid old inheritance data */

  inherits_from = allocate(i = sizeof(inherited));
  if (issue) /* recompile: only inheritance data changes */
    issue[I_INHERITS_FROM] = inherits_from;
  else /* fresh compile: make a new issue node */
    issue = NEW_NON_INHERITABLE(dested_version(objname), id,
				objname, inherits_from);
  /* find issues of inherited objects */
  while (i--) {
    tmp = status(inherited[i])[O_INDEX];
    inherits_from[i] = ISSUE(tmp);
    /* register inheritance */
    inherits_from[i][IL_O_INHERITORS] =
			    NEW_NODE(inherits_from[i][IL_O_INHERITORS], issue);
  }

  /* register issue */
  new_issue(issue);
}

/* ---------------------- interface for driver object ---------------------- */

/*
 * NAME:        compiling()
 * DESCRIPTION: The given object is about to be compiled.
 *              This is also called when recompiling an object.  Even when
 *              the object cannot be recompiled because it is inherited.
 *              At this point we do not know whether compilation succeeds.
 */
void compiling(string path)
{
  if (previous_object() != driver)
    return;
}

/*
 * NAME:        compile()
 * DESCRIPTION: The given non-inheritable object has just been compiled.
 *              Called just before the object is initialized with create(0).
 */
void compile(string owner, object obj, string inherited...)
{
  if (previous_object() != driver)
    return;

  new_noninheritable(owner, obj, inherited);
}

/*
 * NAME:        compile_lib()
 * DESCRIPTION: The given inheritable object has just been compiled.
 */
void compile_lib(string owner, string path, string inherited...)
{
  if (previous_object() != driver)
    return;

  new_inheritable(owner, path, inherited);
}

/*
 * NAME:        compile_failed()
 * DESCRIPTION: An attempt to compile the given object has failed.
 */
void compile_failed(string owner, string path)
{
  if (previous_object() != driver)
    return;
}

/*
 * NAME:        clone()
 * DESCRIPTION: The given object has just been cloned.  Called just
 *              before the object is initialized with create(1).
 */
void clone(string owner, object obj)
{
  if (previous_object() != driver)
    return;

  ISSUE(status(obj)[O_INDEX])[IO_CLONES]++;
}

/*
 * NAME:        destruct()
 * DESCRIPTION: The given object is about to be destructed.
 */
void destruct(string owner, object obj)
{
  mixed *issue;

  if (previous_object() != driver)
    return;

  issue = ISSUE(status(obj)[O_INDEX]);

  if (sscanf(object_name(obj), "%*s#"))
    issue[IO_CLONES]--; /* dested a clone */
  else {
# ifdef ONE_VERSION
    if (!upgrading && issue[IO_CLONES])
      error("Cannot destruct cloned object");
# endif
    dested = NEW_NODE(dested, issue);
  }
}

/*
 * NAME:        destruct_lib()
 * DESCRIPTION: The given inheritable object is about to be destructed.
 */
void destruct_lib(string owner, string path)
{
  mixed *issue;

  if (previous_object() != driver)
    return;

  issue = ISSUE(status(path)[O_INDEX]);

# ifdef ONE_VERSION
  if (!upgrading && (issue[IL_L_INHERITORS] || issue[IL_O_INHERITORS]))
    error("Cannot destruct inherited object");
# endif

  dested = NEW_NODE(dested, issue);
}

/*
 * NAME:        remove_program()
 * DESCRIPTION: The last reference to the given program has been removed.
 *              This function is not called when recompiling, the new version
 *              simply reuses the same index.
 */
void remove_program(string owner, string path, int timestamp, int index)
{
  if (previous_object() != driver)
    return;

  rem_issue(index, owner);
}

/*
 * NAME:        path_special()
 * DESCRIPTION: If the standard include file contains the line
 *              `# include "AUTO"', this function is called so a file can be
 *              included that depends on what file is currently being compiled.
 */
string path_special(string compiled)
{
  if (previous_object() != driver)
    return nil;

  return nil;
}

/*
 * NAME:        include()
 * DESCRIPTION: The file <path> (which might not exist) is about to be
 *              included by <from>.
 */
void include(string from, string path)
{
  if (previous_object() != driver)
    return;
}

/*
 * NAME:        forbid_inherit()
 * DESCRIPTION: Return a non-zero value if inheritance of <path> by <from>
 *              is not allowed.
 */
int forbid_inherit(string from, string path, int priv)
{
  return FALSE;
}

/* ---------------------- interface for system objects --------------------- */

/*
 * NAME:        query_libs_owned - inheritables belonging to resource owner
 * DESCRIPTION: Returns a list of inheritables owned by resource owner
 *              <owner>, in the form of an array with two equal sized arrays
 *              as elements.  The first element is an array of integers; the
 *              issue IDs of the objects.  The second element is an array of
 *              strings; the object names.
 */
mixed **query_libs_owned(string owner)
{
  int *issue_list;
  string *name_list;
  mixed *node;

  if (!SYSTEM())
    return nil;

  issue_list = ({ });
  name_list = ({ });

  if (node = owned_libs[owner])
    do {
      issue_list += ({ node[LL_DATA][I_ID] });
      name_list += ({ node[LL_DATA][I_NAME] });
    } while (node = node[LL_NEXT]);

  return ({ issue_list, name_list });
}

/*
 * NAME:        inheriting
 * DESCRIPTION: Help function for query_inherited.
 */
private mapping inheriting(int id, int max_depth, int max_width)
{
  mixed *issue, *node;
  mapping itree;
  int cnt;

  if (max_depth == 0 || IS_NON_INHERITABLE(issue = ISSUE(id)))
    return ([ ]);

  cnt = max_width;
  itree = ([ ]);

  if (node = issue[IL_L_INHERITORS])
    do {
      id = node[LL_DATA][I_ID];
      itree[ ({ id, node[LL_DATA][I_NAME] }) ] = inheriting(id, max_depth - 1,
							     max_width);
      cnt--;
    } while ((node = node[LL_NEXT]) && cnt);

  if (cnt && (node = issue[IL_O_INHERITORS]))
    do {
      id = node[LL_DATA][I_ID];
      itree[ ({ id, node[LL_DATA][I_NAME] }) ] = ([ ]);
      cnt--;
    } while ((node = node[LL_NEXT]) && cnt);

  return itree;
}

/*
 * NAME:        query_inheriting - inheritance tree, down
 * DESCRIPTION: Returns the structure of objects inheriting issue <id>.
 *              The indices are arrays ({ (int) ID, (string) object name }).
 *              The values are new mappings representing the next
 *              inheritance level.  An empty mapping when not inherited.
 *
 *              <max_depth> is how many levels deep the inheritance tree
 *              may go.  Unlimited if 0 or not specified.
 *
 *              <max_width> is how wide the inheritance tree may be.
 *              If 0 or not specified it is limited to the max array and
 *              mapping size as set in the driver configuration file.
 *
 *              Returns nil if there is no object with issue ID <id>.
 *
 * SEE ALSO:    objectd/query_inherited_by
 */
mapping query_inheriting(int id, varargs int max_depth, int max_width)
{
  if (!SYSTEM())
    return nil;

  if (!max_depth)
    max_depth = -1;
  if (!max_width)
    max_width = status()[ST_ARRAYSIZE];
  if (!get_if_exists(id))
    return nil;
  return inheriting(id, max_depth, max_width);
}

/*
 * NAME:        inherited_by
 * DESCRIPTION: Help function for query_inherited_by.
 */
private mapping inherited_by(int id, int max_depth, int max_width)
{
  mixed **inherited, *issue;
  mapping itree;
  int i;

  if (max_depth == 0)
    return ([ ]);

  issue = ISSUE(id);
  itree = ([ ]);

  inherited = issue[I_INHERITS_FROM];
  i = sizeof(inherited) < max_width ? sizeof(inherited) : max_width;
  while (i--) {
    id = inherited[i][I_ID];
    itree[ ({ id, inherited[i][I_NAME] }) ] = inherited_by(id, max_depth - 1,
							   max_width);
  }

  return itree;
}

/*
 * NAME:        query_inherited_by - inheritance tree, up
 * DESCRIPTION: Like query_inherits, but in the opposite direction.
 *              Returns the structure of objects inherited by <id>.
 * SEE ALSO:    objectd/query_inheriting
 */
mapping query_inherited_by(int id, varargs int max_depth, int max_width)
{
  if (!SYSTEM())
    return nil;

  if (!max_depth)
    max_depth = -1;
  if (!max_width)
    max_width = status()[ST_ARRAYSIZE];
  if (!get_if_exists(id))
    return nil;
  return inherited_by(id, max_depth, max_width);
}

/*
 * NAME:        id_to_name - issue ID to object name
 * DESCRIPTION: Returns the name of the object with issue ID <id>, or nil
 *              if there is no object with that ID.
 */
string id_to_name(int id)
{
  mixed *issue;

  if (!SYSTEM())
    return nil;

  issue = get_if_exists(id);
  if (!issue)
    return nil;
  return issue[I_NAME];
}

/*
 * NAME:        latest_issue - ID of object's latest issue
 * DESCRIPTION: Returns the ID of the latest issue of the object with
 *              name <objname>.  -1 if there is none.
 */
int latest_issue(string objname)
{
  mixed *st, *node, *issue;

  /* find newest issue */
  if (st = status(objname))
    return st[O_INDEX];
  else {
    node = dested;
    while (node && node[LL_DATA][I_NAME] != objname)
      node = node[LL_NEXT];
    if (!node)
      return -1;
    return node[LL_DATA][I_ID];
  }
}

/*
 * NAME:        no_of_clones - number of clones
 * DESCRIPTION: Returns the number of clones of the object with issue ID <id>.
 *              If there is no object issue <id> or if it is an inheritable,
 *              -1 is returned.
 */
int no_of_clones(int id)
{
  mixed *issue;

  if (!SYSTEM())
    return 0;

  issue = get_if_exists(id);
  if (!issue || IS_INHERITABLE(issue))
    return -1;
  return issue[IO_CLONES];
}

/*
 * NAME:        versions - issues of same object
 * DESCRIPTION: Returns a list of all existing issues of the object with
 *              name <objname> in the form of an array of integers, each
 *              integer being the ID of one issue.  The first element is
 *              the newest issue, the last the oldest.  Returns nil if no
 *              issue exists.
 */
int *issues(string objname)
{
  mixed *issue, *st, *node;
  int *res;

  if (!SYSTEM())
    return nil;

  /* find newest issue */
  if (st = status(objname))
    issue = ISSUE(st[O_INDEX]);
  else {
    node = dested;
    while (node && node[LL_DATA][I_NAME] != objname)
      node = node[LL_NEXT];
    if (!node)
      return nil;
    issue = node[LL_DATA];
  }

  /* traverse version list, from newest to oldest */
  res = ({ });
  do {
    res += ({ issue[I_ID] });
  } while (issue = issue[I_PREV]);
  return res;
}

/*
 * NAME:        suspend_system - suspend system activity
 * DESCRIPTION: This function suspends all system activity: user input,
 *              logins and callouts except in this object.
 */
private void suspend_system()
{
  RSRCD->suspend_callouts();
}

/*
 * NAME:        release_system - undo system suspension
 * DESCRIPTION: Reverses the effects of suspend_system().
 */
private void release_system()
{
  RSRCD->release_callouts();
}

/*
 * NAME:        upgrade_auto_part()
 * DESCRIPTION: Used by upgrade_auto to upgrade piecemeal.
 */
static void upgrade_auto_part(object user, object next, object last,
			      string *owners, varargs int suspended)
{
  string name, err;
  int i, cnt;

  rlimits (0; -1) {
# ifdef UPGRADE_CHUNK
    if (suspended && user)
      user->message(".");
# endif
    do {
      while (!next) {
	if (!sizeof(owners)) {
	  if (user)
	    user->message((suspended ? "\n" : "") + "Upgraded \"" +
			  AUTO + "\".\n");
	  if (suspended)
	    release_system();
	  upgrading = FALSE;
	  return;
	}
	next = last = first_link(owners[0]);
	owners = owners[1 .. ];
      }
      do {
	name = object_name(next);
	if (!sscanf(name, "%*s#")) {
	  if (err = catch(compile_object(name)))
	    if (user)
	      user->message("\n" + name + ": " + err + "\n");
	  cnt++;
	}
      } while ((next = next_link(next)) != last
# ifdef UPGRADE_CHUNK
	       && cnt < UPGRADE_CHUNK
# endif
	);
      if (next == last)
	next = nil;
    } while (
# ifdef UPGRADE_CHUNK
      cnt < UPGRADE_CHUNK
# else
      TRUE
# endif
      );
    if (!suspended)
      suspend_system();
    call_out("upgrade_auto_part", 0, user, next, last, owners, TRUE);
  }
}

/*
 * NAME:        upgrade_auto - upgrade auto object
 * DESCRIPTION: Upgrades the auto object, ie. all objects except
 *              the driver object.
 */
private void upgrade_auto(object user)
{
  string *lib_owners, name;
  int i;
  mixed *node;

  rlimits (0; -1) {
    if (user)
      user->message("All objects, except " +
		    "the driver object, will be upgraded.\n");

    lib_owners = map_indices(owned_libs);
    for (i = sizeof(lib_owners); i--; ) {
      if (node = owned_libs[lib_owners[i]])
	do {
	  name = node[LL_DATA][I_NAME];
	  if (name != AUTO)
	    destruct_object(name);
	} while (node = node[LL_NEXT]);
    }

    upgrade_auto_part(user, nil, nil, query_owners());
  }
}

/*
 * NAME:        upgrade_inheritors_part()
 * DESCRIPTION: Used by upgrade_inheritors to upgrade piecemeal.
 */
static void upgrade_inheritors_part(string u_name, object user,
				    mixed *objs, varargs int suspended)
{
  mixed *tmp, *st;
  string name, err;
  int id, cnt;

  rlimits (0; -1) {
# ifdef UPGRADE_CHUNK
    if (suspended && user)
      user->message(".");
# endif
    do {
      if (!objs) {
	if (user)
	  user->message((suspended ? "\n" : "") + "Upgraded \"" +
			u_name + "\".\n");
	if (suspended)
	  release_system();
	upgrading = FALSE;
	return;
      }
      name = objs[LL_DATA][I_NAME];
      id = objs[LL_DATA][I_ID];
      st = status(name);
      if (!st || id != st[O_INDEX]) {
	if (user)
	  user->message("Cannot upgrade old clones of \"" + name + "\".\n");
      } else {
	if (err = catch(compile_object(name)))
	  if (user)
	    user->message("\n" + name + ": " + err + "\n");
	cnt++;
      }
      tmp = objs[LL_NEXT];
      objs[LL_NEXT] = nil;
      objs[LL_DATA] = nil;
      objs = tmp;
    } while (
# ifdef UPGRADE_CHUNK
      cnt < UPGRADE_CHUNK
# else
      TRUE
# endif
      );
    if (!suspended)
      suspend_system();
    call_out("upgrade_inheritors_part", 0, u_name, user, objs, TRUE);
  }
}

/*
 * NAME:        upgrade_inheritors - upgrade inheritors of a lib object
 * DESCRIPTION: Upgrades all objects that inherit <issue>.
 */
private void upgrade_inheritors(mixed *issue, object user)
{
  mixed *libs, *objs, *node, *tmp, *next;
  string name;

  rlimits (0; -1) {
    libs = NEW_NODE(nil, issue);
    objs = nil;

    while (libs) {
      if (node = libs[LL_DATA][IL_L_INHERITORS])
	do {
	  tmp = libs;
	  while ((next = tmp[LL_NEXT]) && tmp[LL_DATA] != node[LL_DATA])
	    tmp = next;
	  if (!next && tmp[LL_DATA] != node[LL_DATA]) {
	    tmp[LL_NEXT] = NEW_NODE(nil, node[LL_DATA]);
	    name = node[LL_DATA][I_NAME];
	    if ((tmp = status(name)) && node[LL_DATA][I_ID] == tmp[O_INDEX])
	      destruct_object(name);
	  }
	} while (node = node[LL_NEXT]);
      if (node = libs[LL_DATA][IL_O_INHERITORS])
	do {
	  tmp = objs;
	  while (tmp && tmp[LL_DATA] != node[LL_DATA])
	    tmp = tmp[LL_NEXT];
	  if (!tmp)
	    objs = NEW_NODE(objs, node[LL_DATA]);
	} while (node = node[LL_NEXT]);
      tmp = libs[LL_NEXT];
      libs[LL_NEXT] = nil;
      libs[LL_DATA] = nil;
      libs = tmp;
    }
    upgrade_inheritors_part(issue[I_NAME], user, objs);
  }
}

/*
 * NAME:        upgrade - upgrade an existing object
 * DESCRIPTION: Upgrades the object with issue ID <id> and all objects
 *              inheriting it, if it is inheritable.
 */
atomic object upgrade(int id, varargs object user)
{
  mixed *issue, *st;
  int update_loaded;
  string name;

  if (!SYSTEM())
    return nil;

  if (!(issue = get_if_exists(id))) {
    if (user)
      user->message("No object with issue ID " + id + ".\n");
    return nil;
  }

  name = issue[I_NAME];
  update_loaded = (st = status(name)) && id == st[O_INDEX];

  if (IS_NON_INHERITABLE(issue)) {
    object o;

    if (update_loaded) {
      o = compile_object(name);
      if (user)
	user->message("Upgraded \"" + name + "\".\n");
    } else if (user)
      user->message("Cannot upgrade old issue of non-inheritable \"" +
		    name + "\".\n");
    return o;
  }

  upgrading = TRUE;

  if (update_loaded)
    destruct_object(name);
  if (!status(name))
    compile_object(name);

  if (name == AUTO)
    upgrade_auto(user);
  else
    upgrade_inheritors(issue, user);

  return nil;
}
