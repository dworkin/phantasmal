#include <config.h>
#include <type.h>
#include <kernel/kernel.h>
#include <log.h>

private int* mobile_segments;

/* Prototypes */
void upgraded(varargs int clone);
private int allocate_mobile_obj(int num, object obj);

#define PHR(x) PHRASED->new_simple_english_phrase(x)

static void create(varargs int clone) {
  if(clone)
    error("Cloning mobiled is not allowed!");

  /* if(!find_object(SIMPLE_EXIT))
     compile_object(SIMPLE_EXIT); */

  mobile_segments = ({ });

  upgraded();

}

void upgraded(varargs int clone) {

}

void destructed(int clone) {

}


void add_mobile_number(object mobile, int num) {
  int newnum;

  newnum = allocate_mobile_obj(num, mobile);
  if(newnum <= 0) {
    error("Can't allocate mobile number!");
  }
}

private int allocate_mobile_obj(int num, object obj) {
  int segment;

  if(num >= 0 && OBJNUMD->get_object(num))
    error("Object already exists with number " + num);

  if(num != -1) {
    if(!(sizeof( ({ num / 100 }) & mobile_segments ))) {
      string tmp;

      mobile_segments |= ({ num / 100 });
    }

    OBJNUMD->allocate_in_segment(num / 100, num, obj);
    return num;
  }

  for(segment = 0; segment < sizeof(mobile_segments); segment++) {
    num = OBJNUMD->new_in_segment(mobile_segments[segment], obj);
    if(num != -1) {
      return num;
    }
  }

  segment = OBJNUMD->allocate_new_segment();

  mobile_segments += ({ segment });
  num = OBJNUMD->new_in_segment(segment, obj);

  return num;
}

void remove_mobile(object mobile) {
  destruct_object(mobile);
}
