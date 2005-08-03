/* There are a whole bunch of protocols we don't (yet?) support, but
   we'd like to be able to. */
#define PROTOCOL_MCP                1
#define PROTOCOL_IMP                2   /* FireClient */
#define PROTOCOL_PUEBLO             4
#define PROTOCOL_MXP                8
#define PROTOCOL_MSP               16
#define PROTOCOL_ZMP               32
#define PROTOCOL_MCCP              64
#define PROTOCOL_EXT_TELNET       128

#define TELOPT_MCCP_OLD            85  /* COMPRESS option, obsolete */
#define TELOPT_MCCP                86  /* COMPRESS2 option for MCCP */
#define TELOPT_MSP                 90
#define TELOPT_MXP                 91
#define TELOPT_ZMP                 93  /* AweMUD's Zenith protocol */
