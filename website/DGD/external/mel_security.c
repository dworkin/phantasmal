/*
   security.c
   contains various functions related to mud security.
*/

/* config.h and options.h are included in overrides.c */

#include <admin.h>
#include <type.h>

/* creator is a string that is the name of the player or wizard who
   is responsible for this object's existence. privileges is a string
   that indicates this object's level of permissions. creator will
   always be the name of the player. privileges can be an arbitrary
   string, which the mudlib can deal with as it pleases.
*/

private int noread, nowrite ;

static nomask string base_name(object ob) ;
static nomask string resolve_path (string str) ;
static nomask int member_array (mixed elt, mixed *arr) ;
static nomask string previous_function() ;

nomask void set_creator() {

    string str, obj_file ;

 /* Once set, you can't reset the creator, unless the creator
       is the temporary login. */

    if (creator && creator!="login") return ;

    obj_file = base_name(this_object()) ;

    /* User and player objects get the name of the user as creator. If
       this_user() isn't defined, we're in login and we set the creator
       appropriately. It will be reset when the player enters his name. */
    if (obj_file==USER || obj_file==PLAYER) {
        if (this_user()) {
            str=this_user()->query_name() ;
            creator=str ;
        } else {
	    creator="login" ;
	}

    /* All other objects get the creator of this_user() as their own. This
       may not be the Right Thing but it serves for now. If there is no
       user object defined, then this object was loaded by the driver. */
    } else {
        if (this_user()) {
	    creator=this_user()->query_creator() ;
	} else {
	    creator="Driver" ;
	}
    }
}

nomask string query_creator() {
    return creator ;
}

nomask void set_privileges() {
    
    string str, obj_file, name ;
    string *dirs ;

 /* Once set, you can't reset the privileges, unless the privileges
       is the temporary login. */

    if (privileges && privileges!="login") return ;

/* Right now we only base privileges on the base name of the object. If
   you wanted to use the clone number, you could do that also, but we
   don't do that right now. */
    obj_file = base_name(this_object()) ;

/* If you're a user or player, you get "login" if you're still logging in,
   and otherwise you get "admin", "wizard", or "player", as you merit. */
    if (obj_file==USER || obj_file==PLAYER) {
        if (!this_user()) {
	    privileges="login" ;
	} else {
	    if (this_object()->query_wizard()) {
    	        name = this_object()->query_name() ;
	        if (member_array(name,ADMIN)==-1) {
	            privileges="wizard" ;
	        } else {
		    privileges="admin" ;
		}
            } else {
    	        privileges="player" ;
	    }
	}
    } else {
/* Commands get "command" for a privileges string. */
        dirs = explode(obj_file,"/") ;
	if (dirs[0]=="cmds") {
	    privileges = "command" ;
	    return ;
	}
/* System objects get "system". */
	if (dirs[0]=="system") {
	    privileges = "system" ;
	    return ;
	}
/* Objects loaded from a wizard's directory get the wizard's name. If
   an object "/users/foo.c" is loaded, it falls through to the default
   privileges below. */
        if (dirs[0]=="users") {
	    if (dirs[1]) {
	        privileges = dirs[1] ;
		return ;
            }
        }
/* Anything not caught above is just a plain old object. */
        privileges = "object" ;
    }
}

nomask string query_privileges() {
    return privileges ;
}

/* An object may defeat its own ability to read and write files if
   it wants to for security reasons. These two functions permit that.
   Note that if you turn off an ability, there is no way to turn it back
   on later.
*/

static nomask void disable_read() {
    noread = 1 ;
}

static nomask void disable_write() {
    nowrite = 1 ;
}

/* The all-important valid_read. The basic rule at this time is:
    1. Player, wizard and admin objects may read anything.
    2. Commands can read anything.
    3. Files in /data can be read only by system and command objects,
       including a user in login.
    4. Other than 3), any object can read anything.
*/

nomask int valid_read (string filename) {

    int flag ;

    if (noread) return 0 ;

/* Resolve the path. */
    filename = resolve_path(filename) ;

    if (strlen(filename)>4 && filename[0..4]=="data/") {
	if (privileges!="command" && privileges!="system" &&
	   privileges!="admin" && privileges!="login") return 0 ;
    }
    return 1 ;
}

/* And valid_write. The basic rule here is:
    1. Admin objects can write anything.
    2. Commands can write anything.
    3. Wizard objects can write to their own directories.
    4. Objects loaded from a wizard's directory can write to that
       directory.
    5. Other objects can't write at all.
   The exception is that if the previous function is log_file(), then
   if this object is a user object, the write succeeds. This allows
   the keeping of the USAGE log with calls that come from an object
   which ordinarily cannot write to /log. Writes to places other than
   /log succeed if they have normal permission, else fail.
*/

nomask int valid_write (string filename) {

    string name,remainder ;

/* If this object has turned off its write access, then forget it. */
    if (nowrite) return 0 ;

/* If this object is the body of an admin or a command, then approve. */
    if (privileges=="admin") return 1 ;
    if (privileges=="command") return 1 ;

/* Resolve the path. */
    filename = resolve_path(filename) ;

/* Strip off a leading "/" if there is one.  (I think resolve_path
   will take care of this, but I'm not sure.) */
    if (filename[0]=='/') filename = filename[1..strlen(filename)-1] ;

/* Wizard bodies can write to their own directories. */
    if (privileges=="wizard") {
        if (sscanf(filename,"users/%s/%s",name,remainder)==2) {
	    if (name==creator) return 1 ;
        }
    }
/* Objects from a wizard's directory can also. */
    if (sscanf(filename,"users/%s/%s",name,remainder)==2) {
	if (name==privileges) return 1 ;
    }
/* log_file calls from user.c succeed if the write is in /log. */
    if (previous_function()=="log_file") {
	if (filename[0..3]=="log/") {
	  if (base_name(this_object())==USER) {
	    return 1 ;
	  }
	}
    }
/* All other attempts are denied. */
    return 0 ;
}

/* A non-recursive copy function for arrays, mappings, and other
   variables represented by pointers. If the variable being copied
   itself contains pointers, those pointers are _not_ copied so the
   data they point to is not secured. (So far I have no need of such
   a structure so this is adequate. And much faster.)
*/

nomask mixed copy (mixed a) {

    mixed b ;

    if (typeof(a)==T_ARRAY || typeof(a)==T_MAPPING) {
        b = a[..] ;
    } else {
        b = a ;
    }
    return b ;
}
