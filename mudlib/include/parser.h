/* Constants used in the parsed form of sentances.
 */
#define JOIN_AND 0
#define JOIN_INT 1
#define JOIN_SUB 2
#define PHR_VERB 16
#define PHR_NOUN 17
#define PHR_PREP 18
#define ADJ_NPR 19

/* field for any object type */
#define TYPE 0

/* The different fields of noun phrases */
#define NPR_TYPE 0
#define NPR_NOUN 1
#define NPR_NUMBER 2
#define NPR_OWNER 3
/* adjectives (if any) are stored at the end -- this is the first adjective,
 * if it exists */
#define NPR_ADJ 4
