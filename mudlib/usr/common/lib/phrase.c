#include <phantasmal/phrase.h>
#include <phantasmal/lpc_names.h>

#include <type.h>

/* Phrase functions */
string as_xml(void) {
  error("Subclass must override this function!");
}

void from_xml(string xml_markup) {
  error("Subclass must override this function!");
}

string as_unq(void) {
  error("Subclass must override this function!");
}

void from_unq(string unq_markup) {
  error("Subclass must override this function!");
}


/* Utility functions */

static string taglist_to_markup(mixed *taglist, int markup_type) {
  int    ctr;
  string label, tmp, body, result;

  if(markup_type != MARKUP_UNQ && markup_type != MARKUP_XML)
    error("Unrecognized markup_type in taglist_to_markup!");

  result = "";
  for(ctr = 0; ctr < sizeof(taglist); ctr+=2) {
    label = taglist[ctr];
    if(label == nil)
      error("(nil) label converting a taglist!");

    if(label != "") {
      switch(label[0]) {

      /* Standalone tag, no closing */
      case '*':
        switch(markup_type) {
        case MARKUP_UNQ:
          result += "~" + label[1..] + "{}";
          break;
        case MARKUP_XML:
          result += "<" + label[1..] + " />";
          break;
        }
        break;

      /* Opening tag */
      case '{':
        switch(markup_type) {
        case MARKUP_UNQ:
          result += "~" + label[1..] + "{";
          break;
        case MARKUP_XML:
          result += "<" + label[1..] + ">";
          break;
        }
        break;

      /* Closing tag */
      case '}':
        switch(markup_type) {
        case MARKUP_UNQ:
          result += "}";
          break;
        case MARKUP_XML:
          result += "</" + label[1..] + ">";
          break;
        }
        break;

      /* All labels should be marked as opening, standalone, or closing! */
      default:
        error("Malformed label converting a taglist!");
      }
    }

    /* Now add body text after label */
    result += taglist[ctr + 1];
  }

  return result;
}

static string taglist_to_unq(mixed *taglist) {
  return taglist_to_markup(taglist, MARKUP_UNQ);
}

static string taglist_to_xml(mixed *taglist) {
  return taglist_to_markup(taglist, MARKUP_XML);
}

static string *unq_data_to_taglist(mixed *unq) {
  string *taglist, *tmp_taglist;
  int     ctr;

  if(typeof(unq) == T_STRING)
    return ({ "", unq });

  taglist = ({ });

  for(ctr = 0; ctr < sizeof(unq); ctr += 2) {
    unq[ctr] = STRINGD->trim_whitespace(unq[ctr]);

    if(typeof(unq[ctr + 1]) == T_STRING) {
      unq[ctr + 1] = STRINGD->trim_whitespace(unq[ctr + 1]);
      if(unq[ctr + 1] == "") {
        taglist += ({ "*" + unq[ctr], "" });
      } else {
        taglist += ({ "{" + unq[ctr], unq[ctr + 1], "}" + unq[ctr], "" });
      }
    } else {
      tmp_taglist = unq_data_to_taglist(unq[ctr + 1]);
      taglist += ({ unq[ctr], "" });
      taglist += tmp_taglist;
    }
  }

  /* If the very first element was a blank label, use an empty string */
  if(sizeof(taglist) && taglist[0] && (strlen(taglist[0]) == 1)) {
    taglist[0] = "";
  }

  /* TODO:  Go through this just once per call rather than
     at every recursive level. */
  /* Go through and remove pairs of empty strings.  They're
     an artifact of the way we convert from UNQ. */
  if(sizeof(taglist) <= 2) return taglist;

  for(ctr = 1; ctr + 1 < sizeof(taglist); ctr += 2) {
    if(taglist[ctr] == "" && taglist[ctr + 1] == "") {
      taglist = taglist[..ctr-1] + taglist[ctr+2..];
      ctr -= 2;
    }
  }

  if(sizeof(taglist) % 1) error("Odd-sized taglist being returned!");
  return taglist;
}

static string *unq_to_taglist(string unq_string) {
  mixed *unq;

  /* Parse the UNQ, remove any tags where the label (if any) and content
     are both whitespace */
  unq = UNQ_PARSER->basic_unq_parse(unq_string);
  unq = UNQ_PARSER->trim_empty_tags(unq);

  return unq_data_to_taglist(unq);
}

static string *xml_to_taglist(string xml_string) {
  error("Not yet implemented!");
}

static string *markup_to_taglist(string markup, int markup_type) {
  switch(markup_type) {
  case MARKUP_UNQ:
    return unq_to_taglist(markup);
  case MARKUP_XML:
    return xml_to_taglist(markup);
  default:
    error("Unrecognized markup_type tag in markup_to_taglist!");
  }
}
