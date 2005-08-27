#include <phantasmal/phrase.h>
#include <phantasmal/lpc_names.h>
#include <phantasmal/log.h>

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

static string *unq_data_to_taglist(mixed *unq, varargs int no_trim) {
  string *taglist, *tmp_taglist;
  int     ctr;

  taglist = ({ });

  catch {
  for(ctr = 0; ctr < sizeof(unq); ctr += 2) {
    unq[ctr] = STRINGD->trim_whitespace(unq[ctr]);

    if(typeof(unq[ctr + 1]) == T_STRING) {
      /* unq[ctr + 1] = STRINGD->trim_whitespace(unq[ctr + 1]); */
      tmp_taglist = ({ "", unq[ctr + 1] });
    } else {
      /* Recursive call, but don't trim tags */
      tmp_taglist = unq_data_to_taglist(unq[ctr + 1], 1);
    }

    if(!unq[ctr]) {
      taglist += tmp_taglist;
    } else if (sizeof(tmp_taglist) == 0
               || (sizeof(tmp_taglist) == 2 && tmp_taglist[1] == "")) {
      taglist = ({ "*" + unq[ctr], "" });
    } else {
      taglist += ({ "{" + unq[ctr], "" }) + tmp_taglist + ({ "}" + unq[ctr], "" });
    }
  }

  if(!sizeof(taglist) || sizeof(taglist) == 2) return taglist;

  /* Go through and remove pairs of empty strings.  They're
     an artifact of the way we convert from UNQ. */
  if(!no_trim) {
    string *final_taglist;

    final_taglist = ({ taglist[0] });
    for(ctr = 1; ctr + 1 < sizeof(taglist); ctr += 2) {
      if(taglist[ctr] != "" || taglist[ctr + 1] != "") {
        final_taglist += ({ taglist[ctr], taglist[ctr + 1] });
      }
    }
    final_taglist += ({ taglist[ctr] });
    taglist = final_taglist;
  }

  if(sizeof(taglist) % 1) error("Odd-sized taglist being returned!");
  } : {
    LOGD->write_syslog("Error converting UNQ data to taglist.  Data: "
                       + STRINGD->mixed_sprint(unq) + ", Taglist: "
                       + STRINGD->mixed_sprint(taglist), LOG_ERR);
    return nil;
  }
  return taglist;
}

static string *unq_to_taglist(string unq_string) {
  mixed *unq;

  /* Parse the UNQ */
  unq = UNQ_PARSER->basic_unq_parse(unq_string);
  /* unq = UNQ_PARSER->trim_empty_tags(unq); */

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
