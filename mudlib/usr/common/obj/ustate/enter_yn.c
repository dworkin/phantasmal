#include <config.h>
#include <kernel/user.h>

inherit USER_STATE;

private string prompt;

static void create(varargs int clone) {
  ::create();
  if(clone) {
    prompt = "Yes or no? ";
  }
}

void set_prompt(string new_prompt) {
  prompt = new_prompt;
}

int from_user(string input) {
  if(input == "y" || input == "Y"
     || !STRINGD->stricmp(input, "yes")) {
    pass_data(1);

    pop_state();
    return MODE_ECHO;
  }
  if(input == "n" || input == "N"
     || !STRINGD->stricmp(input, "no")) {
    pass_data(0);

    pop_state();
    return MODE_ECHO;
  }

  send_string("That wasn't a definite 'yes' or 'no'.  Please try again.\r\n");
  send_string(prompt);

  return MODE_ECHO;
}

void switch_to(int pushp) {
  send_string(prompt);
}
