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
#define PORTABLED   "/usr/common/sys/portabled"
#define OBJNUMD     "/usr/common/sys/objnumd"
#define CHANNELD    "/usr/common/sys/channeld"


/* Kernel lib tie-in objects -- must be in these dirs to be recognized
   as privileged */
#define INITD               "/usr/System/initd"
#define SYSTEM_USER         "/usr/System/obj/user"
#define SYSTEM_WIZTOOL      "/usr/System/obj/wiztool"
#define SYSTEM_WIZTOOLLIB   "/usr/System/lib/wiztoollib"


/* Libraries -- inheritable, not clonable */
#define USER_STATE          "/usr/common/lib/user_state"
#define UNQABLE             "/usr/common/lib/unqable"
#define PHRASE_REPOSITORY   "/usr/common/lib/phrase_repository"
#define PHRASE              "/usr/common/lib/phrase"
#define INTL_PHRASE         "/usr/common/lib/intl_phrase"
#define OBJECT              "/usr/common/lib/object"
#define CONTAINER           "/usr/common/lib/container"
#define ROOM                "/usr/common/lib/room"
#define EXIT                "/usr/common/lib/exit"
#define MOBILE              "/usr/common/lib/mobile"
#define PORTABLE            "/usr/common/lib/portable"
#define ISSUE_LWO           "/usr/common/lib/issue_lwo"


/* Instantiable (clonable) MUD structures */
#define ACCOUNT             "/usr/common/obj/account"
#define SIMPLE_PHRASE       "/usr/common/obj/simple_phrase"
#define SIMPLE_ROOM         "/usr/common/obj/simple_room"
#define SIMPLE_EXIT         "/usr/common/obj/simple_exit"
#define SIMPLE_MOBILE       "/usr/common/obj/simple_mobile"
#define SIMPLE_PORTABLE     "/usr/common/obj/simple_portable"
#define UNQ_PARSER          "/usr/common/obj/basic_unq_parser"
#define UNQ_DTD             "/usr/common/obj/unq_dtd"
#define HEAVY_ARRAY         "/usr/common/obj/heavy_array"
#define US_ENTER_DATA       "/usr/common/obj/ustate/enter_data"
#define US_OBJ_DESC         "/usr/common/obj/ustate/set_obj_desc"


/* Specific objects (clonable) */
#define THE_VOID            "/usr/System/obj/void"
#define PLAYERBODY          "/usr/common/obj/playerbody"


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
#define PORTABLE_DTD        "/data/dtd/portable.dtd"
#define HELP_DTD            "/data/dtd/help.dtd"
#define LOGD_DTD            "/data/dtd/logchannel.dtd"

/* Other data files */
#define ROOM_FILE           "/data/object/roomfile.unq"
#define PORT_FILE           "/data/object/portablefile.unq"
#define BUG_DATA            "/data/text/bug_reports.txt"
#define TYPO_DATA           "/data/text/typo_reports.txt"
#define LOG_CHANNELS        "/data/system/logd_channels.unq"
#define SYSTEM_PHRASES      "/data/phrase/system.phr"
#define EXITD_PHRASES       "/data/phrase/exitd.phr"
#define USER_COMMANDS_FILE  "/data/phrase/user_cmds.unq"

# undef SYS_PERSISTENT
# undef SYS_DATAGRAMS	    /* off by default */


# ifdef __SKOTOS__
#  define SYS_NETWORKING	/* Skotos server has networking capabilities */
# endif

# ifdef SYS_NETWORKING
#  define TELNET_PORT	8888	/* default telnet port */
#  define BINARY_PORT	8889	/* default binary port */
# endif
