/*
 * Gameconfig.h
 *
 * This file tells Phantasmal things about your configuration.  You can
 * also keep your own stuff in here
 *
 */

/*
 * You have to define GAME_INITD so that Phantasmal knows what to call
 * when you start up.  Other configuration is done in this object's
 * create() function, which will call ConfigD to set everything else
 * that Phantasmal needs to know.
 */
#define GAME_INITD            "/usr/game/initd"
#define GAME_DRIVER           "/usr/game/sys/gamedriver"

/*
 * These are just for the default setup.  You can remove any of them
 * as long as you get rid of the references to them in /usr/game.  Feel
 * free to add new entries here as well.
 */
#define PATHAUTOD             "/usr/game/sys/pathautod"
#define INHERIT_SCRIPT_AUTO   "/usr/game/include/inherit_script_auto.h"
#define SCRIPT_AUTO_OBJECT    "/usr/game/lib/script_auto"

#define CONFIGD_DTD         "/usr/common/sys/config.dtd"

#define HEART_BEAT            "/usr/game/sys/heart_beat"

#define GAME_VERSION          "0.001"
