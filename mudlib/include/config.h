#define USR                 "/usr"
#define SECOND_AUTO_HEADER  "/usr/System/open/include/auto.h"
#define SECOND_AUTO         "/usr/System/open/lib/auto.c"


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


/* Kernel lib tie-in objects -- must be in these dirs to be recognized
   as privileged */
#define INITD               "/usr/System/initd"
#define SYSTEM_USER         "/usr/System/obj/user"
#define SYSTEM_WIZTOOL      "/usr/System/obj/wiztool"


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
#define EXIT                "/usr/common/lib/exit"
#define MOBILE              "/usr/common/lib/mobile"

/* Instantiable (clonable) MUD structures */
#define ACCOUNT             "/usr/common/obj/account"
#define SIMPLE_PHRASE       "/usr/common/obj/simple_phrase"
#define SIMPLE_EXIT         "/usr/common/obj/simple_exit"
#define USER_MOBILE         "/usr/common/obj/user_mobile"
#define SIMPLE_ROOM         "/usr/common/obj/simple_room"
#define UNQ_PARSER          "/usr/common/obj/basic_unq_parser"
#define UNQ_DTD             "/usr/common/obj/unq_dtd"
#define HEAVY_ARRAY         "/usr/common/obj/heavy_array"


/* User State types */
#define US_ENTER_DATA       "/usr/common/obj/ustate/enter_data"
#define US_ENTER_YN         "/usr/common/obj/ustate/enter_yn"
#define US_OBJ_DESC         "/usr/common/obj/ustate/set_obj_desc"
#define US_SCROLL_TEXT      "/usr/common/obj/ustate/scroll_text"
#define US_MAKE_ROOM        "/usr/common/obj/ustate/makeroom"


/* Specific objects (clonable) */
#define THE_VOID            "/usr/System/obj/void"


/* DGD Lightweight objects */
#define LWO_PHRASE          "/usr/common/data/lwo_phrase"
#define LIB_LWO             "/usr/common/data/lib_issue"
#define CLONABLE_LWO        "/usr/common/data/clonable_issue"


/* Storage directories */
#define SYSTEM_USER_DIR     "/usr/System/users"
#define LOGDIR              "/log"


/* Non-LPC configuration files */

/* UNQ DTDs */
#define MAPD_ROOM_DTD       "/usr/common/sys/room.dtd"
#define MOB_FILE_DTD        "/usr/common/sys/mobile.dtd"
#define CONFIGD_DTD         "/usr/common/sys/config.dtd"
#define ZONED_DTD           "/usr/common/sys/zoned.dtd"
#define BIND_DTD            "/usr/common/sys/bind.dtd"
#define SOULD_DTD           "/usr/common/sys/sould.dtd"

/* Other data files */
#define CONFIG_FILE         "/data/system/config.unq"
#define ROOM_DIR            "/data/object/stuff"
#define MOB_FILE            "/data/object/mobfile.unq"
#define ZONE_FILE           "/data/object/zonefile.unq"
#define SAFE_ROOM_DIR       "/data/object/safe_stuff"
#define SAFE_MOB_FILE       "/data/object/safe_mobfile.unq"
#define SAFE_ZONE_FILE      "/data/object/safe_zonefile.unq"
#define ROOM_BIND_FILE      "/usr/common/sys/room_binder.unq"
#define MOBILE_BIND_FILE    "/usr/common/sys/mobile_binder.unq"
#define BUG_DATA            "/usr/game/text/bug_reports.txt"
#define IDEA_DATA           "/usr/game/text/idea_reports.txt"
#define TYPO_DATA           "/usr/game/text/typo_reports.txt"
#define LOG_CHANNELS        "/data/system/logd_channels.unq"
#define SYSTEM_PHRASES      "/usr/System/obj/system.phr"
#define EXITD_PHRASES       "/usr/common/sys/exitd.phr"
#define USER_COMMANDS_FILE  "/data/phrase/user_cmds.unq"
#define SOULD_FILE          "/data/system/sould.unq"
#define NL_PARSE_FILE       "/usr/common/sys/nl_parser.dpd"

#define WELCOME_MESSAGE     "/usr/game/text/welcome.msg"
#define SUSPENDED_MESSAGE   "/usr/game/text/suspended.msg"
#define SHUTDOWN_MESSAGE    "/usr/game/text/shutdown.msg"


/* Random inherited stuff */
#define SYSTEM_WIZTOOLLIB      "/usr/System/lib/wiztoollib"
#define SYSTEM_ROOMWIZTOOLLIB  "/usr/System/lib/room_wiztool"
#define SYSTEM_OBJWIZTOOLLIB   "/usr/System/lib/obj_wiztool"
#define SYSTEM_COMMANDSETLIB   "/usr/System/lib/commandsetlib"
#define SYSTEM_USER_IO         "/usr/System/lib/user_io"


# define COMMON()	(sscanf(previous_program(), USR + "/common/%*s") != 0)


# define SYS_PERSISTENT
# undef SYS_DATAGRAMS	    /* off by default */


# ifdef __SKOTOS__
#  define SYS_NETWORKING	/* Skotos server has networking capabilities */
# endif

# ifdef SYS_NETWORKING
#  define TELNET_PORT	8888	/* default telnet port */
#  define BINARY_PORT	8889	/* default binary port */
# endif

#define CALLOUTRSRC  FALSE  /* don't have callouts as a resource */
