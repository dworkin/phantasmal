#include <kernel/kernel.h>
#include <config.h>
#include <log.h>
#include <type.h>

/* User-state processing stack */
private object* state_stack;
private mixed   state_data;

static  mapping state;		/* state for a connection object */
private object  scroll_state;

/* Saved by save_object? */
int    num_lines;               /* how many lines on terminal */

/* Prototypes */
static  int    process_message(string str);
static  void   print_prompt(void);
        void   push_state(object state);

/* Macros */
#define NEW_PHRASE(x) PHRASED->new_simple_english_phrase(x)

void create(void) {
    state = ([ ]);

    state_stack = ({ });

    /* More defaults */
    num_lines = 20;

}

void upgraded(void) {
  if(!find_object(US_SCROLL_TEXT)) { compile_object(US_SCROLL_TEXT); }
}


int get_num_lines(void) {
  return num_lines;
}

static void set_num_lines(int new_num) {
  num_lines = new_num;
}

void set_state(object key_obj, int new_state) {
  state[key_obj] = new_state;
}

int get_state(object key_obj) {
  return state[key_obj];
}


/****** USER_STATE stack implementation ********/

void to_state_stack(string str) {
  state_stack[0]->to_user(str);
}

int message_scroll(string str) {
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
  int first_state;

  if(!state_stack || !sizeof(state_stack)) {
    destruct_object(state);
    error("Popping empty stack!");
  }

  if(!(state_stack && ({ state })))
    error("Popping state not in stack!");

  if(state_stack[0] == state)
    first_state = 1;
  else
    first_state = 0;

  state_stack = state_stack - ({ state });

  if(first_state) {
    state->switch_from(1);  /* 1 because popp is true */
    if(sizeof(state_stack)) {
      state_stack[0]->switch_to(0);  /* 0 because pushp is false */
    }
    /* No longer print prompt here, that's handled in receive_message. */
  }

  destruct_object(state);
}

void push_state(object state) {
  if(!SYSTEM())
    error("Only privileged code can call push_state()!");

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

object peek_state(void) {
  if(!state_stack || !sizeof(state_stack))
    return nil;

  return state_stack[0];
}

static void set_state_data(mixed data) {
  state_data = data;
}


/****************/

/*
 * NAME:	message_all_users()
 * DESCRIPTION:	send message to listening users
 */
static void message_all_users(string str)
{
    object *users, user;
    int i;

    users = users();
    for (i = sizeof(users); --i >= 0; ) {
	user = users[i];
	if (user != this_object() &&
	    sscanf(object_name(user), SYSTEM_USER + "#%*d") != 0) {
	    user->message(str);
	}
    }
}


/*
 * NAME:	system_phrase_all_users()
 * DESCRIPTION:	send message to listening users
 */
static void system_phrase_all_users(string str)
{
    object *users, user;
    int i;

    users = users();
    for (i = sizeof(users); --i >= 0; ) {
	user = users[i];
	if (user != this_object() &&
	    sscanf(object_name(user), SYSTEM_USER + "#%*d") != 0) {
	    user->send_system_phrase(str);
	}
    }
}

/*
static void system_phrase_room(object room, string str)
{
  object phr;

  phr = PHRASED->file_phrase(SYSTEM_PHRASES, str);
  if(!phr)
    error("Can't get system phrase " + str);

  tell_room(room, phr);
}
*/

/*
 * NAME:	tell_room()
 * DESCRIPTION:	send message to everybody in specified location
 */
/*
static void tell_room(object room, mixed msg)
{
// replace these with proper comments if this is ever uncommented.
// tell everyone in the room except the person themselves
  room->tell_room(msg, ({ body }) );
}
*/

mixed state_receive_message(string str) {
  return state_stack[0]->from_user(str);
}
