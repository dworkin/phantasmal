/* Includable files */
/* Note the .h on the end -- it's intentional */
#define INHERIT_SCRIPT_AUTO "/include/phantasmal/inherit_script_auto.h"
#define INHERIT_COMMON_AUTO "/include/phantasmal/inherit_common_auto.h"


/* Checking to see if the caller is authorized */
# define COMMON() (sscanf(previous_program(), USR_DIR + "/common/%*s") != 0)
# define GAME()	  (sscanf(previous_program(), USR_DIR + "/game/%*s") != 0)

/* System managers and daemons */
#define TELNETD     "/usr/System/sys/telnetd"
#define OBJECTD     "/usr/System/sys/objectd"
#define ERRORD      "/usr/System/sys/errord"
#define LOGD        "/usr/System/sys/logd"
#define PHRASED     "/usr/System/sys/phrased"
#define HELPD       "/usr/System/sys/helpd"
#define SOUNDEXD    "/usr/common/sys/soundexd"
#define STRINGD     "/usr/common/sys/stringd"
#define MAPD        "/usr/common/sys/mapd"
#define EXITD       "/usr/common/sys/exitd"
#define ZONED       "/usr/common/sys/zoned"
#define MOBILED     "/usr/common/sys/mobiled"
#define OBJNUMD     "/usr/common/sys/objnumd"
#define CHANNELD    "/usr/common/sys/channeld"
#define CONFIGD     "/usr/common/sys/configd"
#define TIMED       "/usr/common/sys/timed"
#define SOULD       "/usr/common/sys/sould"
#define PARSED      "/usr/common/sys/parsed"
#define TAGD        "/usr/common/sys/tagd"
#define PATHSPECIAL "/usr/common/sys/pathspecial"


/* The InitD must be in this location to be recognized as privileged */
#define INITD               "/usr/System/initd"


/* GameLib tie-in objects */
#define PHANTASMAL_USER     "/usr/System/open/lib/userlib"


/* System Libraries -- inheritable, not clonable */
#define SYSTEM_WIZTOOLLIB   "/usr/System/lib/wiztoollib"
#define SYSTEM_USER_IO      "/usr/System/lib/user_io"
#define USER_STATE          "/usr/common/lib/user_state"
#define ISSUE_LWO           "/usr/common/lib/issue_lwo"
#define DTD_UNQABLE         "/usr/common/lib/dtd_unqable"
#define TAGGED              "/usr/common/lib/tagged"
#define PHRASE_REPOSITORY   "/usr/common/lib/phrase_repository"
#define SCRIPT_AUTO         "/usr/common/lib/script_auto"


/* Libraries for fundamental game objects */
#define OBJECT              "/usr/common/lib/object"
#define ROOM                "/usr/common/lib/room"
#define EXIT                "/usr/common/lib/exit"
#define MOBILE              "/usr/common/lib/mobile"


/* Clonable MUD structures */
#define SYSTEM_WIZTOOL      "/usr/System/obj/wiztool"
#define SYSTEM_USER_OBJ     "/usr/System/obj/user"
#define DEFAULT_USER_OBJ    "/usr/game/obj/user"
#define SIMPLE_EXIT         "/usr/common/obj/simple_exit"
#define SIMPLE_ROOM         "/usr/common/obj/simple_room"
#define UNQ_PARSER          "/usr/common/obj/basic_unq_parser"
#define UNQ_DTD             "/usr/common/obj/unq_dtd"
#define HEAVY_ARRAY         "/usr/common/obj/heavy_array"


/* User State types (also clonable) */
#define US_ENTER_DATA       "/usr/common/obj/ustate/enter_data"
#define US_ENTER_YN         "/usr/common/obj/ustate/enter_yn"
#define US_OBJ_DESC         "/usr/common/obj/ustate/set_obj_desc"
#define US_SCROLL_TEXT      "/usr/common/obj/ustate/scroll_text"
#define US_MAKE_ROOM        "/usr/common/obj/ustate/makeroom"


/* Lightweight objects */
#define LWO_PHRASE          "/usr/common/data/lwo_phrase"


/* Storage directories */
#define LOGDIR              "/log"


/* Non-LPC configuration files */

/* UNQ DTDs */
#define MAPD_ROOM_DTD       "/usr/common/sys/room.dtd"
#define MOB_FILE_DTD        "/usr/common/sys/mobile.dtd"
#define ZONED_DTD           "/usr/common/sys/zoned.dtd"
#define BIND_DTD            "/usr/common/sys/bind.dtd"
#define SOULD_DTD           "/usr/common/sys/sould.dtd"
#define CONFIGD_DTD         "/usr/common/sys/config.dtd"

/* Save locations, used by INITD and the wiztool */
#define ROOM_DIR            "/usr/game/object/stuff"
#define MOB_FILE            "/usr/game/object/mobfile.unq"
#define ZONE_FILE           "/usr/game/object/zonefile.unq"
#define SAFE_ROOM_DIR       "/usr/game/object/safe_stuff"
#define SAFE_MOB_FILE       "/usr/game/object/safe_mobfile.unq"
#define SAFE_ZONE_FILE      "/usr/game/object/safe_zonefile.unq"

/* Other data files */
#define SYSTEM_PHRASES      "/usr/System/obj/system.phr"
