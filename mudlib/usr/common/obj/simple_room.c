#include <phrase.h>
#include <config.h>
#include <log.h>

inherit room ROOM;
inherit unq UNQABLE;

private int pending_location;

#define PHR(x) PHRASED->new_simple_english_phrase(x)

static void create(varargs int clone) {
  room::create(clone);
  unq::create(clone);
  if(clone) {
    bdesc = PHR("a room");
    gdesc = PHR("a room");
    ldesc = PHR("You see a room here.");
    edesc = nil;

    pending_location = -1;

    MAPD->add_room_object(this_object());
  }
}

void destructed(int clone) {
  room::destructed(clone);
  unq::destructed(clone);
  if(clone) {
    MAPD->remove_room_object(this_object());
  }
}

int get_pending_location(void) {
  return pending_location;
}


/* Include only exits that appear to have been created from this room
   so that they aren't doubled up when reloaded */
private string exits_to_unq(void) {
  object exit, dest, other_exit;
  mixed* exit_arr;
  int    ctr, opp_dir;
  string ret;
  object shortphr;

  ret = "";
  for(ctr = 0; ctr < sizeof(exits); ctr++) {
    exit_arr = exits[ctr];
    exit = exit_arr[1];
    dest = exit->get_destination();
    if(dest) {
      opp_dir = EXITD->opposite_direction(exit->get_direction());
      other_exit = dest->get_exit(opp_dir);
      if(!other_exit || other_exit->get_destination() != this_object()) {
	LOGD->write_syslog("Problem finding return exit!");
      } else {
	if(exit->get_number() < other_exit->get_number()) {
	  shortphr = EXITD->get_short_for_dir(exit->get_direction());

	  ret += "  ~exit{"
	    + shortphr->get_content_by_lang(LANG_englishUS)
	    + ": #" + dest->get_number() + " "
	    + exit->get_number()
	    + " " + other_exit->get_number() + "}\n";
	}
      }
    } else
      LOGD->write_syslog("Couldn't find destination!", LOG_WARNING);
  }

  return ret;
}

string to_unq_text(void) {
  string ret, tmp_n, tmp_a;
  int    locale;

  ret = "~room{\n";
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

  ret += exits_to_unq();

  ret += "}\n";

  return ret;
}

void from_dtd_unq(mixed* unq) {
  int ctr, ctr2;

  if(unq[0] != "room")
    error("Doesn't look like room data!");

  /* Cut off label "room" in front */
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
    else if(unq[ctr][0] == "nouns") {
      for(ctr2 = 0; ctr2 < sizeof(unq[ctr][1]); ctr2++) {
	add_noun(unq[ctr][1][ctr2]);
      }
    } else if(unq[ctr][0] == "adjectives") {
      for(ctr2 = 0; ctr2 < sizeof(unq[ctr][1]); ctr2++) {
	add_adjective(unq[ctr][1][ctr2]);
      }
    } else if(unq[ctr][0] == "exit") {
      string dirname;
      int    roomnum, dir, exitnum1, exitnum2;

      unq[ctr][1] = STRINGD->trim_whitespace(unq[ctr][1]);
      if(sscanf(unq[ctr][1], "%s: #%d %d %d", dirname, roomnum,
		exitnum1, exitnum2) == 4) {

	dir = EXITD->direction_by_string(dirname);
	if(dir == -1)
	  error("Can't find direction for dirname " + dirname);

	if(tr_num <= 0)
	  error("Can't yet request an exit from an unnumbered room!");

	EXITD->room_request_simple_exit(tr_num, roomnum, dir,
					exitnum1, exitnum2);
      } else {
	error("Can't parse as exit desc: '" + unq[ctr][1] + "'");
      }

    } else {
      /* Should never happen on actual DTD output */
      error("Unrecognized UNQ tag in room: " + unq[ctr][0]);
    }
  }

}
