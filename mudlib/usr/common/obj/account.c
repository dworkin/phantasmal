#include <config.h>

/* account.c:
 *
 * Defines the "account" object which defines the player as opposed to that
 * player's characters.  The player may have many logins, but each login
 * by that player will link to the same account object.
 *
 */

/* Saved by save_object */
string password;		/* user password */
int    locale;                  /* chosen output locale */
string name;	                /* user name */

