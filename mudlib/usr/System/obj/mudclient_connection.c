#include <kernel/kernel.h>
#include <kernel/user.h>
#include <phantasmal/lpc_names.h>
#include <phantasmal/telnet.h>
#include <phantasmal/mudclient.h>
#include <phantasmal/log.h>

inherit COMMON_AUTO;
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

private string buffer, outbuf;
private mixed* input_lines;
private int    active_protocols;

/* telnet options */
private int    suppress_ga;
private int    suppress_echo;
private string tel_goahead;

private int new_telnet_input(string str);
private string debug_escape_str(string line);

static void create(int clone)
{
    if (clone) {
      conn::create("telnet");	/* Treat it like a telnet object */
      buffer = "";
      outbuf = nil;
      input_lines = ({ });
      active_protocols = 0;

      tel_goahead = " "; tel_goahead[0] = TP_GA;
      suppress_ga = suppress_echo = 0;
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
 * DESCRIPTION:	accept connection
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
 * DESCRIPTION:	pass mode changes to the binary connection object
 */
void set_mode(int mode)
{
    if (SYSTEM() && mode >= MODE_UNBLOCK) {
	query_conn()->set_mode(mode);

	if(suppress_echo && mode == MODE_ECHO) {
	  suppress_echo = 0;
	  this_object()->send_telnet_option(TP_DO, TELOPT_ECHO);
	} else if(!suppress_echo && mode == MODE_NOECHO) {
	  suppress_echo = 1;
	  this_object()->send_telnet_option(TP_DONT, TELOPT_ECHO);
	}
    }
}

/*
 * NAME:	user_input()
 * DESCRIPTION:	send filtered input to inherited telnet connection
 */
static int user_input(string str)
{
  LOGD->write_syslog("MCC user_input: " + str, LOG_WARN);
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
int message_done()
{
    object user;
    int mode;

    if(previous_program() == LIB_CONN) {
      user = query_user();
      if (user) {
	mode = user->message_done();
	if (mode == MODE_DISCONNECT || mode >= MODE_UNBLOCK) {
	  return mode;
	}
      }
      return MODE_NOCHANGE;
    } else
      error("Illegal call to message_done()!");
}

/*
 * NAME:        binary_message
 * DESCRIPTION: does a buffered send on the underlying binary connection
 */
int binary_message(string str) {
  if(user::query_conn()) {
    if(outbuf) {
      user::message(outbuf);
      outbuf = nil;
    }

    return user::message(str);
  } else {
    if(!outbuf) {
      outbuf = str;
      return strlen(str);
    }
    outbuf += str;
    return strlen(str);
  }
}

/*
 * NAME:	message()
 * DESCRIPTION:	send a message to the other side
 */
int message(string str)
{
  if(previous_program() == LIB_USER || previous_program() == PHANTASMAL_USER) {
    /* Do appropriate send-filtering first */
    LOGD->write_syslog("MCC message: " + str, LOG_WARN);
    return binary_message(str);
  } else {
    error("Unprivileged code calling MCC::message()!");
  }
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

/*
 * NAME:	receive_message()
 * DESCRIPTION:	receive a message
 */
int receive_message(string str)
{
  string line;
  int    mode;

  if (previous_program() == LIB_CONN || previous_program() == MUDCLIENTD) {
    new_telnet_input(str);

    line = get_input_line();
    while(line) {
      mode = user_input(line);
      if(mode == MODE_DISCONNECT || mode >= MODE_UNBLOCK)
	return mode;

      line = get_input_line();
    }
    if(!suppress_ga && !strlen(buffer)) {
      /* Have read all pending lines, nothing uncompleted in buffer */
      return binary_message(tel_goahead);
    }

    return MODE_NOCHANGE;
  }

  error("Illegal call!");
}

nomask int send_telnet_option(int command, int option) {
  string opts;

  if(!SYSTEM() && previous_object() != MUDCLIENTD->get_telopt_handler(option))
    error("Only SYSTEM code and telopt handlers can send telnet options!");

  if(command != TP_DO && command != TP_DONT && command != TP_WILL
     && command != TP_WONT)
    error("Invalid command in send_telnet_option!");

  LOGD->write_syslog("Sending telnet option IAC " + command + " "
		     + option, LOG_WARN);

  opts = "   ";
  opts[0] = TP_IAC;
  opts[1] = command;
  opts[2] = option;

  return binary_message(opts);
}

nomask int send_telnet_subnegotiation(int option, string arguments) {
  string tmp, options;

  if(!SYSTEM() && previous_object() != MUDCLIENTD->get_telopt_handler(option))
    error("Only SYSTEM code and telopt handlers can send telnet options!");

  tmp = "  ";
  tmp[0] = TP_IAC;
  tmp[1] = TP_SB;

  options = tmp;

  tmp[1] = option;
  options = options + tmp[1..1] + arguments;

  tmp[1] = TP_SE;
  options = options + tmp;

  return binary_message(options);
}

void should_suppress_ga(int do_suppress) {
  if(SYSTEM()
     || previous_object() == MUDCLIENTD->get_telopt_handler(TELOPT_SGA)) {
    suppress_ga = do_suppress;
  } else
    error("Only privileged code may call should_suppress_ga!");
}

/************************************************************************/
/************************************************************************/
/************************************************************************/
/* Specifics of telnet protocol                                         */
/************************************************************************/
/************************************************************************/
/************************************************************************/

private string debug_escape_str(string line) {
  string ret;
  int    ctr;

  ret = "";
  for(ctr = 0; ctr < strlen(line); ctr++) {
  /*if(line[ctr] >= 32 && line[ctr] <= 127)
      ret += line[ctr..ctr];
      else */
      ret += "\\" + line[ctr];
  }

  return ret;
}

private string double_iac_filter(string input_line) {
  string pre, post, outstr, iac_str;

  outstr = "";
  post = input_line;
  iac_str = " "; iac_str[0] = TP_IAC;
  while(sscanf(post, "%s" + iac_str + iac_str + "%s", pre, post) == 2) {
    outstr += pre + iac_str;
  }
  outstr += post;

  return outstr;
}

private void negotiate_option(int command, int option) {
  object handler;

  handler = MUDCLIENTD->get_telopt_handler(option);
  if(handler) {
    switch(command) {
    case TP_WILL:
      handler->telnet_will(option);
      break;
    case TP_WONT:
      handler->telnet_wont(option);
      break;
    case TP_DO:
      handler->telnet_do(option);
      break;
    case TP_DONT:
      handler->telnet_dont(option);
      break;
    }
  } else {
    /* If no handler, ignore */
    LOGD->write_syslog("Ignoring telnet option " + option, LOG_WARN);
  }
}

/* This function is called on any subnegotiation string sent.  The string
   passed in was originally between an IAC SB and an IAC SE. */
private void subnegotiation_string(string str) {
  object handler;

  /* First, remove doubling of TP_IAC characters in string */
  str = double_iac_filter(str);

  handler = MUDCLIENTD->get_telopt_handler(str[0]);
  if(handler) {
    handler->telnet_sb(str[0], str[1..]);
  } else {
    /* Now, ignore it.  We don't yet accept subnegotiation on that option. */
    LOGD->write_syslog("Ignoring subnegotiation, option " + str[0] + ": '"
		       + debug_escape_str(str) + "'", LOG_WARN);
  }
}

/* Scan off a series starting with TP_IAC and return the remainder.
   The function will return nil if the series isn't a complete IAC
   sequence.  The caller is assumed to have stripped off the leading
   TP_IAC character of 'series'.  The caller has also already filtered
   for double-TP_IAC series, so we don't have to worry about those.
*/
static string scan_iac_series(string series) {
  string pre, post, iac_str, se_str;

  /* If there's nothing, we're not done yet */
  if(!series || !strlen(series))
    return nil;

  switch(series[0]) {
  case TP_WILL:
  case TP_WONT:
  case TP_DO:
  case TP_DONT:
    if(strlen(series) < 2)
      return nil;

    LOGD->write_syslog("Processing option: " + series[0] + " / " + series[1],
		       LOG_WARN);
    negotiate_option(series[0], series[1]);
    return series[2..];

  case TP_SB:
    /* Scan for IAC SB ... IAC SE sequence */
    iac_str = " "; iac_str[0] = TP_IAC;
    se_str = " "; se_str[0] = TP_SE;
    if(sscanf(series, "%s" + iac_str + se_str + "%s", pre, post) == 2) {
      LOGD->write_syslog("Processing sub-neg: IAC SB '" + pre[1] + " "
			 + debug_escape_str(pre[2..]) + "' IAC SE",
			 LOG_WARN);

      subnegotiation_string(pre[1..]);
      return post;
    } else {
      /* Don't have whole series yet */
      return nil;
    }
    break;

  default:
    /* Unrecognized, ignore */
    return series[1..];
  }
}

/*
 * This function filters for newlines (CR,LF, or CRLF) and backspaces.
 * It then chops up input into the line array.  Much code taken from
 * the Kernel Library binary connection object.
 */
static void crlfbs_filter(void)
{
    int mode, len;
    string str, head, pre;

    while (this_object() &&
	   (mode=query_mode()) != MODE_BLOCK && mode != MODE_DISCONNECT)
      {

	/* We only bother to process the buffer for this stuff if
	   there's a line waiting. */
	if (sscanf(buffer, "%s\r\n%s", str, buffer) != 0 ||
	    sscanf(buffer, "%s\r\0%s", str, buffer) != 0 ||
	    sscanf(buffer, "%s\r%s", str, buffer) != 0 ||
	    sscanf(buffer, "%s\n%s", str, buffer) != 0) {

	  while (sscanf(str, "%s\b%s", head, str) != 0) {

	    /* Process 'DEL' character in a string with backspaces */
	    while (sscanf(head, "%s\x7f%s", pre, head) != 0) {
	      len = strlen(pre);
	      if (len != 0) {
		head = pre[0 .. len - 2] + head;
	      }
	    }

	    len = strlen(head);
	    if (len != 0) {
	      str = head[0 .. len - 2] + str;
	    }
	  }

	  /* Process 'DEL' character after all backspaces are gone */
	  while (sscanf(str, "%s\x7f%s", head, str) != 0) {
	    len = strlen(head);
	    if (len != 0) {
	      str = head[0 .. len - 2] + str;
	    }
	  }

	  input_lines += ({ str });

	  /* Currently, log all input */
	  LOGD->write_syslog("MCC input: '" + str + "'", LOG_WARN);
	} else {
	  break; /* No more newline-delimited input.  Out of full lines. */
	}
      }
}

/*
 * Add new characters to buffer.  Filter newlines, backspaces and
 * telnet IAC codes appropriately.  If a full line of input has been
 * read, set input_line appropriately.
 */
private int new_telnet_input(string str) {
  string iac_series, iac_str, chunk, tmpbuf, post, series;

  iac_str = " "; iac_str[0] = TP_IAC;
  buffer += str;
  iac_series = "";
  tmpbuf = "";

  /* Note: can't use double_iac_filter function here, because then we
     might collapse double-IAC sequences in the wrong place in the
     input and it'd corrupt other IAC sequences. */

  /* Scan for TP_IAC. */
  while(sscanf(buffer, "%s" + iac_str + "%s", chunk, series) == 2) {
    tmpbuf += chunk;
    if(strlen(series) && series[0] == TP_IAC) {
      tmpbuf += iac_str;
      buffer = series[1..];
      continue;
    }
    post = scan_iac_series(series);
    if(!post) {
      /* Found an incomplete IAC series, wait for the rest */
      LOGD->write_syslog("Incomplete IAC series: '" + debug_escape_str(series)
			 + "'", LOG_WARN);
      iac_series = iac_str + series;
      break;
    }
    buffer = post;
  }
  buffer = tmpbuf + buffer;

  crlfbs_filter();  /* Handle newline stuff, backspace and DEL.  Chunk out
		       complete lines into input_lines. */
  buffer += iac_series;  /* Add back incomplete IAC series, if any */
}
