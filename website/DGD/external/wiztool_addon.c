/*
 * wiztool commands
 *
 *        Copyright (C) 1999, 2000 Geir Harald Hansen.
 *        Free for non-commercial use.
 *        For commercial use, contact me.
 *
 *        Send suggestions and bug reports to: geirhans@ifi.uio.no
 *
 *        These commands add a user interface to the object manager.
 *        Add them to your wiztool.
 *        Sorry no help pages yet.
 */

# define OBJECTD            "/usr/System/sys/objectd"

/*
 * NAME:        parse_obj_spec - parse object specification
 * DESCRIPTION: Parses an object specification <str> given by the user.
 *              <usage> is the error message shown if <str> does not fit
 *              the input format.  Returns ({ issue ID, object name }) or
 *              nil if the specification was incorrect, in which case
 *              an error message is displayed before returning.
 *              Also checks read access.
 */
private mixed *parse_obj_spec(string usage, string str)
{
  mixed obj;
  int i, id;

  id = i = -1;
  if (!str ||
      (sscanf(str, "$%d%s", i, str) != 0 && str != "") ||
      (sscanf(str, "i%d%s", id, str) != 0 && str != "")) {
    message(usage);
    return nil;
  }

  if (id >= 0) {
    obj = OBJECTD->id_to_name(id);
    if (!obj) {
      message("No object with issue ID i" + id + ".\n");
      return nil;
    }
  } else if (i >= 0) {
    string output;

    if (output = catch(obj = fetch(i))) {
      message(output + "\n");
      return nil;
    }
    if (typeof(obj) != T_OBJECT) {
      message("Not an object.\n");
      return nil;
    }
  } else if (sscanf(str, "$%s", str) != 0) {
    obj = ident(str);
    if (!obj) {
      message("Unknown $ident.\n");
      return nil;
    }
  } else {
    obj = DRIVER->normalize_path(str, query_directory(), query_owner());
  }
  if (typeof(obj) == T_OBJECT)
    obj = object_name(obj);
  if (!access(query_owner(), obj, READ_ACCESS)) {
    message(obj + ": Access denied.\n");
    return nil;
  }
  if (id == -1)
    id = OBJECTD->latest_issue(obj);
  if (id == -1) {
    message(obj + ": No such object loaded.\n");
    return nil;
  }
  return ({ id, obj });
}

/*
 * NAME:	cmd_upgrade()
 * DESCRIPTION:	Upgrade an object and all inheriting objects, if inheritable.
 */
static void cmd_upgrade(object user, string cmd, string str)
{
  mixed obj_spec;
  object o;

  if (!(obj_spec = parse_obj_spec("Usage: " + cmd +
				  " <obj> | $<ident> | <issue ID>\n", str)))
    return;

  if (!access(query_owner(), obj_spec[1], WRITE_ACCESS)) {
    message("Access denied.\n");
    return;
  }

  if (o = OBJECTD->upgrade(obj_spec[0], user))
    store(user, o);
}

/*
 * NAME:        inheritance_entry
 * DESCRIPTION: used by cmd_inheritance and inheritance_tree
 */
private string inheritance_entry(string objname, int id)
{
  int clones, issues;

  return "\"" + objname + "\" [i" + id + "]" +
    ((sscanf(objname, "%*s/obj/") &&
      (clones = OBJECTD->no_of_clones(id)) != -1) ?
     " * " + clones : "") +
    ((issues = sizeof(OBJECTD->issues(objname))) != 1 ?
     " ^ " + issues : "");
}

/*
 * NAME:        inheritance_tree
 * DESCRIPTION: used by cmd_inheritance
 */
private string inheritance_tree(string indent, mapping itree, int up,
				int max_depth, int max_width)
{
  mixed **indices, *t;
  string res, tmp;
  int i, j, sz;

  max_depth--;
  res = "";
  indices = map_indices(itree);
  sz = sizeof(indices);

  for (i = 0; i < sz; i++)
    for (j = 0; j < i; j++)
      if (map_sizeof(itree[indices[i]]) > map_sizeof(itree[indices[j]])) {
	t = indices[i];
	indices[i] = indices[j];
	indices[j] = t;
      }

  i = sz < max_width ? sz : max_width;
  while (i--) {
    tmp = indent + (i == 0 && sz <= max_width ? (up ? "," : "`") : "|") +
          (max_depth == 0 && map_sizeof(itree[indices[i]]) ? "+" : "-") +
	  inheritance_entry(indices[i][1], indices[i][0]) + "\n";
    if (up)
      res = tmp + res;
    else
      res += tmp;
    if (max_depth != 0) {
      tmp = inheritance_tree(indent + (i == 0 && sz <= max_width ?
				       "  " : "| "),
			     itree[indices[i]], up, max_depth, max_width);
      if (up)
	res = tmp + res;
      else
	res += tmp;
    }
  }

  if (sz > max_width) {
    if (up)
      res = indent + ",+ . . .\n" + res;
    else
      res = res + indent + "`+ . . .\n";
  }

  return res;
}

/*
 * NAME:        cmd_inheritance()
 * DESCRIPTION: show inheritance graph for object
 */
static void cmd_inheritance(object user, string cmd, string str)
{
  mapping inherits, inherited;
  string tmp, obj, usage;
  mixed obj_spec;
  int id, max_depth, max_width;

  usage = "Usage: " + cmd +
	  " (<obj> | $<ident> | <issue ID>) [depth [width]]\n";

  if (!str ||
      (sscanf(str, "%s %d %d%s", str, max_depth, max_width, tmp) > 2 &&
       tmp != "") ||
      (sscanf(str, "%s %d%s", str, max_depth, tmp) != 0 && tmp != "")) {
    message(usage);
    return;
  }

  if (max_depth < 0) {
    message("Illegal depth.\n");
    return;
  }
  if (max_width < 0) {
    message("Illegal width.\n");
    return;
  }

  if (!(obj_spec = parse_obj_spec(usage, str)))
    return;
  id = obj_spec[0];
  obj = obj_spec[1];

  sscanf(obj, "%s#", obj); /* We want the name of the master object. */

  if (!max_depth)
    max_depth = -1;
  else
    max_depth++; /* need an extra level to show "-" or "+" */
  if (!max_width)
    max_width = status()[ST_ARRAYSIZE];
  else
    max_width++; /* need an extra level to show ". . ." */

  inherits = OBJECTD->query_inherited_by(id, max_depth, max_width);
  inherited = OBJECTD->query_inheriting(id, max_depth, max_width);

  if (max_depth != -1)
    max_depth--;
  max_width--;

  user->more_string(inheritance_tree("   ", inherits, TRUE,
				     max_depth, max_width) +
		    "-> " + inheritance_entry(obj, id) + "\n" +
		    inheritance_tree("   ", inherited, FALSE,
				     max_depth, max_width));
}

/*
 * NAME:	cmd_objects()
 * DESCRIPTION:	show objects belonging to a resource owner
 */
static void cmd_objects(object user, string cmd, string str)
{
  string who, out, owner, tmp;
  mixed **libs;
  object first, o;
  int cnt, lcnt;

  owner = query_owner();
  if (!str)
    who = str = owner;
  else if (str == "Ecru")
    who = nil;
  else
    who = str;

  if (who != owner && !access(owner, "/", FULL_ACCESS)) {
    message("Permission denied.\n");
    return;
  }

  if (sizeof(query_owners() & ({ who })) == 0) {
    message("No such resource owner.\n");
    return;
  }

  libs = OBJECTD->query_libs_owned(who);
  cnt = lcnt = sizeof(libs[0]);
  out = "";
  while (cnt--) {
    tmp = "i" + libs[0][cnt];
    out += ("       " + tmp)[strlen(tmp) .. ] + "   " + libs[1][cnt] + "\n";
  }

  o = first = first_link(who);
  out += "--------+------------------------------\n";
  if (who == "System") {
    cnt = 1;
    tmp = "i" + status(DRIVER)[O_INDEX];
    out += ("       " + tmp)[strlen(tmp) .. ] + "   " + DRIVER + "\n";
  } else
    cnt = 0;
  while (o) {
    tmp = "i" + status(o)[O_INDEX];
    out += ("       " + tmp)[strlen(tmp) .. ] + "   " + object_name(o) + "\n";
    cnt++;
    o = next_link(o);
    if (o == first)
      o = nil;
  }
  out = (cnt + lcnt) + " objects (" + lcnt + " inheritable and " +
	cnt + " non-inheritable) owned by " + str + ".\n\n" +
	"issue    object name\n" +
	"--------+------------------------------\n" +
	out;

  user->more_string(out);
}
