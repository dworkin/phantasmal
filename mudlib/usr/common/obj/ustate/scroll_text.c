#include <phantasmal/lpc_names.h>
#include <phantasmal/log.h>
#include <kernel/user.h>

inherit USER_STATE;

private mapping text;
private int     is_clone;
private int     active;

static void create(varargs int clone) {
  ::create();
  is_clone = clone;

  if(clone) {
    /* Not the current state (yet) */
    active = 0;
  } else {
    text = ([ ]);
  }
}

static void pop_state(void) {
  user->notify_done_scrolling();
  ::pop_state();
}

string get_text(void) {
  if(previous_program() != US_SCROLL_TEXT)
    error("Only scrollstates can call get_text!");

  if(is_clone) return nil;
  return text[previous_object()];
}

void set_text(string newtext) {
  if(previous_program() != US_SCROLL_TEXT)
    error("Only scrollstates can call set_text!");

  if(is_clone) error("Can't set text of a clone!");
  text[previous_object()] = newtext;
}

void append_text(string newtext) {
  string tmp;

  if(previous_program() != US_SCROLL_TEXT)
    error("Only scrollstates can call append_text!");

  tmp = text[previous_object()];
  if(!tmp) tmp = "";
  tmp += newtext;
  text[previous_object()] = tmp;
}

static void scroll_page(void) {
  mixed* lines, *firstlines;
  string text;
  int    num_lines;

  text = call_other(US_SCROLL_TEXT, "get_text");

  if(!text) {
    LOGD->write_syslog("No text found in US_SCROLL_TEXT:scroll_page!",
		       LOG_ERROR);
    return;
  }

  lines = explode("\n" + text + "\n", "\n");
  num_lines = user->get_num_lines() - 1;  /* -1 for status line */
  if(num_lines >= sizeof(lines)) {
    send_string(text);
    call_other(US_SCROLL_TEXT, "set_text", nil);
    pop_state();
    return;
  }

  firstlines = lines[..(num_lines-1)];
  lines = lines[num_lines..];

  text = implode(lines, "\n");
  call_other(US_SCROLL_TEXT, "set_text", text);

  text = implode(firstlines, "\r\n") + "\r\n";
  send_string(text);

  /* Print prompt at bottom */
  send_string("*** (enter to scroll forward, q to quit) ***\r\n");
}


/* USER_STATE functions */

void add_text(string str) {
  call_other(US_SCROLL_TEXT, "append_text", str);
}

int from_user(string input) {
  if(!input) return MODE_ECHO;

  input = STRINGD->trim_whitespace(input);
  if(input == "") {
    scroll_page();
    return MODE_ECHO;
  }
  if(input == "q" || input == "quit") {
    call_other(US_SCROLL_TEXT, "set_text", nil);
    pop_state();
    return MODE_ECHO;
  }

  send_string("*** Don't recognize command: " + input + "\r\n");
  return MODE_ECHO;
}

void to_user(string output) {
  call_other(US_SCROLL_TEXT, "append_text", output);

  if(!active) scroll_page();
}


void switch_to(int pushp) {
  string tmp;

  active = 1;

  /* If we pop back to an empty SCROLL_TEXT state, get rid of it */

  tmp = call_other(US_SCROLL_TEXT, "get_text");
  if(!tmp || tmp == "") {
    pop_state();
    return;
  }

  scroll_page();
}

void switch_from(int popp) {
  active = 0;

  /* call_other(US_SCROLL_TEXT, "set_text", nil); */
}
