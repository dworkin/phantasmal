#define USR                 "/usr"
#define SECOND_AUTO_HEADER  "/usr/System/open/include/auto.h"
#define SECOND_AUTO         "/usr/System/open/lib/auto.c"


/* System managers and daemons */
#define TELNETD     "/usr/System/sys/telnetd"
#define OBJECTD     "/usr/System/sys/objectd"
#define ERRORD      "/usr/System/sys/errord"
#define LOGD        "/usr/System/sys/logd"
#define PHRASED     "/usr/System/sys/phrased"
#define HELPD       "/usr/common/sys/helpd"
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


/* Kernel lib tie-in objects -- must be in these dirs to be recognized
   as privileged */
#define INITD               "/usr/System/initd"
#define SYSTEM_USER         "/usr/System/obj/user"
#define SYSTEM_WIZTOOL      "/usr/System/obj/wiztool"
#define SYSTEM_WIZTOOLLIB   "/usr/System/lib/wiztoollib"


/* Libraries -- inheritable, not clonable */
#define USER_STATE          "/usr/common/lib/user_state"
#define ISSUE_LWO           "/usr/common/lib/issue_lwo"
#define UNQABLE             "/usr/common/lib/unqable"
#define DTD_UNQABLE         "/usr/common/lib/dtd_unqable"
#define PHRASE_REPOSITORY   "/usr/common/lib/phrase_repository"
#define PHRASE              "/usr/common/lib/phrase"
#define INTL_PHRASE         "/usr/common/lib/intl_phrase"
#define OBJECT              "/usr/common/lib/object"
#define ROOM                "/usr/common/lib/room"
#define DETAIL              "/usr/common/lib/detail"
#define EXIT                "/usr/common/lib/exit"
#define MOBILE              "/usr/common/lib/mobile"


/* Instantiable (clonable) MUD structures */
#define ACCOUNT             "/usr/common/obj/account"
#define SIMPLE_PHRASE       "/usr/common/obj/simple_phrase"
#define SIMPLE_DETAIL       "/usr/common/obj/simple_detail"
#define SIMPLE_EXIT         "/usr/common/obj/simple_exit"
#define USER_MOBILE         "/usr/common/obj/user_mobile"
#define SIMPLE_ROOM         "/usr/common/obj/simple_room"
#define SIMPLE_PORTABLE     "/usr/common/obj/simple_portable"
#define MOBILE_PORTABLE     "/usr/common/obj/mobile_portable"
#define UNQ_PARSER          "/usr/common/obj/basic_unq_parser"
#define UNQ_DTD             "/usr/common/obj/unq_dtd"
#define HEAVY_ARRAY         "/usr/common/obj/heavy_array"


/* User State types */
#define US_ENTER_DATA       "/usr/common/obj/ustate/enter_data"
#define US_OBJ_DESC         "/usr/common/obj/ustate/set_obj_desc"
#define US_SCROLL_TEXT      "/usr/common/obj/ustate/scroll_text"


/* Specific objects (clonable) */
#define THE_VOID            "/usr/System/obj/void"


/* DGD Lightweight objects */
#define LWO_PHRASE          "/usr/common/data/lwo_phrase"
#define LIB_LWO             "/usr/common/data/lib_issue"
#define CLONABLE_LWO        "/usr/common/data/clonable_issue"


/* Storage directories */
#define SYSTEM_USER_DIR     "/data/user"
#define LOGDIR              "/log"


/* Non-LPC configuration files */

/* UNQ DTDs */
#define MAPD_ROOM_DTD       "/data/dtd/room.dtd"
#define MOB_FILE_DTD        "/data/dtd/mobile.dtd"
#define HELP_DTD            "/data/dtd/help.dtd"
#define LOGD_DTD            "/data/dtd/logchannel.dtd"
#define CONFIGD_DTD         "/data/dtd/config.dtd"
#define ZONED_DTD           "/data/dtd/zoned.dtd"
#define BIND_DTD            "/data/dtd/bind.dtd"
#define SOULD_DTD           "/data/dtd/sould.dtd"

/* Other data files */
#define CONFIG_FILE         "/data/system/config.unq"
#define ROOM_FILE           "/data/object/roomfile.unq"
#define MOB_FILE            "/data/object/mobfile.unq"
#define ZONE_FILE           "/data/object/zonefile.unq"
#define SAFE_ROOM_FILE      "/data/object/safe_roomfile.unq"
#define SAFE_MOB_FILE       "/data/object/safe_mobfile.unq"
#define SAFE_ZONE_FILE      "/data/object/safe_zonefile.unq"
#define ROOM_BIND_FILE      "/data/system/room_binder.unq"
#define MOBILE_BIND_FILE    "/data/system/mobile_binder.unq"
#define BUG_DATA            "/data/text/bug_reports.txt"
#define TYPO_DATA           "/data/text/typo_reports.txt"
#define LOG_CHANNELS        "/data/system/logd_channels.unq"
#define SYSTEM_PHRASES      "/data/phrase/system.phr"
#define EXITD_PHRASES       "/data/phrase/exitd.phr"
#define USER_COMMANDS_FILE  "/data/phrase/user_cmds.unq"
#define SOULD_FILE          "/data/system/sould.unq"

#define WELCOME_MESSAGE     "/data/text/welcome.msg"
#define SUSPENDED_MESSAGE   "/data/text/suspended.msg"
#define SHUTDOWN_MESSAGE    "/data/text/shutdown.msg"

# undef SYS_PERSISTENT
# undef SYS_DATAGRAMS	    /* off by default */


# ifdef __SKOTOS__
#  define SYS_NETWORKING	/* Skotos server has networking capabilities */
# endif

# ifdef SYS_NETWORKING
#  define TELNET_PORT	8888	/* default telnet port */
#  define BINARY_PORT	8889	/* default binary port */
# endif
