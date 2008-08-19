# include "phantasmal/ssh.h"
# include "phantasmal/ssh_asn1.h"

# define BASE64 ("...........................................\x3e..." +   \
		 "\x3f\x34\x35\x36\x37\x38\x39\x3a\x3b\x3c\x3d...=..." +  \
		 "\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c" + \
		 "\x0d\x0e\x0f\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19" + \
		 "......" +                                               \
		 "\x1a\x1b\x1c\x1d\x1e\x1f\x20\x21\x22\x23\x24\x25\x26" + \
		 "\x27\x28\x29\x2a\x2b\x2c\x2d\x2e\x2f\x30\x31\x32\x33" + \
		 "...................................................." + \
		 "...................................................." + \
		 ".............................")

/*
 * NAME:	base64_decode()
 * DESCRIPTION:	decode a base64 string
 */
static string base64_decode(string str)
{
    string result, bits;
    int i, len, b1, b2, b3, b4;

    result = "";
    bits = "...";
    for (i = 0, len = strlen(str); i < len; i += 4) {
	b1 = BASE64[str[i]];
	b2 = BASE64[str[i + 1]];
	b3 = BASE64[str[i + 2]];
	b4 = BASE64[str[i + 3]];
	bits[0] = (b1 << 2) | (b2 >> 4);
	bits[1] = (b2 << 4) | (b3 >> 2);
	bits[2] = (b3 << 6) | b4;
	result += bits;
    }

    if (b3 == '=') {
	return result[.. strlen(result) - 3];
    } else if (b4 == '=') {
	return result[.. strlen(result) - 2];
    }
    return result;
}

/*
 * NAME:        parse_public_key()
 * DESCRIPTION: check for a valid (DSA) public key.
 */
static string parse_public_key(string str)
{
    if (sscanf(str, "ssh-dss %s ", str) == 0 &&
	sscanf(str, "ssh-rsa %s ", str) == 0) {
	return nil;
    }
    return base64_decode(str);
}

/*
 * NAME:        parse_private_key()
 * DESCRIPTION: check for a valid (DSA) private key.
 */
static string parse_private_key(string str)
{
    if (sscanf(str, "-----BEGIN DSA PRIVATE KEY-----%s" +
	            "-----END DSA PRIVATE KEY-----", str) == 0) {
	return nil;
    }
    str = implode(explode(str, "\r"), "");
    sscanf(str, "%*s\n\n%s", str);	/* skip possible comments */
    return base64_decode(implode(explode(str, "\n"), ""));
}

/*
 * NAME:        hexdump()
 * DESCRIPTION: dump the string in hexadecimal form.
 */
static string hexdump(string str)
{
    int i, len;
    string result;

    result = str + str + str;
    len = strlen(str);
    if (!len) {
	return "";
    }
    for (i = 0; i < len; i++) {
	int ch;

	result[i * 3 + 0] = ':';
	ch = str[i];
	switch (ch >> 4) {
	case  0.. 9:
	    result[i * 3 + 1] = '0' + (ch >> 4);
	    break;
	case 10..15:
	    result[i * 3 + 1] = 'A' + (ch >> 4) - 10;
	    break;
	}
	switch (ch & 0x0f) {
	case  0.. 9:
	    result[i * 3 + 2] = '0' + (ch & 0x0f);
	    break;
	case 10..15:
	    result[i * 3 + 2] = 'A' + (ch & 0x0f) - 10;
	    break;
	}
    }
    return result[1..];
}

/*
 * NAME:	random_string()
 * DESCRIPTION:	create a string of pseudo-random bytes
 */
static string random_string(int length)
{
    string str;
    int n, rand;

    str = "................................";
    while (strlen(str) < length) {
	str += str;
    }
    str = str[.. length - 1];
    for (n = length - length % 3; n != 0; ) {
	/* create three random bytes at a time */
	rand = random(0x1000000);
	str[--n] = rand >> 16;
	str[--n] = rand >> 8;
	str[--n] = rand;
    }

    switch (length % 3) {
    case 1:
	str[length - 1] = random(0x100);
	break;

    case 2:
	rand = random(0x10000);
	str[length - 2] = rand >> 8;
	str[length - 1] = rand;
	break;
    }

    return str;
}

/*
 * NAME:	better_random_string()
 * DESCRIPTION:	create a slightly more random string
 */
static string better_random_string(int length)
{
    string str;

    str = "";
    while (length >= 20) {
	str += hash_sha1(random_string(20));
	length -= 20;
    }
    if (length >= 0) {
	str += hash_sha1(random_string(length))[.. length - 1];
    }

    return str;
}

/*
 * NAME:	make_int()
 * DESCRIPTION:	build a SSH int
 */
static string make_int(int i)
{
    string str;

    str = "....";
    str[0] = i >> 24;
    str[1] = i >> 16;
    str[2] = i >> 8;
    str[3] = i;

    return str;
}

/*
 * NAME:	make_string()
 * DESCRIPTION:	build a SSH string
 */
static string make_string(string str)
{
    string header;
    int length;

    length = strlen(str);
    header = "\0\0..";
    header[2] = length >> 8;
    header[3] = length;

    return header + str;
}

/*
 * NAME:	make_mpint()
 * DESCRIPTION:	build a multi-precision integer
 */
static string make_mpint(string str)
{
    string header;
    int length;

    length = strlen(str);
    header = "\0\0..";
    header[2] = length >> 8;
    header[3] = length;

    return header + str;
}

/*
 * NAME:	make_mesg()
 * DESCRIPTION:	create a message code
 */
static string make_mesg(int code)
{
    string str;

    str = ".";
    str[0] = code;
    return str;
}

/*
 * NAME:	get_int()
 * DESCRIPTION:	get an int from a buffer
 */
static int get_int(string b, int i)
{
    return (b[i] << 24) + (b[i + 1] << 16) + (b[i + 2] << 8) + b[i + 3];
}

/*
 * NAME:	get_string()
 * DESCRIPTION:	get a string from a buffer
 */
static string get_string(string b, int i)
{
    return b[i + 4 .. i + (b[i] << 24) + (b[i + 1] << 16) + (b[i + 2] << 8) +
		      b[i + 3] + 3];
}

/*
 * NAME:        get_mpint()
 * DESCRIPTION: get a multi-precision integer from a buffer
 */
static string get_mpint(string b, int i)
{
    int len;

    len = (b[i] << 24) + (b[i + 1] << 16) + (b[i + 2] << 8) + b[i + 3];
    return b[i + 4 .. i + 3 + len];
}

#define STRCASE(foo) case foo: return #foo

/*
 * NAME:        dump_packet_type()
 * DESCRIPTION: show the packet type in symbolic form
 */
static string dump_ssh_msg_type(int type)
{
    switch (type) {
    STRCASE(SSH_MSG_DISCONNECT);
    STRCASE(SSH_MSG_IGNORE);
    STRCASE(SSH_MSG_UNIMPLEMENTED);
    STRCASE(SSH_MSG_DEBUG);
    STRCASE(SSH_MSG_SERVICE_REQUEST);
    STRCASE(SSH_MSG_SERVICE_ACCEPT);
    STRCASE(SSH_MSG_KEXINIT);
    STRCASE(SSH_MSG_NEWKEYS);
    STRCASE(SSH_MSG_KEXDH_INIT);
    STRCASE(SSH_MSG_KEXDH_REPLY);
    STRCASE(SSH_MSG_USERAUTH_REQUEST);
    STRCASE(SSH_MSG_USERAUTH_FAILURE);
    STRCASE(SSH_MSG_USERAUTH_SUCCESS);
    STRCASE(SSH_MSG_USERAUTH_BANNER);
    STRCASE(SSH_MSG_USERAUTH_PK_OK);
    STRCASE(SSH_MSG_GLOBAL_REQUEST);
    STRCASE(SSH_MSG_REQUEST_SUCCESS);
    STRCASE(SSH_MSG_REQUEST_FAILURE);
    STRCASE(SSH_MSG_CHANNEL_OPEN);
    STRCASE(SSH_MSG_CHANNEL_OPEN_CONFIRMATION);
    STRCASE(SSH_MSG_CHANNEL_OPEN_FAILURE);
    STRCASE(SSH_MSG_CHANNEL_WINDOW_ADJUST);
    STRCASE(SSH_MSG_CHANNEL_DATA);
    STRCASE(SSH_MSG_CHANNEL_EXTENDED_DATA);
    STRCASE(SSH_MSG_CHANNEL_EOF);
    STRCASE(SSH_MSG_CHANNEL_CLOSE);
    STRCASE(SSH_MSG_CHANNEL_REQUEST);
    STRCASE(SSH_MSG_CHANNEL_SUCCESS);
    STRCASE(SSH_MSG_CHANNEL_FAILURE);
    default:
        return "Unknown(" + type + ")"; 
    }
}

static string dump_ssh_disconnect_type(int type)
{
    switch (type) {
    STRCASE(SSH_DISCONNECT_HOST_NOT_ALLOWED_TO_CONNECT);
    STRCASE(SSH_DISCONNECT_PROTOCOL_ERROR);
    STRCASE(SSH_DISCONNECT_KEY_EXCHANGE_FAILED);
    STRCASE(SSH_DISCONNECT_RESERVED);
    STRCASE(SSH_DISCONNECT_MAC_ERROR);
    STRCASE(SSH_DISCONNECT_COMPRESSION_ERROR);
    STRCASE(SSH_DISCONNECT_SERVICE_NOT_AVAILABLE);
    STRCASE(SSH_DISCONNECT_PROTOCOL_VERSION_NOT_SUPPORTED);
    STRCASE(SSH_DISCONNECT_HOST_KEY_NOT_VERIFIABLE);
    STRCASE(SSH_DISCONNECT_CONNECTON_LOST);
    STRCASE(SSH_DISCONNECT_BY_APPLICATION);
    STRCASE(SSH_DISCONNECT_TOO_MANY_CONNECTONS);
    STRCASE(SSH_DISCONNECT_AUTH_CANCELED_BY_USER);
    STRCASE(SSH_DISCONNECT_NO_MORE_AUTH_METHODS_AVAILABLE);
    STRCASE(SSH_DISCONNECT_ILLEGAL_USER_NAME);
    default:
	return "Unknown (" + type + ")";
    }
}

static string dump_ssh_open_type(int type)
{
    switch (type) {
    STRCASE(SSH_OPEN_ADMINISTRATIVELY_PROHIBITED);
    STRCASE(SSH_OPEN_CONNECT_FAILED);
    STRCASE(SSH_OPEN_UNKNOWN_CHANNEL_TYPE);
    STRCASE(SSH_OPEN_RESOURCE_SHORTAGE);
    default:
	return "Unknown (" + type + ")";
    }
}
