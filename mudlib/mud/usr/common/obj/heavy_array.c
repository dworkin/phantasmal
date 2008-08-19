/* Released into the public domain Jan 2002 by Noah Gibbs.
   Everything in this file is in the public domain, and is
   released without warranty, guarantee, or even the implication
   of marketability or fitness for a particular purpose.
   If you start a war in Asia with this, don't come crying to
   me. */

#include <phantasmal/lpc_names.h>

/* This will eventually be an array type capable of holding unlimited
   numbers of items.  At the moment it's stubbed out to simply contain
   a single DGD-standard array.  When we need more objects, we'll need
   to change this as well to match. */

/* The HEAVY_ARRAY type is a single heavyweight object which will wind up
   containing all the lightweight objects.  If we kept references to any
   of the LWOs in this object also they'd be copied here at the end of
   thread execution - that means we need to keep all such references
   confined to staying inside the issues object (the HEAVY_ARRAY). */

private mixed* val;
private int    size;

static int create(varargs int clone) {
  if(clone) {
    val = ({ });
    size = 0;
  }
}

mixed index(int ind) {
  if(ind < 0) return nil;
  if(ind >= size) return nil;
  return val[ind];
}

void set_index(int ind, mixed value) {
  if(ind < 0) error("heavy_array: Can't set negative index!");
  if(ind >= size) {
    val += allocate(ind - size + 1);
    size = ind + 1;
  }
  val[ind] = value;
}
