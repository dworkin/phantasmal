string type_for_tag(string tag_name) {
  string type;

  if(sscanf(tag_name, "custom:/%s", type)) {
    return "/usr/game/rooms/" + type;
  }

  return nil;
}
