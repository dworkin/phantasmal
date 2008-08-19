# include <kernel/kernel.h>
# include <kernel/user.h>
# include <kernel/rsrc.h>

# include "phantasmal/lpc_names.h"
# include "phantasmal/log.h"
# include "phantasmal/ssh.h"

# include "gameconfig.h"

inherit LIB_CONN;
inherit rsrc API_RSRC;
private inherit SSH_UTILS;

object userd;		/* user daemon */
string version;		/* version string */
string host_key;	/* private host key */
string pub_host_key;	/* public host key */
static int suspended, shutdown;

/*
 * NAME:	create()
 * DESCRIPTION:	initialize ssh connection daemon
 */
static create()
{
    string str;

    /*
     * read private host key
     */
    str = read_file("~/keys/id_dsa");
    if (!str) {
	error("No host key");
    }
    host_key = parse_private_key(str);
    if (!host_key) {
	error("Bad host key");
    }

    /*
     * read public host key
     */
    str = read_file("~/keys/id_dsa.pub");
    if (!str) {
	error("No public host key");
    }
    pub_host_key = parse_public_key(str);
    if (!pub_host_key) {
	error("Bad public host key");
    }

    if (hash_crc32(host_key, pub_host_key) == 6922236) {
	DRIVER->message("*WARNING*\n\nYou are using pre-configured host keys.  To install your own host keys, run the\ncommand 'ssh-keygen -t dsa' and save the files in mudlib directory\n/usr/System/keys.\n\n");
    }

    /*
     * initialize
     */
    rsrc::create();
    /*rsrc::rsrc_set_limit("System", "ticks", 3000000);*/
    compile_object(SSH_TRANSPORT);
    compile_object(SSH_CONNECTION);
    userd = find_object(USERD);
    userd->set_binary_manager(0, this_object());
    version = "SSH-2.0-LPCssh_1.0";
}


void suspend_input(int shutdownp) {
  if(!SYSTEM() && !KERNEL())
    error("Invalid call to suspend_input!");

  if(suspended)
    LOGD->write_syslog("Suspended again without release!", LOG_ERR);

  suspended = 1;
  if(shutdownp)
    shutdown = 1;
}

void release_input(void) {
  if(!SYSTEM() && !KERNEL())
    error("Invalid call to suspend_input!");

  if(!suspended)
    LOGD->write_syslog("Released without suspend!", LOG_ERR);

  suspended = 0;
}

/*
 * NAME:	select()
 * DESCRIPTION:	select protocol
 */
object select(string protocol)
{
    if (previous_object() == userd && sscanf(protocol, "SSH-2.0-%*s") != 0) {
	return clone_object(SSH_CONNECTION);
    }
    return this_object();
}

/*
 * NAME:	login()
 * DESCRIPTION:	display an errormessage and disconnect
 */
int login(string str)
{
    previous_object()->message("Protocol mismatch.\r\n");
    return MODE_DISCONNECT;
}


/*
 * NAME:	query_timeout()
 * DESCRIPTION:	return login timeout
 */
int query_timeout(object connection)
{
  object game_driver;

  if(suspended || shutdown)
    return -1;

  game_driver = find_object(GAME_DRIVER);

  if(game_driver && query_ip_number(connection)
     && game_driver->site_is_banned(query_ip_number(connection))) {
    return -1;
  }

  return DEFAULT_TIMEOUT;
}

/*
 * NAME:	query_banner()
 * DESCRIPTION:	return login banner
 */
string query_banner(object obj)
{
    return version + "\r\n";
}

/*
 * NAME:	query_version()
 * DESCRIPTION:	return the version string
 */
string query_version()
{
    return version;
}

/*
 * NAME:	query_host_key()
 * DESCRIPTION:	return the (private) host key
 */
string query_host_key()
{
    if (previous_program() == SSH_TRANSPORT) {
	return host_key;
    }
}

/*
 * NAME:	query_pub_host_key()
 * DESCRIPTION:	return the public host key
 */
string query_pub_host_key()
{
    return pub_host_key;
}

/*
 * NAME:        valid_public_key()
 * DESCRIPTION: Check the ~/.ssh/ directory to see if this is an acceptable
 *              public key.
 */
int valid_public_key(string name, string publickey)
{
    if (previous_program() == SSH_CONNECTION) {
	string str;

	str = read_file("~" + name + "/.ssh/id_dsa.pub");
	if (str) {
	    string pkey;

	    sscanf(str, "%s\n", str);
	    pkey = parse_public_key(str);
	    if (pkey && pkey == publickey) {
		return 1;
	    }
	}
	str = read_file("~" + name + "/.ssh/id_rsa.pub");
	if (str) {
	    string pkey;

	    sscanf(str, "%s\n", str);
	    pkey = parse_public_key(str);
	    if (pkey && pkey == publickey) {
		return 1;
	    }
	}
	str = read_file("~" + name + "/.ssh/authorized_keys");
	if (str) {
	    int    i, sz;
	    string *lines;

	    lines = explode(implode(explode(str, "\r"), ""), "\n");
	    sz = sizeof(lines);
	    for (i = 0; i < sz; i++) {
		if (lines[i] && strlen(lines[i])) {
		    string pkey;

		    pkey = parse_public_key(lines[i]);
		    if (pkey && pkey == publickey) {
			return 1;
		    }
		}
	    }
	}
    }
    return 0;
}
