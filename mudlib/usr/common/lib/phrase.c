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

static mixed *unq_data_to_taglist(mixed *unq) {
  string *taglist, *tmp_taglist;
  int     ctr;

  taglist = ({ });

  for(ctr = 0; ctr < sizeof(unq); ctr += 2) {
    unq[ctr] = STRINGD->trim_whitespace(unq[ctr]);

    if(typeof(unq[ctr + 1]) == T_STRING) {
      unq[ctr + 1] = STRINGD->trim_whitespace(unq[ctr + 1]);
      taglist += ({ unq[ctr], unq[ctr + 1] });
    } else {
      tmp_taglist = unq_data_to_taglist(unq[ctr + 1]);
      if(unq[ctr] && strlen(unq[ctr])) {
        taglist += ({ unq[ctr], "" });
      }
      /* Empty outer tag */
      taglist += tmp_taglist;
    }
  }

  /* TODO:  Go through this just once at the end rather than
     at every recursive level. */
  /* Go through and remove pairs of empty strings.  They're
     an artifact of the way we convert from UNQ. */
  for(ctr = 1; ctr < sizeof(taglist); ctr += 2) {
    if(taglist[ctr] == "" && taglist[ctr + 1] == "") {
      taglist = taglist[..ctr-1] + taglist[ctr+2..];
      ctr -= 2;
    }
  }
  return taglist;
}

static mixed *unq_to_taglist(string unq_string) {
  mixed *unq;

  /* Parse the UNQ, remove any tags where the label (if any) and content
     are both whitespace */
  unq = UNQ_PARSER->basic_unq_parse(unq_string);
  unq = UNQ_PARSER->trim_empty_tags(unq);

  return unq_data_to_taglist(unq);
}

static mixed *xml_to_taglist(string xml_string) {
  error("Not yet implemented!");
}

static mixed *markup_to_taglist(string markup, int markup_type) {
  switch(markup_type) {
  case MARKUP_UNQ:
    return unq_to_taglist(markup);
  case MARKUP_XML:
    return xml_to_taglist(markup);
  default:
    error("Unrecognized markup_type tag in markup_to_taglist!");
  }
}
