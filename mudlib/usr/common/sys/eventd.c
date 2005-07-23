#include <kernel/kernel.h>

#include <phantasmal/lpc_names.h>

/*
  Phantasmal events are described in "http://phantasmal.sf.net/Design".

  Basic events consist of a representation (smell, taste, sound,
  vibration, etc), a perceptibility, and a Phrase to indicate what the
  perceiver actually sees/hears/tastes/etc.

  A Cascaded event consists of a list of events.  The perceiver will
  detect the first of the list of events that s/he is able to, and all
  further events will be ignored.

  A Composite event consists of a list of events, and an overall
  event.  If all events in the list can be perceived then it is
  assumed that the Composite event can be perceived as a whole, and
  the overall event may be substituted.

  A Timed event consists of a small set number of Instant events - for
  things like beginning, ending, and perhaps various sorts of
  interruption.
*/
