#define USR                 "/usr"

# define COMMON()	(sscanf(previous_program(), USR + "/common/%*s") != 0)
# define GAME()	        (sscanf(previous_program(), USR + "/game/%*s") != 0)

/* Directory to store user passwords in */
#define SYSTEM_USER_DIR     "/usr/game/users"


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
