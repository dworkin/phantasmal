#include <config.h>
#include <log.h>
#include <channel.h>

mixed*  channels;
int     num_channels;
mapping channel_attributes;

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
    channels[ctr] = ({ });
  }

  channel_attributes = ([ CHANNEL_OOC              : ({ "OOC", 0 }),
			  CHANNEL_ERR              : ({ "Error",
							  ATTRIB_ADMIN }),
			  CHANNEL_GOSSIP           : ({ "Gossip", 0 }),
			  ]);
}

object* channel_list(int is_admin) {
  mixed* ret;
  int    ctr;

  ret = ({ });
  for(ctr = 0; ctr < num_channels; ctr++) {
    if(is_admin || !(channel_attributes[ctr][1] & ATTRIB_ADMIN)) {
      ret +=
	({ PHRASED->new_simple_english_phrase(channel_attributes[ctr][0]) });
    }
  }

  return ret;
}

void phrase_to_channel(int channel, object phrase) {
  int ctr;

  for(ctr = 0; ctr < sizeof(channels[channel]); ctr++) {
    channels[channel][ctr][0]->send_phrase(phrase);
  }
}

void string_to_channel(int channel, string str) {
  int ctr;

  for(ctr = 0; ctr < sizeof(channels[channel]); ctr++) {
    channels[channel][ctr][0]->message(str);
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

  channels[channel] += ({ ({ user, args }) });
  return 1;
}

int is_subscribed(object user, int channel) {
  int ctr;

  for(ctr = 0; ctr < sizeof(channels[channel]); ctr++) {
    if(channels[channel][ctr][0] == user ) {
      return 1;
    }
  }

  return 0;
}
