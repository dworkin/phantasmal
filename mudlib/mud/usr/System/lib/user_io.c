#include <kernel/kernel.h>
#include <kernel/user.h>

#include <phantasmal/log.h>
#include <phantasmal/phrase.h>
#include <phantasmal/lpc_names.h>

#include <type.h>

/*
 * This library implements the user-side stuff for terminal
 * processing, user state stacks and similar tricks involving telnet,
 * MUD clients, and various forms of user interaction that doesn't
 * take place at the regular command prompt.
 */

inherit COMMON_AUTO;
inherit LIB_USER;

/* User-state processing stack */
private object* state_stack;

private object  scroll_state;

/* Terminal-type variables: */
private mapping substitutions;
private int     subs_correct;
private int     mudclient_conn;

/* Saved by save_object */
int    num_lines;               /* how many lines the terminal has */
int    num_cols;                /* how many columns the terminal has */
int    locale;                  /* chosen output locale */

/* Prototypes */
void   push_state(object state);
object peek_state(void);
void   to_state_stack(string str);
static nomask int send_string(string str);
void   upgraded(varargs int clone);

/* Macros */
#define NEW_PHRASE(x) PHRASED->new_simple_english_phrase(x)

static void create(varargs int clone) {
    if(clone) {
      /* More defaults */
      num_lines = 20;
      num_cols = 78;

      /* Default to enUS locale */
      locale = PHRASED->language_by_name("english");

      state_stack = ({ });
    }

    upgraded(clone);
}

void upgraded(varargs int clone) {
  if(SYSTEM()) {
    if(!find_object(US_SCROLL_TEXT)) { compile_object(US_SCROLL_TEXT); }
  } else
    error("Non-System code called upgraded!");
}


int get_num_lines(void) {
  if(!SYSTEM() && !COMMON() && !GAME())
    error("Only privileged code can call get_num_lines!");

  return num_lines;
}

static void set_num_lines(int new_num) {
  num_lines = new_num;
}

int get_locale(void) {
  return locale;
}

static void set_locale(int new_loc) {
  locale = new_loc;
}


/****** User Message Functions *****************/

int message(string str) {
  if(!SYSTEM() && !COMMON() && !GAME())
    return -1;

  if(peek_state()) {
    to_state_stack(str);
  } else {
    return send_string(str);
  }
}

/*
 * NAME:        message_all_users()
 * DESCRIPTION: send message to listening users
 */
static void message_all_users(string str)
{
    object *users, user;
    int i;

    if(!SYSTEM() && !COMMON() && !GAME())
      return;

    users = users();
    for (i = sizeof(users); --i >= 0; ) {
        user = users[i];
        if (user && user != this_object()) {
            user->message(str);
        }
    }
}

/*
 * NAME:        system_phrase_all_users()
 * DESCRIPTION: send message to listening users
 */
static void system_phrase_all_users(string str)
{
    object *users, user;
    int i;

    if(!SYSTEM() && !COMMON() && !GAME())
      return;

    users = users();
    for (i = sizeof(users); --i >= 0; ) {
        user = users[i];
        if (user != this_object()) {
            user->send_system_phrase(str);
        }
    }
}

/* This sends a Phrase, allowing locale and terminal settings to
   affect output */
int send_phrase(object PHRASE obj) {
  if(!SYSTEM() && !COMMON() && !GAME())
    error("Can't send phrase!  Not privileged!");

  return message(obj->to_string(this_object()));
}

int send_system_phrase(string phrname) {
  object PHRASE phr;

  if(!SYSTEM() && !COMMON() && !GAME())
    error("Can't send system phrase!  Not privileged!");

  phr = PHRASED->file_phrase(SYSTEM_PHRASES, phrname);
  if(!phr) {
    LOGD->write_syslog("Can't find system phrase " + phrname + "!", LOG_ERR);
    return -1;
  }
  return send_phrase(phr);
}

/****** USER_STATE stack implementation ********/

/* This is called only by the USER_STATE object, and is used to send
   already-filtered output on the channel.
*/
nomask int ustate_send_string(string str) {
  if(previous_program() == USER_STATE)
    return ::message(str);
  else
    error("Only USER_STATE can call PHANTASMAL_USER:ustate_send_string!");
}

/* This does a lowest-level, unfiltered send to the connection object
   itself.  Normally sends will be filtered through the user_state
   object(s) active, if any, but this function is different. */
static nomask int send_string(string str) {
  return ::message(str);
}

void to_state_stack(string str) {
  if(SYSTEM() || COMMON() || GAME()) {
    state_stack[0]->to_user(str);
  }
}

void message_scroll(string str) {
  if(!SYSTEM() && !COMMON() && !GAME())
    error("Only privileged code can call message_scroll!");

  if(scroll_state) {
    scroll_state->to_user(str);
  } else {
    scroll_state = clone_object(US_SCROLL_TEXT);
    if(scroll_state) {
      scroll_state->add_text(str);
      push_state(scroll_state);
    } else {
      LOGD->write_syslog("Couldn't clone US_SCROLL_TEXT state object!",
                         LOG_ERROR);
    }
  }
}

void notify_done_scrolling(void) {
  if(previous_object() != scroll_state)
    error("Only our own scrolling state can notify a user it's done!");

  scroll_state = nil;

}

void pop_state(object state) {
  int    first_state;  /* This is a boolean value */
  int    ctr;
  object prev_state, next_state;

  if(!SYSTEM() && !COMMON() && !GAME())
    error("Unprivileged code calling pop_state!");

  if(!state_stack || !sizeof(state_stack))
    error("Popping empty stack!");

  if(!(state_stack && ({ state })))
    error("Popping state not in stack!");

  if(state_stack[0] == state)
    first_state = 1;
  else {
    first_state = 0;
    for(ctr = 1; ; ctr++) {
      if(state_stack[ctr] == state) {
        prev_state = state_stack[ctr - 1];
        if(ctr + 1 < sizeof(state_stack))
          next_state = state_stack[ctr + 1];
        break;
      }
    }
  }

  state_stack = state_stack - ({ state });

  if(first_state) {
    state->switch_from(1);  /* 1 because popp is true */
    if(sizeof(state_stack)) {
      state_stack[0]->switch_to(0);  /* 0 because pushp is false */
    }
    /* No longer print prompt here, that's handled in receive_message. */
  } else {
    /* Make sure that the next_state values are correct in user state. */
    prev_state->init(this_object(), next_state);
  }

  destruct_object(state);
}

void push_state(object state) {
  if(!SYSTEM() && previous_program() != USER_STATE)
    error("Only privileged code can call push_state()!");

  if(!SYSTEM() && !COMMON() && !GAME())
    error("Unprivileged code calling SYSTEM_USER_IO:push_state!");

  if(!state_stack) {
    state_stack = ({ });
  }

  if(sizeof(state_stack)) {
    state->init(this_object(), state_stack[0]);
    state_stack[0]->switch_from(0);  /* 0 because popp is false */
  } else {
    state->init(this_object(), nil);
  }

  state_stack = ({ state }) + state_stack;

  /* Call switch_to() after adding the new state to the stack --
     'cause it can pop the state right back off in some cases... */
  state->switch_to(1); /* 1 because pushp is true */
}

void push_new_state(mixed state_type, mixed params...) {
  object new_state;

  if(!SYSTEM() && !COMMON() && !GAME())
    error("Unprivileged code calling SYSTEM_USER_IO:push_new_state!");

  new_state = clone_object(state_type);
  if(!new_state)
    error("Can't create new state to push in push_new_state()!");

  new_state->set_up_func(params...);
  push_state(new_state);
}

object peek_state(void) {
  if(!SYSTEM() && !COMMON() && !GAME())
    error("Only privileged code can call peek_state!");

  if(!state_stack || !sizeof(state_stack))
    return nil;

  return state_stack[0];
}

mixed state_receive_message(string str) {
  if(!SYSTEM() && !COMMON() && !GAME())
    error("Not privileged!");

  return state_stack[0]->from_user(str);
}

/****************/


/**********************************************/

/* UNQ markup stuff */

void set_up_substitutions(void) {
  object  conn;
  mapping info;
  string *colors;
  int     ctr;

  if(subs_correct)
    return;

  conn = this_object()->query_conn();
  if(conn) {
    if(sscanf(object_name(conn), MUDCLIENT_CONN + "#%*d") == 1) {
      LOGD->write_syslog("Is MUDclient conn!");
      mudclient_conn = 1;
    } else {
      LOGD->write_syslog("Not MUDclient conn!");
      mudclient_conn = 0;
    }
  } else {
    return;  /* Can't set up subs yet */
  }

  colors = ({ "black", "red", "yellow", "green", "blue", "magenta", "cyan", "white" });

  /* Currently hardcode, no locale stuff */
  substitutions = ([ "{enUS" : "", "}enUS" : "" ]);

  if(!mudclient_conn) {
    /* This connection was made on a non-MUDclient port.  That means
       no term types, no ANSI color, no window size... */

    for(ctr = 0; ctr < sizeof(colors); ctr++) {
      substitutions["{" + colors[ctr]] = "";
      substitutions["}" + colors[ctr]] = "";
    }

    subs_correct = 1;
    return;
  }

  info = conn->terminal_info();
  if(info["protocol"] == "telnet") {
    string esc_start;

    esc_start = "\033[";
    for(ctr = 0; ctr < sizeof(colors); ctr++) {
      substitutions["}" + colors[ctr]] = esc_start + "0m";
    }
    substitutions += ([
                       "{black" : esc_start + "30m",
                       "{red" : esc_start + "31m",
                       "{green" : esc_start + "32m",
                       "{yellow" : esc_start + "33m",
                       "{blue" : esc_start + "34m",
                       "{magenta" : esc_start + "35m",
                       "{cyan" : esc_start + "36m",
                       "{white" : esc_start + "37m",
                       "*reset" : esc_start + "0m",
                       ]);
  } else if(info["protocol"] == "imp") {
    for(ctr = 0; ctr < sizeof(colors); ctr++) {
      substitutions["{" + colors[ctr]] = "<FONT COLOR=\"" + colors[ctr] + "\">";
      substitutions["}" + colors[ctr]] = "</FONT>";
     }
    substitutions += ([
                       "*client-startup" : "<IMPDEMO>",
                       ]);
  } else {
    error("Unrecognized protocol when setting up substitution maps!");
  }

  subs_correct = 1;
}

int supports_tag(string tag) {
  if(!subs_correct)
    set_up_substitutions();

  if(substitutions[tag])
    return 1;

  return 0;
}

string taglist_to_string(mixed *taglist) {
  int ctr;
  string result;
  mapping my_subs;

  if(!SYSTEM() && !COMMON() && !GAME())
    error("Only privileged code may call this!");

  catch {

  if(!subs_correct)
    set_up_substitutions();

  if(!subs_correct) {
    /* Conn's not set up yet...  Trouble! */
    my_subs = ([ "{enUS" : "", "}enUS" : "" ]);
  } else {
    my_subs = substitutions;
  }

  result = "";

  for(ctr = 0; ctr < sizeof(taglist); ctr += 2) {
    if(my_subs[taglist[ctr]]) {
      /* do stuff */
      result += my_subs[taglist[ctr]];
    } else {
      if(taglist[ctr] != "" && taglist[ctr][0] == '{') {
        string close_tag;

        close_tag = taglist[ctr];
        close_tag[0] = '}';
        /* Skip until closing tag */
        while(taglist[ctr] != close_tag) ctr+=2;
      }
    }
    result += taglist[ctr + 1];
  }

  } : {
    LOGD->write_syslog("Error in taglist_to_string, with taglist '"
                       + STRINGD->mixed_sprint(taglist) + "', counter " + ctr, LOG_ERR);
  }

  return result;
}
