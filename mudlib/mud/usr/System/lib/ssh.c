# include "phantasmal/ssh.h"
# include "phantasmal/ssh_asn1.h"

# define DEBUG SSH_DEBUG

inherit glue SSH_GLUE;
private inherit ASN1_UTILS;
private inherit SSH_UTILS;


private int receive_packet(string str);
static int userauth(string str);	/* supplied by connection layer */


/* ========================================================================= *
 *			    Section 1: packet layer			     *
 * ========================================================================= */

private string obuffer;			/* send buffer */
private int cts;			/* clear to send? */
private string ibuffer;			/* receive buffer */
private string header;			/* first 8 characters of packet */
private int length;			/* length of packet to receive */
private int recv_seqno, send_seqno;	/* send and receive sequence numbers */
private string dkey1, dkey2, dkey3;	/* decryption keys */
private string ekey1, ekey2, ekey3;	/* encryption keys */
private string dstate, estate;		/* en/decryption state */
private string client_mac;		/* client MAC key */
private string server_mac;		/* server MAC key */

/*
 * NAME:	make_packet()
 * DESCRIPTION:	build a packet (without MAC)
 */
private string make_packet(string str)
{
    int length, padding;

    /* minimum padding is 4 bytes, round up to multiple of 8 bytes */
    length = strlen(str);
    padding = 12 - (length + 1) % 8;
    length += padding + 1;

    str = "\0\0..." + str + random_string(padding);
    str[2] = length >> 8;
    str[3] = length;
    str[4] = padding;

    return str;
}

/*
 * NAME:	encrypt_packet()
 * DESCRIPTION:	encrypt a packet
 */
private string encrypt_packet(string str)
{
    int i, n, length;
    string *encrypted;

    length = strlen(str);
    encrypted = allocate(length / 8);
    for (i = n = 0; i < length; i += 8, n++) {
	estate = encrypt("DES", ekey3,
			 decrypt("DES", ekey2,
				 encrypt("DES", ekey1,
					 asn_xor(str[i .. i + 7], estate)))),
	encrypted[n] = estate;
    }
    return implode(encrypted, "");
}

/*
 * NAME:	decrypt_string()
 * DESCRIPTION:	decrypt a string
 */
private string decrypt_string(string str)
{
    int i, n, length;
    string chunk, *decrypted;

    length = strlen(str);
    decrypted = allocate(length / 8);
    for (i = n = 0; i < length; i += 8, n++) {
	chunk = str[i .. i + 7];
	decrypted[n] = asn_xor(decrypt("DES", dkey1,
				       encrypt("DES", dkey2,
					       decrypt("DES", dkey3, chunk))),
			       dstate);
	dstate = chunk;
    }
    return implode(decrypted, "");
}

/*
 * NAME:	hmac()
 * DESCRIPTION:	compute HMAC
 */
private string hmac(string key, string str)
{
    string ipad, opad;

    ipad = "\x36\x36\x36\x36\x36\x36\x36\x36\x36\x36\x36\x36\x36\x36\x36\x36" +
	   "\x36\x36\x36\x36\x36\x36\x36\x36\x36\x36\x36\x36\x36\x36\x36\x36" +
	   "\x36\x36\x36\x36\x36\x36\x36\x36\x36\x36\x36\x36\x36\x36\x36\x36" +
	   "\x36\x36\x36\x36\x36\x36\x36\x36\x36\x36\x36\x36\x36\x36\x36\x36";
    opad = "\x5c\x5c\x5c\x5c\x5c\x5c\x5c\x5c\x5c\x5c\x5c\x5c\x5c\x5c\x5c\x5c" +
	   "\x5c\x5c\x5c\x5c\x5c\x5c\x5c\x5c\x5c\x5c\x5c\x5c\x5c\x5c\x5c\x5c" +
	   "\x5c\x5c\x5c\x5c\x5c\x5c\x5c\x5c\x5c\x5c\x5c\x5c\x5c\x5c\x5c\x5c" +
	   "\x5c\x5c\x5c\x5c\x5c\x5c\x5c\x5c\x5c\x5c\x5c\x5c\x5c\x5c\x5c\x5c";
    return hash_string("SHA1", asn_xor(key, opad), hash_string("SHA1", asn_xor(key, ipad), str));
}

/*
 * NAME:	__send_packet()
 * DESCRIPTION:	send a packet to the other side
 */
private atomic int __send_packet(string str)
{
    DEBUG(2, "__send_packet:  " + dump_ssh_msg_type(str[0]));

    str = make_packet(str);
    if (server_mac) {
	str = encrypt_packet(str) +
	      hmac(server_mac, make_int(send_seqno) + str);
    }
    send_seqno++;

    if (!cts) {
	if (obuffer) {
	    obuffer += str;
	} else {
	    obuffer = str;
	}
	return FALSE;
    }
    return cts = ::message(str);
}

/*
 * NAME:	message_done()
 * DESCRIPTION:	ready for another message
 */
int message_done()
{
    if (SSH_GLUE_CALL) {
	string str;

	if (obuffer) {
	    str = obuffer;
	    obuffer = nil;
	    cts = ::message(str);
	} else {
	    cts = TRUE;
	    return ::message_done();
	}
    }
    return MODE_NOCHANGE;
}

/*
 * NAME:	process_message()
 * DESCRIPTION:	process a message
 */
static int process_message(string str)
{
    if (client_mac) {
	string mac;

	/* decrypt & verify MAC */
	mac = str[length ..];
	str = header + decrypt_string(str[.. length - 1]);
	if (mac != hmac(client_mac, make_int(recv_seqno) + str)) {
	    DEBUG(0, "bad MAC");
	    __send_packet(make_mesg(SSH_MSG_DISCONNECT) +
			  make_int(SSH_DISCONNECT_MAC_ERROR) +
			  make_string("bad MAC") +
			  make_string("en"));
	    return MODE_DISCONNECT;
	}
    } else {
	/* unencrypted */
	str = header + str;
    }
    recv_seqno++;

    str = str[5 .. length + 7 - str[4]];
    length = -1;
    return receive_packet(str);
}

/*
 * NAME:	receive_message()
 * DESCRIPTION:	receive a message
 */
int receive_message(string str)
{
    int mode;

    if (SSH_GLUE_CALL || previous_program() == SSH_TRANSPORT) {
	if (!ibuffer) {
	    ibuffer = str;
	    return MODE_NOCHANGE;
	}

	ibuffer += str;
	while (this_object()) {
	    if (length < 0) {
		/*
		 * new packet
		 */
		if (strlen(ibuffer) < 8) {
		    break;
		}
		header = ibuffer[.. 7];
		ibuffer = ibuffer[8 ..];
		if (client_mac) {
		    header = decrypt_string(header);
		}
		length = get_int(header, 0);
		if (length <= 0 || length > 35000 - 4 || (length & 7) != 4) {
		    DEBUG(0, "bad packet length " + length);
		    catch {
			__send_packet(make_mesg(SSH_MSG_DISCONNECT) +
				      make_int(SSH_DISCONNECT_PROTOCOL_ERROR) +
				      make_string("bad packet length") +
				      make_string("en"));
		    }
		    return MODE_DISCONNECT;
		}
		length -= 4;
		if (client_mac) {
		    length += 20;
		}
	    }

	    if (strlen(ibuffer) < length) {
		break;
	    }

	    /*
	     * full packet received
	     */
	    str = ibuffer[.. length - 1];
	    ibuffer = ibuffer[length ..];
	    if (client_mac) {
		length -= 20;
	    }
	    SSH_GLUE_RLIMITS(mode, process_message, str);
	    if (mode == MODE_DISCONNECT) {
		return MODE_DISCONNECT;
	    }
	    if (mode >= MODE_UNBLOCK) {
		set_mode(mode);
	    }
	}
    }
    return MODE_RAW;
}

/*
 * NAME:	recv_seqno()
 * DESCRIPTION:	return the sequence number of the last message
 */
static int recv_seqno()
{
    return recv_seqno - 1;
}

/*
 * NAME:	create_packet()
 * DESCRIPTION:	initialize packet layer functions
 */
private void create_packet()
{
    cts = TRUE;
    length = -1;
}


/* ========================================================================= *
 *			    Section 2: transport layer			     *
 * ========================================================================= */

# define TRANSPORT_KEXINIT	0
# define TRANSPORT_SKIP		1
# define TRANSPORT_KEXDH	2
# define TRANSPORT_NEWKEYS	3
# define TRANSPORT_TRANSPORT	4


private int state;		/* transport state */
private string *packet_buffer;	/* output packet buffer while rekeying */
private string client_version;	/* client protocol version string */
private string client_kexinit;	/* client KEXINIT string */
private string server_kexinit;	/* server KEXINIT string */
private string p, q;		/* prime and group order */
private string y;		/* intermediate result */
private string f, e;		/* shared secrets */
private string K, H;		/* crypto stuff */
private string session_id;	/* ID for this entire session */

/*
 * NAME:	send_packet()
 * DESCRIPTION:	send a packet, unless rekeying
 */
private int send_packet(string str)
{
    if (packet_buffer) {
	packet_buffer += ({ str });
	return FALSE;
    }  else {
	return __send_packet(str);
    }
}

/*
 * NAME:	flush_packet_buffer()
 * DESCRIPTION:	flush output packet buffer after rekeying
 */
private void flush_packet_buffer()
{
    int i, sz;

    for (i = 0, sz = sizeof(packet_buffer); i < sz; i++) {
	__send_packet(packet_buffer[i]);
    }
    packet_buffer = nil;
}

/*
 * NAME:	ssh_dss_sign()
 * DESCRIPTION:	sign m with the host key
 */
private string ssh_dss_sign(string m, string host_key)
{
    string p, q, g, x;
    string k, r, s;
    mixed *asn1;

    asn1 = parse_asn1(host_key, 0);
    if (!asn1) {
	error("Invalid Host Key");
    }

    /* parse_asn1() returns ({ parsed-data, offset }) */
    asn1 = asn1[0];

    /* Assume it's a sequence of 6 integers. */
    p = asn1[ASN_CONTENTS][1][ASN_CONTENTS];
    q = asn1[ASN_CONTENTS][2][ASN_CONTENTS];
    g = asn1[ASN_CONTENTS][3][ASN_CONTENTS];
    x = asn1[ASN_CONTENTS][5][ASN_CONTENTS];

    /* k = random 0 < k < q */
    do {
	k = asn_mod("\1" + better_random_string(strlen(q)), q);
    } while (strlen(k) < strlen(q) - 1);

    /* r = (g ^ k mod p) mod q */
    r = asn_mod(asn_pow(g, k, p), q);

    /* s = (k ^ -1 * (H(m) + x * r)) mod q */
    s = asn_mult(asn_pow(k, asn_sub(q, "\2", q), q),
		 asn_add("\0" + hash_string("SHA1", m), asn_mult(x, r, q), q),
		 q);

    r = ("\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0" + r)[strlen(r) ..];
    s = ("\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0" + s)[strlen(s) ..];
    return make_string("ssh-dss") + make_string(r + s);
}

/*
 * NAME:	shift_key()
 * DESCRIPTION:	shift out the lowest bit of all characters in a key string
 */
private string shift_key(string key)
{
    int i, len;

    /*
     * Believe it or not, but the openssl crypto suite has the parity for
     * DES setkey in the <lowest> bit.
     */
    for (i = 0, len = strlen(key); i < len; i++) {
	key[i] >>= 1;
    }

    return key;
}

/*
 * NAME:	set_keys()
 * DESCRIPTION:	create keys as negotiated
 */
private void set_keys()
{
    string str, client_key, server_key;

    str = make_string(K) + H;
    dstate = hash_string("SHA1", str, "A", session_id)[.. 7];
    estate = hash_string("SHA1", str, "B", session_id)[.. 7];
    client_key = hash_string("SHA1", str, "C", session_id);
    client_key += hash_string("SHA1", str, client_key);
    server_key = hash_string("SHA1", str, "D", session_id);
    server_key += hash_string("SHA1", str, server_key);
    client_mac = hash_string("SHA1", str, "E", session_id) +
		 "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0" +
		 "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";
    server_mac = hash_string("SHA1", str, "F", session_id) +
		 "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0" +
		 "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";

    dkey1 = decrypt("DES key", shift_key(client_key[.. 7]));
    dkey2 = encrypt("DES key", shift_key(client_key[8 .. 15]));
    dkey3 = decrypt("DES key", shift_key(client_key[16 .. 23]));
    ekey1 = encrypt("DES key", shift_key(server_key[.. 7]));
    ekey2 = decrypt("DES key", shift_key(server_key[8 .. 15]));
    ekey3 = encrypt("DES key", shift_key(server_key[16 .. 23]));
}

/*
 * NAME:	query_session_id()
 * DESCRIPTION:	return the session ID for authentication purposes
 */
static string query_session_id()
{
    return session_id;
}

/*
 * NAME:	start_transport()
 * DESCRIPTION:	start up the transport layer
 */
static void start_transport(string version)
{
    DEBUG(1, "client version is " + version);

    state = TRANSPORT_KEXINIT;
    client_version = version;
    server_kexinit = make_mesg(SSH_MSG_KEXINIT) +
		     better_random_string(16) +
		     make_string("diffie-hellman-group1-sha1") +
		     make_string("ssh-dss") +
		     make_string("3des-cbc") +
		     make_string("3des-cbc") +
		     make_string("hmac-sha1") +
		     make_string("hmac-sha1") +
		     make_string("none") +
		     make_string("none") +
		     make_string("") +
		     make_string("") +
		     "\0" +
		     "\0\0\0\0";
    __send_packet(server_kexinit);
    receive_message("");	/* process message in buffer, if any */
}

/*
 * NAME:	receive_packet()
 * DESCRIPTION:	receive a packet from connection
 */
private int receive_packet(string str)
{
    int offset;

    DEBUG(2, "receive_packet: " + dump_ssh_msg_type(str[0]));

    if (state == TRANSPORT_SKIP) {
	state = TRANSPORT_KEXDH;
	return MODE_NOCHANGE;
    }

    switch (str[0]) {
    case SSH_MSG_DISCONNECT:
	return MODE_DISCONNECT;

    case SSH_MSG_IGNORE:
	break;

    case SSH_MSG_UNIMPLEMENTED:
	DEBUG(1, "received SSH_MSG_UNIMPLEMENTED for packet " + get_int(str, 1));
	break;

    case SSH_MSG_DEBUG:
	DEBUG(0, "received SSH_MSG_DEBUG: " +
	      implode(explode(get_string(str, 2), "\r\n"), "\n"));
	break;

    case SSH_MSG_KEXINIT:
	if (state == TRANSPORT_TRANSPORT) {
	    /*
	     * client wants to negotiate new keys
	     */
	    server_kexinit = make_mesg(SSH_MSG_KEXINIT) +
			     better_random_string(16) +
			     make_string("diffie-hellman-group1-sha1") +
			     make_string("ssh-dss") +
			     make_string("3des-cbc") +
			     make_string("3des-cbc") +
			     make_string("hmac-sha1") +
			     make_string("hmac-sha1") +
			     make_string("none") +
			     make_string("none") +
			     make_string("") +
			     make_string("") +
			     "\0" +
			     "\0\0\0\0";
	    __send_packet(server_kexinit);
	    packet_buffer = ({ });
	} else if (state != TRANSPORT_KEXINIT) {
	    DEBUG(1, "Unexpected SSH_MSG_KEXINIT received");
	    break;
	}

	client_kexinit = str;

	/* generate random y (0 < y < q) */
	do {
	    y = asn_mod("\1" + better_random_string(strlen(q)), q);
	} while (strlen(y) < strlen(q) - 1);

	/* f = g ^ y mod p */
	f = asn_pow("\2", y, p);

	offset = 17;				/* type + random */
	offset += 4 + get_int(str, offset);	/* kex */
	offset += 4 + get_int(str, offset);	/* host key */
	offset += 4 + get_int(str, offset);	/* decrypt */
	offset += 4 + get_int(str, offset);	/* encrypt */
	offset += 4 + get_int(str, offset);	/* demac */
	offset += 4 + get_int(str, offset);	/* mac */
	offset += 4 + get_int(str, offset);	/* decompress */
	offset += 4 + get_int(str, offset);	/* compress */
	offset += 4 + get_int(str, offset);	/* de-lang */
	offset += 4 + get_int(str, offset);	/* lang */
	if (str[offset]) {
	    state = TRANSPORT_SKIP;
	} else {
	    state = TRANSPORT_KEXDH;
	}
	break;

    case SSH_MSG_KEXDH_INIT:
	if (state == TRANSPORT_KEXDH) {
	    e = get_string(str, 1);
	    str = SSHD->query_pub_host_key();

	    /* K = e ^ y mod p */
	    K = asn_pow(e, y, p);

	    /* H = shared secret */
	    H = hash_string("SHA1", make_string(client_version),
			  make_string(SSHD->query_version()),
			  make_string(client_kexinit),
			  make_string(server_kexinit),
			  make_string(str),
			  make_mpint(e),
			  make_mpint(f),
			  make_mpint(K));
	    if (!session_id) {
		session_id = H;
	    }

	    str = make_mesg(SSH_MSG_KEXDH_REPLY) +
		  make_string(str) +
		  make_mpint(f) +
		  make_string(ssh_dss_sign(H, SSHD->query_host_key()));
	    __send_packet(str);
	    state = TRANSPORT_NEWKEYS;
	} else {
	    DEBUG(1, "Unexpected SSH_MSG_KEXDH_INIT received");
	}
	break;

    case SSH_MSG_NEWKEYS:
	if (state == TRANSPORT_NEWKEYS) {
	    __send_packet(make_mesg(SSH_MSG_NEWKEYS));
	    set_keys();
	    state = TRANSPORT_TRANSPORT;

	    if (packet_buffer) {
		flush_packet_buffer();
	    }
	} else {
	    DEBUG(1, "Unexpected SSH_MSG_NEWKEYS received");
	}
	break;

    default:
	if (state == TRANSPORT_TRANSPORT) {
	    return userauth(str);
	} else {
	    DEBUG(1, "Unexpected packet type " + str[0] + " received");
	}
	break;
    }

    return MODE_NOCHANGE;
}

/*
 * NAME:	message()
 * DESCRIPTION:	send a message (well, a packet) to the connection
 */
static int message(string str)
{
    return send_packet(str);
}

/*
 * NAME:	create_transport()
 * DESCRIPTION:	initialize transport layer
 */
static void create_transport()
{
    glue::create_glue();
    create_packet();

    /* p = 2^1024 - 2^960 - 1 + 2^64 * floor( 2^894 Pi + 129093 ) */
    p = "\0" +
	"\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xC9\x0F\xDA\xA2\x21\x68\xC2\x34" +
	"\xC4\xC6\x62\x8B\x80\xDC\x1C\xD1\x29\x02\x4E\x08\x8A\x67\xCC\x74" +
	"\x02\x0B\xBE\xA6\x3B\x13\x9B\x22\x51\x4A\x08\x79\x8E\x34\x04\xDD" +
	"\xEF\x95\x19\xB3\xCD\x3A\x43\x1B\x30\x2B\x0A\x6D\xF2\x5F\x14\x37" +
	"\x4F\xE1\x35\x6D\x6D\x51\xC2\x45\xE4\x85\xB5\x76\x62\x5E\x7E\xC6" +
	"\xF4\x4C\x42\xE9\xA6\x37\xED\x6B\x0B\xFF\x5C\xB6\xF4\x06\xB7\xED" +
	"\xEE\x38\x6B\xFB\x5A\x89\x9F\xA5\xAE\x9F\x24\x11\x7C\x4B\x1F\xE6" +
	"\x49\x28\x66\x51\xEC\xE6\x53\x81\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF";
    /* q = (p - 1) / 2 */
    q = asn_rshift(p, 1);
}
