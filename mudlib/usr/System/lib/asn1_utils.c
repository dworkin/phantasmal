# include "phantasmal/ssh.h"
# include "phantasmal/ssh_asn1.h"

# define DEBUG SSH_DEBUG

inherit SSH_UTILS;

# define CASESTR(var, tag) case tag: var = #tag; break;

/*
 * NAME:        interpret_asn1()
 * DESCRIPTION: describe the contents of what parse_asn1() returns
 */
static string
interpret_asn1(mixed *asn1, varargs int indent)
{
    string spaces, description;

    switch (asn1[ASN_CLASS]) {
    case ASN_UNIVERSAL:
	switch (asn1[ASN_NUMBER]) {
	CASESTR(description, ASN_BOOLEAN);
	CASESTR(description, ASN_INTEGER);
	CASESTR(description, ASN_BIT_STR);
	CASESTR(description, ASN_OCTET_STR);
	CASESTR(description, ASN_NULL);
	CASESTR(description, ASN_OBJECT_ID);
	CASESTR(description, ASN_OBJECT_DESC);
	CASESTR(description, ASN_EXTERNAL);
	CASESTR(description, ASN_REAL);
	CASESTR(description, ASN_ENUMERATED);
	CASESTR(description, ASN_ENCRYPTED);
	CASESTR(description, ASN_UTF8_STR);
	CASESTR(description, ASN_RELATIVE_OID);
	CASESTR(description, ASN_SEQUENCE);
	CASESTR(description, ASN_SET);
	CASESTR(description, ASN_NUMERIC_STR);
	CASESTR(description, ASN_PRINT_STR);
	CASESTR(description, ASN_T61_STR);
	CASESTR(description, ASN_VIDEOTEX_STR);
	CASESTR(description, ASN_IA5_STR);
	CASESTR(description, ASN_UTC_TIME);
	CASESTR(description, ASN_GENERAL_TIME);
	CASESTR(description, ASN_GRAPHIC_STR);
	CASESTR(description, ASN_VISIBLE_STR);
	CASESTR(description, ASN_GENERAL_STR);
	CASESTR(description, ASN_CHARACTER_STR);
	CASESTR(description, ASN_BMP_STR);
	default:
	    description = "Unknown(" + (string)asn1[ASN_NUMBER] + ")";
	}
	break;
    case ASN_APPLICATION:
	description = "(ASN_APPLICATON " + asn1[ASN_NUMBER] + ")";
	break;
    case ASN_CONTEXT:
	description = "[" + asn1[ASN_NUMBER] + "]";
	break;
    case ASN_PRIVATE:
	description = "(ASN_PRIVATE " + asn1[ASN_NUMBER] + ")";
	break;
    default:
	error("Internal error");
    }

    spaces = "                                        "[..indent - 1];
    if (asn1[ASN_TYPE] == ASN_CONSTRUCTOR) {
	int i, sz;
	string *str;

	sz = sizeof(asn1[ASN_CONTENTS]);
	str = allocate(sz);
	for (i = 0; i < sz; i++) {
	    str[i] = interpret_asn1(asn1[ASN_CONTENTS][i], indent + 4);
	}
	return
	    spaces + description + " {\n" +
	    implode(str, "") +
	    spaces + "}\n";
    } else {
	if (asn1[ASN_CLASS] == ASN_UNIVERSAL) {
	    switch (asn1[ASN_NUMBER]) {
	    case ASN_BOOLEAN:
		return
		    spaces + description + " " + (asn1[ASN_CONTENTS][0] ? "TRUE" : "FALSE") + "\n";
	    case ASN_NUMERIC_STR:
	    case ASN_PRINT_STR:
	    case ASN_IA5_STR:
	    case ASN_VISIBLE_STR:
		return
		    spaces + description + " \"" + asn1[ASN_CONTENTS] + "\"\n";
	    default:
		break;
	    }
	}
	return
	    spaces + description + " " + hexdump(asn1[ASN_CONTENTS]) + "\n";
    }
}

/*
 * NAME:        parse_asn1()
 * DESCRIPTION: given input data return structured data
 */
static mixed *parse_asn1(string data, int offset)
{
    int len;
    mixed *result;
    int class;
    int type;
    int number;
    int length;

    len = strlen(data);

    class  = data[offset] & ASN_CLASS_MASK;
    type   = data[offset] & ASN_TYPE_MASK;
    number = data[offset] & ASN_NUMBER_MASK;

    /*
     * Check for an extended tag number.
     */
    if (number == ASN_EXTENSION_ID) {
	number = 0;
	while (offset < len && data[offset] & ASN_EXTENSION_BIT) {
	    number = (number << 7) | (data[offset] & ~ASN_EXTENSION_BIT);
	    offset++;
	}
	if (data[offset] & ASN_EXTENSION_BIT) {
	    /* Invalid data. */
	    DEBUG(1, "ASN.1 data ended prematurely during extended tag number parsing.");
	    return nil;
	}
	number = (number << 7) | data[offset];
    }
    offset++;
    if (offset == len) {
	/* Invalid data. */
	DEBUG(1, "ASN.1 data ended prematurely, no octets for length available.");
	return nil;
    }
    length = data[offset++];
    if (length == ASN_LONG_LEN) {
	/*
	 * Unknown length, need to parse sub-elements until we find a
	 * 0x00 0x00 marker.
	 */
	mixed *asn1_contents;

	asn1_contents = ({ });
	while (offset < len - 1 && data[offset] && data[offset + 1]) {
	    mixed *sub_asn1;

	    sub_asn1 = parse_asn1(data, offset);
	    if (!sub_asn1) {
		/* Invalid data. */
		return nil;
	    }
	    asn1_contents += ({ sub_asn1[0] });
	    offset = sub_asn1[1];
	}
	if (offset < len - 1 && !data[offset] && !data[offset + 1]) {
	    return ({ ({ class, type, number, asn1_contents }), offset + 2 });
	}
	DEBUG(1, "ASN.1 data ended prematurely, no EOC octets found.");
	/* Invalid data. */
	return nil;
    } else if (length & ASN_LONG_LEN) {
	/* Extended length. */
	int ext_length;

	ext_length = 0;
	length = length & ~ASN_LONG_LEN;
	while (offset < len && length > 0) {
	    if (ext_length & 0xff000000) {
		/* We're about to lose information, this can't be good. */
		DEBUG(1, "ASN.1 extended length exceeded integer size.");
		return nil;
	    }
	    ext_length = (ext_length << 8) | data[offset];
	    offset++;
	    length--;
	}
	if (length > 0) {
	    /* Invalid data. */
	    DEBUG(1, "ASN.1 data ended prematurely, extended length indicated more octets.");
	    return nil;
	}
	length = ext_length;
    }

    if (offset + length > len) {
	DEBUG(1, "ASN.1 length octets exceed available data");
	return nil;
    }
    /*
     * Now, are we dealing with a primitive type or a constructed type?
     */
    if (type == ASN_PRIMITIVE) {
	return ({ ({ class, type, number, data[offset .. offset + length - 1] }), offset + length });
    } else {
	int sub_offset, sub_len;
	string sub_data;
	mixed *asn1_contents;

	sub_data = data[offset .. offset + length - 1];
	sub_offset = 0;
	sub_len = length;
	asn1_contents = ({ });
	while (sub_offset < sub_len) {
	    mixed *sub_asn1;

	    sub_asn1 = parse_asn1(sub_data, sub_offset);
	    if (!sub_asn1) {
		/* Invalid data. */
		return nil;
	    }
	    asn1_contents += ({ sub_asn1[0] });
	    sub_offset = sub_asn1[1];
	}
	if (sub_offset != sub_len) {
	    /* Invalid data. */
	    return nil;
	}
	return ({ ({ class, type, number, asn1_contents }), offset + length });
    }
}
