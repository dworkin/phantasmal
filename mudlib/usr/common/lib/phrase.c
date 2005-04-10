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

  result = "";
  for(ctr = 0; ctr < sizeof(taglist); ctr+=2) {
    label = nil;
    if(taglist[ctr] && taglist[ctr] != "") {
      label = taglist[ctr];
    }

    switch(typeof(taglist[ctr + 1])) {
    case T_STRING:
      body = taglist[ctr + 1];
      break;
    case T_ARRAY:
      body = taglist_to_markup(taglist[ctr + 1], markup_type);
      break;
    default:
      error("Malformed taglist in taglist_to_unq!");
    }

    if(body == "") {
      /* Empty tag */
      switch(markup_type) {
      case MARKUP_UNQ:
	result += "~" + label + "{}";
	break;
      case MARKUP_XML:
	result += "<" + label + " />";
	break;
      }
    } else if(label) {
      /* Non-empty tag */
      switch(markup_type) {
      case MARKUP_UNQ:
	result += "~" + label + "{" + body + "}";
	break;
      case MARKUP_XML:
	result += "<" + label + ">" + body + "</label>";
	break;
      }
    } else {
      result += body;
    }
  }

  return result;
}

static string taglist_to_unq(mixed *taglist) {
  return taglist_to_markup(taglist, MARKUP_UNQ);
}

static string taglist_to_xml(mixed *taglist) {
  return taglist_to_markup(taglist, MARKUP_XML);
}
