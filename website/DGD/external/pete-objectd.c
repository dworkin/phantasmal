#include <kernel/kernel.h>
#include <kernel/objreg.h>
#include <kernel/rsrc.h>
#include <kernel/access.h>
#include <kernel/user.h>
#include <type.h>
#include <status.h>

inherit objreg API_OBJREG;
inherit rsrc API_RSRC;
inherit access API_ACCESS;

/* Mapping of obj_name : ({ array of descendants, source string, array of inherites, destructed_flag }) */
private mapping libs;

/* Mapping of obj_name : ({ array of clone numbers, source string, array of inherites }) */
private mapping objects;

/* Array of indices (O_INDEX) of destructed but not yet removed programs */
private int *destructed;

nomask static void create(varargs int clone)
{
    string *owners, obj_name, base_obj_name;
    object obj, first_obj;
    int i, clone_number;
    
    libs = ([ ]);
    objects = ([ ]);
    destructed = ({ });
    
    objreg::create();
    rsrc::create();
    access::create();
    
    find_object(DRIVER)->set_object_manager(this_object());
    
    libs[AUTO] = ({ ({ }), nil, ({ }), 0 });
    objects[DRIVER] = ({ ({ }), nil, ({ }) });
    
    /* This is needed, otherwise driver->recompile is called with nil object (bug?) */
    destruct_object(LIB_WIZTOOL);
    
    owners = rsrc::query_owners();
    for(i = sizeof(owners); i--; )
    {
	for(first_obj = obj = objreg::first_link(owners[i]); obj; )
	{
	    obj_name = object_name(obj);
	    if(sscanf(obj_name, "%s#%d", base_obj_name, clone_number) == 2)
	    {
		if(!objects[base_obj_name])
		    compile_object(base_obj_name);
		if(indexof(clone_number, objects[base_obj_name][0]) >= 0)
		    error("Duplicate cloned object");
		else
		    objects[base_obj_name][0] += ({ clone_number });
	    }
	    else
		if(!objects[obj_name])
		    compile_object(obj_name);
	    
	    obj = objreg::next_link(obj);
	    if(obj == first_obj)
		break;
	}
    }
}

nomask void compile(string owner, object obj, string source, string inherited...)
{
    string obj_name, *inh_diff;
    int i, ci;
    
    if(previous_program() != DRIVER)
	return;
    
    obj_name = object_name(obj);
    
    if(obj_name != DRIVER && sizeof(inherited) <= 0)
        inherited += ({ AUTO });
    
    if(!objects[obj_name])
	objects[obj_name] = ({ ({ }), source, inherited });
    else
    {
	inh_diff = objects[obj_name][2] - inherited;
	
	objects[obj_name][1] = source;
	objects[obj_name][2] = inherited;
	
	for(i = sizeof(inh_diff); i--; )
	    if(libs[inh_diff[i]])
		libs[inh_diff[i]][0] -= ({ obj_name });
    }
    
    for(i = sizeof(inherited); i--; )
	if(libs[inherited[i]])
	    libs[inherited[i]][0] |= ({ obj_name });
	else
	{
	    destruct_object(inherited[i]);
	    libs[inherited[i]] = ({ ({ obj_name }), nil, ({ }), 1 });
	    catch {
		compile_object(inherited[i]);
	    } :
		error("Cannot recompile inherited object");
	}
}

nomask void compile_lib(string owner, string obj_name, string source, string inherited...)
{
    string *inh_diff;
    int i;
    
    if(previous_program() != DRIVER)
	return;
    
    if(obj_name != AUTO && sizeof(inherited) <= 0)
	inherited += ({ AUTO });
    
    if(!libs[obj_name])
	libs[obj_name] = ({ ({ }), source, inherited, 0 });
    else 
    {
	inh_diff = (!libs[obj_name][2] ? ({ }) : libs[obj_name][2] - inherited);
	
	libs[obj_name][1] = source;
	libs[obj_name][2] = inherited;
	libs[obj_name][3] = 0;
	
	for(i = sizeof(inh_diff); i--; )
	    if(libs[inh_diff[i]])
		libs[inh_diff[i]][0] -= ({ obj_name });
    }
    
    for(i = sizeof(inherited); i--; )
	if(libs[inherited[i]])
	    libs[inherited[i]][0] |= ({ obj_name });
	else
	{
	    destruct_object(inherited[i]);
	    libs[inherited[i]] = ({ ({ obj_name }), nil, ({ }), 1 });
	    catch {
		compile_object(inherited[i]);
	    } :
		error("Cannot recompile inherited object");
	}
}

nomask void clone(string owner, object obj)
{
    string obj_name, base_obj_name;
    int clone_number;
    
    if(previous_program() != DRIVER)
	return;
    
    obj_name = object_name(obj);
    sscanf(obj_name, "%s#%d", base_obj_name, clone_number);
    
    if(!objects[base_obj_name])
    {
	if(!find_object(base_obj_name) || !compile_object(base_obj_name))
	    error("Cloned object without master object");
    }
    
    if(indexof(clone_number, objects[base_obj_name][0]) >= 0)
	error("Duplicate cloned object");
    else
	objects[base_obj_name][0] += ({ clone_number });
}

private void clear_libs()
{
    int i, j;
    string *ind, lib_name, *libs_to_clear;
    mixed *val;
    
    ind = map_indices(libs);
    val = map_values(libs);
    libs_to_clear = ({ });
    for(i = map_sizeof(libs); i--; )
    {
	if(sizeof(val[i][0]) <= 0 && val[i][3])
	{
	    for(j = sizeof(val[i][2]); j--; )
	    {
		lib_name = val[i][2][j];
		if(libs[lib_name])
		{
		    libs[lib_name][0] -= ({ ind[i] });
		    if(sizeof(libs[lib_name][0]) <= 0 && libs[lib_name][3])
			libs_to_clear += ({ lib_name });
		}
	    }
	    libs[ind[i]] = nil;
	    libs_to_clear -= ({ ind[i] });
	}
    }
    if(sizeof(libs_to_clear) > 0)
	clear_libs();
}

nomask void destruct(string owner, object obj)
{
    string obj_name, base_obj_name, lib_name;
    int clone, clone_number, i, do_clear_libs;
    
    if(previous_program() != DRIVER)
	return;
    
    base_obj_name = obj_name = object_name(obj);
    clone = (sscanf(obj_name, "%s#%d", base_obj_name, clone_number) == 2);
    
    if(!objects[base_obj_name])
    {
	catch(error("Non-registered object"));
	return;
    }
    
    if(clone)
    {
	if(indexof(clone_number, objects[base_obj_name][0]) < 0)
	    catch(error("Non-registered cloned object"));
	else
	    objects[base_obj_name][0] -= ({ clone_number });
    }
    else if(sizeof(objects[base_obj_name][0]) > 0)
	error("Destructing master object is disallowed while clones exist");
    else
    {
	for(i = sizeof(objects[base_obj_name][2]); i--; )
	{
	    lib_name = objects[base_obj_name][2][i];
	    libs[lib_name][0] -= ({ base_obj_name });
	    if(sizeof(libs[lib_name][0]) <= 0 && libs[lib_name][3])
		do_clear_libs = 1;
	}
	objects[base_obj_name] = nil;
	if(do_clear_libs)
	    clear_libs();
	
	destructed += ({ status(base_obj_name)[O_INDEX] });
    }
}

nomask void destruct_lib(string owner, string obj_name)
{
    int i, do_clear_libs;
    string lib_name;
    
    if(previous_program() != DRIVER)
	return;
    
    destructed += ({ status(obj_name)[O_INDEX] });
    
    if(!libs[obj_name])
	return;
    
    if(libs[obj_name][3])
    {
	catch(error("Destructing already destructed inheritable"));
	return;
    }
    
    if(sizeof(libs[obj_name][0]) <= 0)
    {
	for(i = sizeof(libs[obj_name][2]); i--; )
	{
	    lib_name = libs[obj_name][2][i];
	    if(libs[lib_name])
	    {
		libs[lib_name][0] -= ({ obj_name });
		if(sizeof(libs[lib_name][0]) <= 0 && libs[lib_name][3])
		    do_clear_libs = 1;
	    }
	}
	libs[obj_name] = nil;
	if(do_clear_libs)
	    clear_libs();
    }
    else
    {
	libs[obj_name][1] = nil;
	libs[obj_name][3] = 1;
    }
}

nomask void remove_program(string owner, string obj_name, int timestamp, int index)
{
    if(previous_program() != DRIVER)
	return;
    
    if(indexof(index, destructed) < 0)
	catch(error("Removing undestructed object " + obj_name));
    else
	destructed -= ({ index });
}

nomask void compiling(string obj_name)
{
}

nomask void include(string from, string path)
{
}

nomask void compile_failed(string owner, string path)
{
}

nomask string path_special(string compiled)
{
    return "";
}

nomask int forbid_call(string path)
{
    return 0;
}

nomask int forbid_inherit(string from, string path, int priv)
{
    return 0;
}

nomask mapping query_libs()
{
    mapping rslt;
    string *ind;
    int i;
    
    rslt = ([ ]);
    ind = map_indices(libs);
    for(i = sizeof(ind); i--; )
	rslt[ind[i]] = ({ libs[ind[i]][0][..], libs[ind[i]][2][..], libs[ind[i]][3] });
    
    return rslt;
}

nomask mapping query_objects()
{
    mapping rslt;
    string *ind;
    int i;
    
    rslt = ([ ]);
    ind = map_indices(objects);
    for(i = sizeof(ind); i--; )
	if(typeof(objects[ind[i]]) == T_ARRAY)
	    rslt[ind[i]] = ({ objects[ind[i]][0][..], objects[ind[i]][2][..] });
    return rslt;
}

private void get_upgrade_dependecies(string obj_name, mapping deps, int depth)
{
    string dep_obj_name;
    int i;
    
    for(i = sizeof(libs[obj_name][0]); i--; )
    {
	dep_obj_name = libs[obj_name][0][i];
	if(!deps[dep_obj_name] || deps[dep_obj_name] < depth)
	    deps[dep_obj_name] = depth;
	if(libs[dep_obj_name])
	    get_upgrade_dependecies(dep_obj_name, deps, depth + 1);
    }
}

private atomic void __do_upgrade(string **obj_names)
{
    int i, j;
    
    for(i = 0; i < sizeof(obj_names); i++)
	for(j = sizeof(obj_names[i]); j--; )
	    if(libs[obj_names[i][j]])
		destruct_object(obj_names[i][j]);
	    else if(!objects[obj_names[i][j]])
		error("Invalid try to upgrade non-registered object");
    for(i = 0; i < sizeof(obj_names); i++)
	for(j = sizeof(obj_names[i]); j--; )
	    if(libs[obj_names[i][j]])
		compile_object(obj_names[i][j], libs[obj_names[i][j]][1]);
	    else if(objects[obj_names[i][j]])
		compile_object(obj_names[i][j], objects[obj_names[i][j]][1]);
}

nomask int upgrade_object(mixed obj_name)
{
    mapping deps, rdeps;
    string *ind;
    int *val, i;
    
    if(!previous_object())
	return FALSE;
    
    if(typeof(obj_name) == T_OBJECT)
	obj_name = object_name(obj_name);
    else if(typeof(obj_name) == T_STRING)
	obj_name = DRIVER->normalize_path(obj_name,
	    object_name(previous_object()) + "/..",
	    DRIVER->creator(object_name(previous_object())));
    
    if(typeof(obj_name) != T_STRING)
	error("Bad argument 1 for function upgrade_object");
    sscanf(obj_name, "%s#%*d", obj_name);
    
    if(!libs[obj_name] && !objects[obj_name])
	return FALSE;
    
    /* Permissions: taken from auto's compile_object.
       If you can compile object you can upgrade it too. */
    if(DRIVER->creator(obj_name) &&
	DRIVER->creator(object_name(previous_object())) != "System" &&
	!access::access(object_name(previous_object()), obj_name,
	    sscanf(obj_name, "/kernel/%*s") == 0 && libs[obj_name] &&
	    !libs[obj_name][1] ? READ_ACCESS : WRITE_ACCESS))
	error("Access denied");
    
    if(libs[obj_name])
    {
	deps = ([ obj_name : 0 ]);
	get_upgrade_dependecies(obj_name, deps, 1);
	
	rdeps = ([ ]);
	ind = map_indices(deps);
	val = map_values(deps);
	for(i = map_sizeof(deps); i--; )
	{
	    #ifdef O_UPGRADING
		if(status(ind[i])[O_UPGRADING])
		    error("Cannot perform upgrade, some objects are already being upgraded");
	    #endif
	    if(!rdeps[val[i]])
		rdeps[val[i]] = ({ ind[i] });
	    else
		rdeps[val[i]] += ({ ind[i] });
	}
	
	rlimits(0; -1)
	{
	    __do_upgrade(map_values(rdeps));
	}
    }
    else if(objects[obj_name])
	compile_object(obj_name, objects[obj_name][1]);
    else
	error("Invalid try to upgrade non-registered object");
    
    return TRUE;
}
