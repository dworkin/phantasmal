/* Telnet protocol constants */
#define TP_IAC     255             /* interpret as command: */
#define TP_DONT    254             /* you are not to use option */
#define TP_DO      253             /* please, you use option */
#define TP_WONT    252             /* I won't use option */
#define TP_WILL    251             /* I will use option */
#define TP_SB      250             /* interpret as subnegotiation */
#define TP_GA      249             /* you may reverse the line */
#define TP_EL      248             /* erase the current line */
#define TP_EC      247             /* erase the current character */
#define TP_AYT     246             /* are you there */
#define TP_AO      245             /* abort output--but let prog finish */
#define TP_IP      244             /* interrupt process--permanently */
#define TP_BREAK   243             /* break */
#define TP_DM      242             /* data mark--for connect. cleaning */
#define TP_NOP     241             /* nop */
#define TP_SE      240             /* end sub negotiation */
#define TP_EOR     239             /* end of record (transparent mode) */
#define TP_ABORT   238             /* Abort process */
#define TP_SUSP    237             /* Suspend process */
#define TP_EOF     236             /* End of file */
#define TP_SYNCH   242             /* For telfunc calls */

#define TELOPT_ECHO     1       /* echo */
#define TELOPT_RCP      2       /* prepare to reconnect */
#define TELOPT_SGA      3       /* suppress go ahead */
#define TELOPT_STATUS   5       /* give status */
#define TELOPT_TM       6       /* timing mark */
#define TELOPT_NAOL     8       /* negotiate about output line width */
#define TELOPT_NAOP     9       /* negotiate about output page size */
#define TELOPT_NAOCRD   10      /* negotiate about CR disposition */
#define TELOPT_NAOHTS   11      /* negotiate about horizontal tabstops */
#define TELOPT_NAOHTD   12      /* negotiate about horizontal tab disposition */
#define TELOPT_NAOFFD   13      /* negotiate about formfeed disposition */
#define TELOPT_NAOVTS   14      /* negotiate about vertical tab stops */
#define TELOPT_NAOVTD   15      /* negotiate about vertical tab disposition */
#define TELOPT_NAOLFD   16      /* negotiate about output LF disposition */
#define TELOPT_LOGOUT   18      /* force logout */
#define TELOPT_TTYPE    24      /* terminal type */
#define TELOPT_EOR      25      /* end or record */
#define TELOPT_NAWS     31      /* window size */
#define TELOPT_TSPEED   32      /* terminal speed */
#define TELOPT_LFLOW    33      /* remote flow control */
#define TELOPT_LINEMODE 34      /* Linemode option */


/* TELOPT_LINEMODE options */
#define	LM_MODE		1

#define	MODE_EDIT	0x01
