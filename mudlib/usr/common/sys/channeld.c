#include <config.h>
#include <log.h>
#include <channel.h>
#include <type.h>

mixed*  channels;
int     num_channels;
mixed*  channel_attributes;

#define ATTRIB_ADMIN       1

/* Prototypes */
void upgraded(varargs int clone);
int is_subscribed(object user, int channel);


static void create(varargs int clone) {
  if(clone) {
    error("Can't clone CHANNELD!");
  }

  upgraded();
}

void upgraded(varargs int clone) {
  int ctr;

  num_channels = 3;
  channels = allocate(num_channels);

  for(ctr = 0; ctr < num_channels; ctr++) {
    channels[ctr] = ([ ]);
  }

  channel_attributes = ({ ({ "OOC", 0 }),
			    ({ "Error", ATTRIB_ADMIN }),
			    ({ "Gossip", 0 }),
			    });
}

mixed* channel_list(int is_admin) {
  mixed* ret, *tmp;
  int    ctr;

  ret = ({ });
  for(ctr = 0; ctr < num_channels; ctr++) {
    if(is_admin || !(channel_attributes[ctr][1] & ATTRIB_ADMIN)) {
      tmp = channel_attributes[ctr];
      ret += ({ ({ PHRASED->new_simple_english_phrase(tmp[0]), ctr }) });
    }
  }

  return ret;
}

/* The user will be used for the locale eventually. */
int get_channel_by_name(string name, object user) {
  int ctr;

  for(ctr = 0; ctr < sizeof(channel_attributes); ctr++) {
    if(!STRINGD->stricmp(channel_attributes[ctr][0], name)) {
      return ctr;
    }
  }

  return -1;
}

void phrase_to_channel(int channel, object phrase) {
  int    ctr;
  mixed* keys;

  keys = map_indices(channels[channel]);
  for(ctr = 0; ctr < sizeof(keys); ctr++) {
    channels[channel][keys[ctr]][0]->send_phrase(phrase);
  }
}

void string_to_channel(int channel, string str) {
  int    ctr;
  mixed* keys;

  keys = map_indices(channels[channel]);
  for(ctr = 0; ctr < sizeof(keys); ctr++) {
    channels[channel][keys[ctr]][0]->message(str);
  }
}

int subscribe_user(object user, int channel, string args) {
  int    attrib, ctr;

  if(channel < CHANNEL_OOC || channel > num_channels) {
    return -1;
  }
  attrib = channel_attributes[channel][1];

  if((attrib & ATTRIB_ADMIN) && !user->is_admin()) {
    return -1;
  }

  if(is_subscribed(user, channel))
    return -1;

  channels[channel][user->query_name()] = ({ user, args });
  return 1;
}

int unsubscribe_user(mixed user, int channel) {
  string name;

  if(typeof(user) == T_STRING) {
    name = user;
  } else if (typeof(user) == T_OBJECT) {
    name = user->query_name();
  }

  if(channels[channel][name] ) {
    /* Remove user's entry */
    channels[channel][name] = nil;

    if(channels[channel][name]) {
      LOGD->write_syslog("Failed unsubscribe attempt!", LOG_WARNING);
    }

    return 1;
  }

  return -1;
}

void unsubscribe_user_from_all(object user) {
  int ctr, chan;

  for(chan = 0; chan < num_channels; chan++) {
    unsubscribe_user(user, chan);
  }
}

int is_subscribed(object user, int channel) {
  int ctr;

  if(channels[channel][user->query_name()]) {
    return 1;
  }

  return 0;
}
