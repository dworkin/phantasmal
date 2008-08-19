#include <kernel/user.h>
#include <phantasmal/lpc_names.h>

inherit USER_STATE;

private string full_block;

static void create(varargs int clone) {
  ::create();
  if(clone) {
  }
}

int from_user(string input) {
  if(input == "~" || input == "~\n") {
    pass_data(full_block ? full_block : "");
    full_block = nil;

    pop_state();
    return MODE_ECHO;
  }

  if(!full_block) full_block = "";

  full_block += input + "\n";
  send_string(" > ");

  return MODE_ECHO;
}

void switch_to(int pushp) {
  send_string("Enter text at the prompt.  Enter ~ on a line by itself to " +
	      "end.\r\n");
  send_string(" > ");
}
