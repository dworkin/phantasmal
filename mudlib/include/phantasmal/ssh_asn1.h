/*
 * Based on ITU-T Rec X.690 (12/97).
 */

# define ASN1_UTILS "/usr/System/lib/asn1_utils"

/* Constants to define the results of parse_asn1(), */

# define ASN_CLASS	    0
# define ASN_TYPE	    1
# define ASN_NUMBER	    2
# define ASN_CONTENTS	    3

/* Constants used for the ASN.1 identifier octet(s). */

# define ASN_CLASS_MASK     0xC0
# define ASN_TYPE_MASK      0x20
# define ASN_NUMBER_MASK    0x1F

/* What class of data. */

# define ASN_UNIVERSAL      0x00
# define ASN_APPLICATION    0x40
# define ASN_CONTEXT        0x80
# define ASN_PRIVATE        0xC0

/* What type of data. */

# define ASN_PRIMITIVE      0x00
# define ASN_CONSTRUCTOR    0x20

/* What tag number. */

# define ASN_BOOLEAN        0x01
# define ASN_INTEGER        0x02
# define ASN_BIT_STR        0x03
# define ASN_OCTET_STR      0x04
# define ASN_NULL           0x05
# define ASN_OBJECT_ID      0x06
# define ASN_OBJECT_DESC    0x07
# define ASN_EXTERNAL       0x08
# define ASN_REAL           0x09
# define ASN_ENUMERATED     0x0A
# define ASN_ENCRYPTED      0x0B
# define ASN_UTF8_STR       0x0C
# define ASN_RELATIVE_OID   0x0D
/* 0x0E? */
/* 0x0F? */
# define ASN_SEQUENCE       0x10
# define ASN_SET            0x11
# define ASN_NUMERIC_STR    0x12
# define ASN_PRINT_STR      0x13
# define ASN_T61_STR        0x14
# define ASN_TELETEX_STR    ASN_T61_STR
# define ASN_VIDEOTEX_STR   0x15
# define ASN_IA5_STR        0x16
# define ASN_UTC_TIME       0x17
# define ASN_GENERAL_TIME   0x18
# define ASN_GRAPHIC_STR    0x19
# define ASN_VISIBLE_STR    0x1A
# define ASN_GENERAL_STR    0x1B
# define ASN_CHARACTER_STR  0x1C
/* 0x1D? */
# define ASN_BMP_STR        0x1E

/* Possibly extended in subsequent octets. */

# define ASN_EXTENSION_ID   0x1F
# define ASN_EXTENSION_BIT  0x80

/* Constant used for the ASN.1 length octet(s) */

# define ASN_LONG_LEN       0x80

/* Macros used for encoding of real values. */

# define ASN_REAL_TYPE	    0xC0

/* Constants used for 8.5.5 type real values. */

# define ASN_REAL_BASE	    0x30
# define ASN_REAL_SCALE	    0x0C
# define ASN_REAL_EXP	    0x03

/* Constants used for 8.5.7 type real values. */

# define ASN_REAL_PLUS_INF  0x40
# define ASN_REAL_MINUS_INF 0x41
