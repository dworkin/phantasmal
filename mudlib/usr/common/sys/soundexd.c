#include <config.h>

/* The Soundex object contains functions to extract the Soundex algorithm
   string from a given normal string.  For more on Soundex, see
   "http://www.genealogy.org/census/intro-6.html".  C source code is
   available to calculate a Soundex string on KaVir's MUD page, at
   "http://www.kavir.dial.pipex.com/snippets.html".  Thanks to Richard
   Woolcock (KaVir), who put up the C code and documentation from which
   I wrote this LPC implementation. */

#define KeySize 4

private int case_diff;
private string SoundexConvKey;

static void create(varargs int clone) {
  SoundexConvKey = "01230120022455012623010202";
  case_diff = 'a' - 'A';
}

private int letterconvert(int letter) {
  int newletter;

  if((letter <= 'z') && (letter >= 'a')) {
    return SoundexConvKey[letter - 'a'];
  }
  return '0';
}

string get_key(string word) {
  int lowindex, bufindex, letter, len;
  string buf, low;

  if(word == "") return "0000";

  /* KeySize being 4 actually means 4 letters *following* first char */
  buf = "0000000000000000"[0..KeySize];

  low = STRINGD->to_lower(word);
  len = strlen(low);
  buf[0] = low[0];  /* First-char same in Soundex key */

  lowindex = 1;
  bufindex = 1;

  if(len <= 1) return buf;

  do {  /* Loop through low, which is just word in lowercase */
    /* Ignore repeat (double, triple) letters */
    while((lowindex < len - 1) && (low[lowindex] == low[lowindex+1])) {
      lowindex++;
    }

    /* Convert letter to Soundex value and store */
    letter = letterconvert(low[lowindex]);

    if (letter != '0' && bufindex < KeySize) {
      /* Store soundex value */
      buf[bufindex] = letter;
      bufindex++;
      if(bufindex > KeySize) break;
    }
    lowindex++;
  } while(lowindex < len);

  return buf[0..bufindex-1];
}

float match(string first, string second) {
  int match;
  int total, least;
  int ctr;
  int len1, len2;

  total = 0;
  len1 = strlen(first);
  len2 = strlen(second);
  if(len1 > len2) {
    least = len2;
  } else {
    least = len1;
  }

  for(ctr=0, match = 0; ctr < least; ctr++) {
    if((first[ctr] != '0') || (second[ctr] != '0')) {
      total++;
    }

    if(((first[ctr] != '0') && (second[ctr] != '0')) &&
       (first[ctr] == second[ctr])) {
      /* If both are nonzero and match each other, one more character
	 matches */
      match++;
    }
  }

  return ((float)match / (float)total);
}
