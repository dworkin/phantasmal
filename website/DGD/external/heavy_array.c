#include <config.h>

/* This will eventually be an array type capable of holding unlimited
   numbers of items.  At the moment it's stubbed out to simply contain
   a single DGD-standard array.  When we need more objects, we'll need
   to change this as well to match. */

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
  /* Just for getting this stuff up and running... */
  if(ind > 10000)
    error("Need to rethink this array plan!");

  if(ind < 0) error("heavy_array: Can't set negative index!");
  if(ind >= size) {
    val += allocate(ind - size + 1);
    size = ind + 1;
  }
  val[ind] = value;
}
