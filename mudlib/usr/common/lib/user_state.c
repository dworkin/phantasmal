#include <phantasmal/lpc_names.h>

object user;
object next_state;

/*
  The user_state object is used on a state stack of user input/output
  processing.  When it is pushed or everything above it has been
  popped, switch_to is called on it.  When another state is pushed
  "over" it, switch_from is called.  When it is the current state,
  input from the user first passes to from_user and output meant for
  the user first passes through to_user.  The state determines how the
  user or login object receive these messages, if at all.

  A user_state may pass messages to the next state in the stack, thus
  "filtering" a previous state.
*/

static void create(varargs int clone) {
  if(clone) {

  }
}

/* By default, this does nothing and accepts any parameters.  Subtypes
   may restrict it appropriately. */
void set_up_func(mixed params...) {
}

void init(object new_user, object new_next_state) {
  if(previous_program() != SYSTEM_USER_IO)
    error("Use push_new_state and pop_state functions for user states!");

  user = new_user;
  next_state = new_next_state;
}

static object get_user(void) {
  return user;
}

int from_user(string input) {
  error("Implement from_user!");
}

void to_user(string output) {
  if(next_state) {
    next_state->to_user(output);
  } else {
    user->ustate_send_string(output);
  }
}

void switch_to(int pushp) {
  /* Defaults to no-op */
}

void switch_from(int popp) {
  /* Defaults to no-op */
}

static int send_string(string str) {
  return user->ustate_send_string(str);
}

static void pop_state(void) {
  user->pop_state(this_object());
}

static void push_state(object state) {
  user->push_state(state);
}

static void push_new_state(mixed state_type, mixed params...) {
  user->push_new_state(state_type, params...);
}

static void pass_data(mixed data) {
  if(next_state) {
    next_state->pass_data(data);
  } else {
    user->user_state_data(data);
  }
}
