#include <config.h>

/* inherit port PORTABLE; */
inherit port ROOM;
inherit unq UNQABLE;

private int pending_location;
private int pending_parent;

/* A simple portable object */

#define PHR(x) PHRASED->new_simple_english_phrase(x)

static void create(varargs int clone) {
  port::create(clone);
  unq::create(clone);
  if(clone) {
    set_brief(PHR("A portable object"));
    set_glance(PHR("A new portable object"));
    set_look(PHR("Why look, it's an object.  Portable, but uninitialized!"));

    pending_location = -1;
    pending_parent = -1;
  }

}

void upgraded(varargs int clone) {
  port::upgraded(clone);
  unq::upgraded(clone);
}

void destructed(varargs int clone) {
  port::destructed(clone);
  unq::destructed(clone);
}


int get_pending_location(void) {
  return pending_location;
}

int get_pending_parent(void) {
  return pending_parent;
}


string to_unq_text(void) {
  string ret, tmp_n, tmp_a;
  int    locale;

  ret = "~portable{\n";
  ret += "  ~number{" + tr_num + "}\n";
  if(location) {
    ret += "  ~location{" + location->get_number() + "}\n";
  }
  ret += "  ~bdesc{" + bdesc->to_unq_text() + "}\n";
  ret += "  ~gdesc{" + gdesc->to_unq_text() + "}\n";
  ret += "  ~ldesc{" + ldesc->to_unq_text() + "}\n";
  if(edesc) {
    ret += "  ~edesc{" + edesc->to_unq_text() + "}\n";
  }

  ret += "  ~article{" + desc_article + "}\n";
  ret += "  ~flags{" + objflags + "}\n";
  if(parent) {
    ret += "  ~parent{" + parent->get_number() + "}\n";
  }

  /* Skip debug locale */
  tmp_n = tmp_a = "";
  for(locale = 1; locale < sizeof(nouns); locale++) {
    if(sizeof(nouns[locale])) {
      tmp_n += "~" + PHRASED->locale_name_for_language(locale) + "{"
	+ implode(nouns[locale], ",") + "}";
    }
    if(sizeof(adjectives[locale])) {
      tmp_a += "~" + PHRASED->locale_name_for_language(locale) + "{"
	+ implode(adjectives[locale], ",") + "}";
    }
  }

  /* The double-braces are intentional -- this uses the efficient
     method of specifying nouns and adjectives rather than the human-
     friendly one. */
  ret += "  ~nouns{{" + tmp_n + "}}\n";
  ret += "  ~adjectives{{" + tmp_a + "}}\n";

  ret += "}\n";

  return ret;
}

void from_dtd_unq(mixed* unq) {
  int ctr, ctr2;

  if(unq[0] != "portable")
    error("Doesn't look like portable data!");

  /* Cut off label "portable" in front */
  unq = unq[1];

  for(ctr = 0; ctr < sizeof(unq); ctr++) {
    if(unq[ctr][0] == "number")
      tr_num = unq[ctr][1];
    else if(unq[ctr][0] == "location")
      pending_location = unq[ctr][1];
    else if(unq[ctr][0] == "bdesc")
      set_brief(unq[ctr][1]);
    else if(unq[ctr][0] == "gdesc")
      set_glance(unq[ctr][1]);
    else if(unq[ctr][0] == "ldesc")
      set_look(unq[ctr][1]);
    else if(unq[ctr][0] == "edesc")
      set_examine(unq[ctr][1]);
    else if(unq[ctr][0] == "article")
      desc_article = unq[ctr][1];
    else if(unq[ctr][0] == "flags")
      objflags = unq[ctr][1];
    else if(unq[ctr][0] == "flags")
      pending_parent = unq[ctr][1];
    else if(unq[ctr][0] == "nouns") {
      for(ctr2 = 0; ctr2 < sizeof(unq[ctr][1]); ctr2++) {
	add_noun(unq[ctr][1][ctr2]);
      }
    } else if(unq[ctr][0] == "adjectives") {
      for(ctr2 = 0; ctr2 < sizeof(unq[ctr][1]); ctr2++) {
	add_adjective(unq[ctr][1][ctr2]);
      }
    } else {
      /* Should never happen on actual DTD-parsed input */
      error("Unrecognized UNQ tag in portable: " + unq[ctr][0]);
    }
  }

}
