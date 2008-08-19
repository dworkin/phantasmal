#include <phantasmal/parser.h>
#include <phantasmal/lpc_names.h>

/* #define LOGGING */

/* Code for super-verbose logging */
#ifdef LOGGING
#define LOG(x) write_file("~/parser.log", x);
#else
#define LOG(x)
#endif


/*
 * This file contains all the various rules to turn lists of
 * parsed tokens into more complex parse-tree structures.
 */

static void create(varargs int clone) {
}

void upgraded(varargs int clone) {
}

/*
 * Every noun phrase is of the form:
 * ({ PHR_NOUN, context, noun, <modifier>, <modifier>, <modifier> })
 *
 * Every modifier is of one of the forms:
 * ({ PHR_ADJ, word, word, word... })
 * ({ PHR_DET, word, word, word... })
 * ({ PHR_PREP, preposition, <noun phrase> })
 *
 * The 'context' field is generally nil for noun phrases.  A noun
 * phrase should have no more than one PHR_ADJ modifier and one
 * PHR_DET modifier, but may have many PHR_PREP modifiers.  The binder
 * will resolve their precedence with respect to the noun and each
 * other.
 *
 * For each of PHR_NOUN, PHR_ADJ and PHR_DET, it is quite acceptable
 * to have only one word or modifier afterward.
 */

static mixed *npr_adj_noun(mixed *tokens) {
  LOG("npr_adj_noun\n");

  if(sizeof(tokens) > 1) {
    return ({ ({ PHR_NOUN, nil, tokens[sizeof(tokens) - 1],
		   ({ PHR_ADJ, tokens[0..sizeof(tokens) - 2]}) }) });
  } else {
    return ({ ({ PHR_NOUN, nil, tokens[0] }) });
  }
}

static mixed *npr_det_adj_noun(mixed *tokens) {
  mixed *ret;

  LOG("npr_det_adj_noun\n");

  ret = npr_adj_noun(tokens[1..]);
  ret[0] += ({ ({ PHR_DET, tokens[0] }) });

  return ret;
}


/* Every independent clause is of one of the forms:
 * ({ IVERB_CLAUSE, context, verb, modifier, modifier, modifier... })
 * ({ TVERB_CLAUSE, context, verb, object, modifier, modifier, modifier... })
 *
 * There may be no modifiers at all in either case, or there may be a
 * large number.  The verb is a string.  The object, if present, is a
 * noun phrase.  The modifiers are each of one of the following forms:
 *
 * ({ PHR_ADV, word, word, word... })
 * ({ PHR_PREP, preposition, <noun phrase> })
 * ({ PHR_IND_OBJ, indicator, <noun phrase> })
 *
 * Indicator is generally nil for an indirect object since they're
 * difficult to tell from prepositional phrases initially.  The cases
 * where they obviously aren't are usually of the form "give john the
 * milk" where there is no preposition ("to" or "for") involved.  So
 * verbs should bear in mind that a modifier marked PHR_PREP may
 * actually be an indirect object.
 */

static mixed *iclause_iverb(mixed *symbols) {
  LOG("iclause_iverb\n");

  if(sizeof(symbols) != 1)
    error("Internal parsing error!");

  return ({ ({ IVERB_CLAUSE, nil, symbols[0] }) });
}

static mixed *iclause_tverb_np(mixed *symbols) {
  LOG("iclause_tverb_np\n");

  if(sizeof(symbols) != 2)
    error("Internal parsing error!");

  return ({ ({ TVERB_CLAUSE, nil, symbols[0], symbols[1] }) });
}
