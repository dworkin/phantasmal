# define SSH_DEBUG_LEVEL 2

# include <kernel/kernel.h>
# include <kernel/user.h>

# define SSH_DEBUG(level, mesg) ((level) <= SSH_DEBUG_LEVEL ? DRIVER->message("SSH:debug" + (level) + ": " + (mesg) + "\n") : 0)

# define SSH_GLUE			SSH_KERNEL_GLUE
# define SSH_GLUE_CALL			(previous_program() == LIB_CONN)
# define SSH_GLUE_RLIMITS(r, f, a)	r = call_limited(#f, a)

# define SSH_KERNEL_GLUE	"/usr/System/lib/ssh_kernel"
# define SSHD			"/usr/System/sys/sshd"
# define SSH_UTILS		"/usr/System/lib/ssh_utils"
# define SSH_TRANSPORT		"/usr/System/lib/ssh"
# define SSH_CONNECTION		"/usr/System/obj/ssh_connection"
# define SSH_USER		"/usr/System/obj/kernel_user"
# define SSH_WIZTOOL		"/usr/System/obj/kernel_wiztool"
# define SSH_USERD		"/usr/System/sys/kernel_telnetd"

/*****************************/

# define SSH_MSG_DISCONNECT			1
# define SSH_MSG_IGNORE				2
# define SSH_MSG_UNIMPLEMENTED			3
# define SSH_MSG_DEBUG				4
# define SSH_MSG_SERVICE_REQUEST		5
# define SSH_MSG_SERVICE_ACCEPT			6
# define SSH_MSG_KEXINIT			20
# define SSH_MSG_NEWKEYS			21
# define SSH_MSG_KEXDH_INIT			30
# define SSH_MSG_KEXDH_REPLY			31
# define SSH_MSG_USERAUTH_REQUEST		50
# define SSH_MSG_USERAUTH_FAILURE		51
# define SSH_MSG_USERAUTH_SUCCESS		52
# define SSH_MSG_USERAUTH_BANNER		53
# define SSH_MSG_USERAUTH_PK_OK			60
# define SSH_MSG_GLOBAL_REQUEST			80
# define SSH_MSG_REQUEST_SUCCESS		81
# define SSH_MSG_REQUEST_FAILURE		82
# define SSH_MSG_CHANNEL_OPEN			90
# define SSH_MSG_CHANNEL_OPEN_CONFIRMATION	91
# define SSH_MSG_CHANNEL_OPEN_FAILURE		92
# define SSH_MSG_CHANNEL_WINDOW_ADJUST		93
# define SSH_MSG_CHANNEL_DATA			94
# define SSH_MSG_CHANNEL_EXTENDED_DATA		95
# define SSH_MSG_CHANNEL_EOF			96
# define SSH_MSG_CHANNEL_CLOSE			97
# define SSH_MSG_CHANNEL_REQUEST		98
# define SSH_MSG_CHANNEL_SUCCESS		99
# define SSH_MSG_CHANNEL_FAILURE		100

# define SSH_DISCONNECT_HOST_NOT_ALLOWED_TO_CONNECT	 1
# define SSH_DISCONNECT_PROTOCOL_ERROR			 2
# define SSH_DISCONNECT_KEY_EXCHANGE_FAILED		 3
# define SSH_DISCONNECT_RESERVED			 4
# define SSH_DISCONNECT_MAC_ERROR			 5
# define SSH_DISCONNECT_COMPRESSION_ERROR		 6
# define SSH_DISCONNECT_SERVICE_NOT_AVAILABLE		 7
# define SSH_DISCONNECT_PROTOCOL_VERSION_NOT_SUPPORTED	 8
# define SSH_DISCONNECT_HOST_KEY_NOT_VERIFIABLE		 9
# define SSH_DISCONNECT_CONNECTON_LOST			10
# define SSH_DISCONNECT_BY_APPLICATION			11
# define SSH_DISCONNECT_TOO_MANY_CONNECTONS		12
# define SSH_DISCONNECT_AUTH_CANCELED_BY_USER		13
# define SSH_DISCONNECT_NO_MORE_AUTH_METHODS_AVAILABLE	14
# define SSH_DISCONNECT_ILLEGAL_USER_NAME		15

# define SSH_OPEN_ADMINISTRATIVELY_PROHIBITED	1
# define SSH_OPEN_CONNECT_FAILED		2
# define SSH_OPEN_UNKNOWN_CHANNEL_TYPE		3
# define SSH_OPEN_RESOURCE_SHORTAGE		4
