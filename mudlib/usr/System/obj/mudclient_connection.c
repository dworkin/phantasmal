# include "phantasmal/telnet.h"

/* # define DEBUG SSH_DEBUG */

inherit conn LIB_CONN;
inherit user LIB_USER;

/*
  This object is both a user object and a connection object.  When the
  MudClientD returns it as a user, a fairly complicated structure
  springs up for handling this connection.  Remember that the Kernel
  Library separates the connection and user object from each other
  anyway.

  [binary conn] <-> [Mudclient_conn]
                     [telnet conn]    <-> [normal user object]

  The Mudclient Connection object gets its input from a standard
  Kernel binary connection in raw mode.  It processes that input and
  returns appropriate lines to its underlying (inherited) connection,
  which believes itself to be a Kernel telnet connection.  The first
  line of input on the connection causes it to query UserD (and thus,
  indirectly, the telnet handler) with a username to get a new user
  object.

  The Mudclient connection thus acts as a filter, and its inherited
  LIB_CONN structure gets only the filtered input.  Because this
  hat-trick isn't perfect, it'll always return a user object as though
  the Mudclient connection were on port offset 0, the first telnet
  port.

  Thanks to Felix's SSH code for showing me how this is done, and for
  a lot of the code in this file.  Have I mentioned that his warped
  brilliance continues to intimidate me?
*/

private string buffer;
private mixed* input_lines;

static void create(int clone)
{
    if (clone) {
      conn::create("telnet");	/* Treat it like a telnet object */
      buffer = "";
      input_lines = ({ });
    }
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
 * NAME:	login()
 * DESCRIPTION:	accept a SSH connection
 */
int login(string str)
{
    if (previous_program() == LIB_CONN) {
	user::connection(previous_object());
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
 * DESCRIPTION:	pass on mode changes to the binary connection object
 */
void set_mode(int mode)
{
    if (SYSTEM() && mode >= MODE_UNBLOCK) {
	query_conn()->set_mode(mode);

	/* TODO: If we're unblocking input, should we dispatch any
	   remaining lines in the array of waiting lines? */
    }
}

/*
 * NAME:	user_input()
 * DESCRIPTION:	send filtered input to inherited telnet connection
 */
static int user_input(string str)
{
    return conn::receive_message(nil, str);
}

/*
 * NAME:	disconnect()
 * DESCRIPTION:	forward a disconnect to the binary connection
 */
void disconnect()
{
    if (previous_program() == LIB_USER) {
	user::disconnect();
    }
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
 * NAME:	message()
 * DESCRIPTION:	send a message to the other side
 */
static int message(string str)
{
  /* Do appropriate send-filtering first */
  return user::message(str);
}

/*
 * Add new characters to buffer.  Filter newlines, backspaces and
 * telnet IAC codes appropriately.  If a full line of input has been
 * read, set input_line appropriately.
 */
private int new_telnet_input(string str) {
  buffer += str;
}

private string get_input_line(void) {
  string tmp;

  if(sizeof(input_lines)) {
    tmp = input_lines[0];
    input_lines = input_lines[1..];
    return tmp;
  }
  return nil;
}

/******************************************************************/
/******************************************************************/
/******************************************************************/
/******************************************************************/

/*
 * NAME:	receive_message()
 * DESCRIPTION:	receive a message
 */
int receive_message(string str)
{
  string line;
  int    mode;

  if (previous_program() == LIB_CONN) {
    new_telnet_input(str);

    line = get_input_line();
    while(line) {
      mode = user_input(input_line);
      if(mode == MODE_DISCONNECT || mode >= MODE_UNBLOCK)
	return mode;

      line = get_input_line();
    }
  }

  return MODE_NOCHANGE;
}
