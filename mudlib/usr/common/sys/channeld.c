#include <config.h>
#include <log.h>
#include <channel.h>

mixed*  channels;
int     num_channels;
mapping channel_attributes;

#define ATTRIB_ADMIN       1

/* Prototypes */
void upgraded(varargs int clone);


static void create(varargs int clone) {
  if(clone) {
    error("Can't clone CHANNELD!");
  }

  upgraded();
}

void upgraded(varargs int clone) {
  int ctr;

  num_channels = 32;
  channels = allocate(num_channels);

  for(ctr = 0; ctr < num_channels; ctr++) {
    channels[ctr] = ({ });
  }

  channel_attributes = ([ CHANNEL_OOC              : ({ }),
			  CHANNEL_ERR              : ({ ATTRIB_ADMIN }),
			  CHANNEL_GOSSIP           : ({ }),
			  ]);
}

void phrase_to_channel(int channel, object phrase) {
  int ctr;

  for(ctr = 0; ctr < sizeof(channels[channel]); ctr++) {
    channels[ctr][0]->send_phrase(phrase);
  }
}

void string_to_channel(int channel, string str) {
  int ctr;

  for(ctr = 0; ctr < sizeof(channels[channel]); ctr++) {
    channels[ctr][0]->message(str);
  }
}

int subscribe_user_to_channel(object user, int channel, string args) {
  mixed* attrib;
  int    ctr;

  attrib = channel_attributes[channel];
  if(!attrib) {
    LOGD->write_syslog("No attributes for channel #" + channel + "!",
		       LOG_WARN);
    attrib = ({ });
  }

  for(ctr = 0; ctr < sizeof(attrib); ctr++) {
    if(attrib[ctr] == ATTRIB_ADMIN) {
      if(!user->is_admin())
	return -1;
    } else {
      LOGD->write_syslog("Unknown attribute " + attrib[ctr]);
      return -1;
    }
  }

}
