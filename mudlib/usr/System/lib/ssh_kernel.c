# include "phantasmal/ssh.h"

# define DEBUG SSH_DEBUG

inherit conn LIB_CONN;
inherit user LIB_USER;


/* ========================================================================= *
 *			SSH glue for the kernel library                      *
 * ========================================================================= */

static void start_transport(string str);      /* supplied by transport layer */
static void create_ssh();

private string name;		/* name of this user */
private int tried_password;	/* password flag; by default only one attempt */


/*
 * NAME:	message()
 * DESCRIPTION:	send a message to the other side
 */
static int message(string str)
{
    return user::message(str);
}

/*
 * NAME:	message_done()
 * DESCRIPTION:	forward message_done to user
 */
static int message_done()
{
    object user;
    int mode;

    user = query_user();
    if (user) {
	mode = user->message_done();
	if (mode == MODE_DISCONNECT || mode >= MODE_UNBLOCK) {
	    return mode;
	}
    }
    return MODE_NOCHANGE;
}

/*
 * NAME:	login()
 * DESCRIPTION:	accept a SSH connection
 */
int login(string str)
{
    if (previous_program() == LIB_CONN) {
	user::connection(previous_object());
	previous_object()->set_mode(MODE_RAW);
	start_transport(str);
    }
    return MODE_RAW;
}

/*
 * NAME:	logout()
 * DESCRIPTION:	disconnect
 */
void logout(int quit)
{
    if (previous_program() == LIB_CONN) {
	conn::close(nil, quit);
	if (quit) {
	    destruct_object(this_object());
	}
    }
}

/*
 * NAME:	set_mode()
 * DESCRIPTION:	pass on mode changes to the real connection object
 */
void set_mode(int mode)
{
    if (SYSTEM() && mode >= MODE_UNBLOCK) {
	query_conn()->set_mode(mode);
    }
}

/*
 * NAME:	user_input()
 * DESCRIPTION:	send input to user object
 */
static int user_input(string str)
{
    return conn::receive_message(nil, str);
}

/*
 * NAME:	datagram_challenge()
 * DESCRIPTION:	there is no datagram channel to be opened
 */
void datagram_challenge(string str)
{
}

/*
 * NAME:	datagram()
 * DESCRIPTION:	don't send a datagram to the client
 */
int datagram(string str)
{
    return 0;
}

/*
 * NAME:	disconnect()
 * DESCRIPTION:	forward a disconnect to the connection
 */
void disconnect()
{
    if (previous_program() == LIB_USER) {
	user::disconnect();
    }
}

/*
 * NAME:	ssh_get_user()
 * DESCRIPTION:	check if user exists and can login
 */
static int ssh_get_user(string str)
{
    if (name) {
	return (str == name && !tried_password && query_user());
    } else {
	name = str;
	return (user_input(str) != MODE_DISCONNECT && query_user());
    }
}

/*
 * NAME:	check_password()
 * DESCRIPTION:	check whether a supplied password is correct
 */
static int ssh_check_password(string str)
{
    if (tried_password) {
	return FALSE;
    }
    tried_password = TRUE;
    return (user_input(str) != MODE_DISCONNECT);
}

/*
 * NAME:	ssh_do_login()
 * DESCRIPTION:	actually login the user
 */
static void ssh_do_login()
{
    query_user()->do_login();
}

/*
 * NAME:	create_glue()
 * DESCRIPTION:	initialize ssh kernel glue
 */
static void create_glue()
{
    conn::create("telnet");	/* pretend */
}

/*
 * NAME:	create()
 * DESCRIPTION:	initialize ssh object
 */
static void create(int clone)
{
    if (clone) {
	create_ssh();
    }
}
