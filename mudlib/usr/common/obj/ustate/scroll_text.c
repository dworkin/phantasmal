#include <config.h>
#include <kernel/user.h>

inherit USER_STATE;

private mapping text;
private int     is_clone;

static void create(varargs int clone) {
  ::create();
  is_clone = clone;
  if(clone) {
  } else {
    text = ([ ]);
  }
}


string get_text(void) {
  if(is_clone) return nil;
  return text[previous_object()];
}

void set_text(string newtext) {
  if(is_clone) error("Can't set text of a clone!");
  text[previous_object()] = newtext;
}

void append_text(string newtext) {
  string tmp;

  tmp = text[previous_object()];
  if(!tmp) tmp = "";
  tmp += newtext;
  text[previous_object()] = tmp;
}

void scroll_page(void) {
  mixed* lines, *firstlines;
  string text;
  int    num_lines;

  text = call_other(US_SCROLL_TEXT, "get_text");
  lines = explode(text, "\n");
  num_lines = user->get_num_lines() - 1;
  if(num_lines >= sizeof(lines)) {
    firstlines = lines;
    lines = ({ });
  } else {
    firstlines = lines[..num_lines-1];
    lines = lines[num_lines..];
  }

  text = implode(lines, "\n");
  call_other(US_SCROLL_TEXT, "set_text", text);

  text = implode(firstlines, "\n");
  user->message(text);

  /* Print prompt at bottom */
  user->message("*** (enter to scroll forward, q to quit) ***\r\n");
}


/* USER_STATE functions */

int from_user(string input) {
  if(!input) return MODE_ECHO;

  input = STRINGD->trim_whitespace(input);
  if(input == "") {
    scroll_page();
    return MODE_ECHO;
  }
  if(input == "q" || input == "quit") {
    call_other(US_SCROLL_TEXT, "set_text", nil);
    pass_data(nil);
    return MODE_ECHO;
  }

  user->message("*** Don't recognize command: " + input + "\r\n");
  return MODE_ECHO;
}

void to_user(string output) {
  call_other(US_SCROLL_TEXT, append_text, output);
}


void switch_to(int pushp) {
  /* If we pop back to an empty SCROLL_TEXT state, get rid of it */
  if(!pushp) {
    tmp = call_other(US_SCROLL_TEXT, get_text);
    if(!tmp || tmp = "");
    pop_state();
    return;
  }

  send_string("Enter text at the prompt.  Enter ~ on a line by itself to " +
	      "end.\n");
  send_string(" > ");
}

void switch_from(int popp) {
  call_other(US_SCROLL_TEXT, "set_text", nil);
}
