#include <gameconfig.h>

string type_for_tag(string tag_name) {
  string type;

  if(sscanf(tag_name, "custom:/%s", type)) {
    return GAME_ROOMS_DIR + type;
  }

  return nil;
}
