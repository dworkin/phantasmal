#include <kernel/kernel.h>

#include <phantasmal/log.h>
#include <phantasmal/lpc_names.h>

#include <type.h>

/* This is the AUTO object for objects in /usr/common and potentially
 * other directories.  It's meant to prevent them from doing anything
 * unpleasant without realizing it.  Specifically, it prevents a
 * call_other() to any nonexistent function.
 */

static mixed call_other_unprotected(mixed obj, string function,
                                    mixed args...) {
  return ::call_other(obj, function, args...);
}

/* wrapping up call_other with its helpers */
/* everything else it "fixes" must be turned off */
/* if the "masking" is itself off */
#if 0
static mixed call_other(mixed obj, string function, mixed args...) {
  /* May want to restrict or error-check call_other later. */

  if(typeof(obj) == T_OBJECT && !function_object(function, obj)
     && find_object(LOGD)) {
    ::call_other(LOGD, "write_syslog", object_name(this_object())
                       + " is calling nonexistent function "
                       + object_name(obj) + ":" + function + "!\n",
                       LOG_ERROR);
  } else if(typeof(obj) == T_STRING && find_object(obj)
          && !function_object(function, find_object(obj))
          && find_object(LOGD)) {
    ::call_other(LOGD, "write_syslog", object_name(this_object())
                       + " is calling nonexistent function "
                       + obj + ":" + function + "!\n", LOG_ERROR);
  }

  return ::call_other(obj, function, args...);
}

static string previous_program(varargs int n) {
  int idx;
  string tmp;

  idx = 1;
  while(1) {
    tmp = ::previous_program(idx);
    if(tmp != "/usr/System/open/lib/common_auto") {
      if(!n) {
        return tmp;
      }
      n--;
    }
    idx++;
  }

}

/* Currently I'm still debugging this stuff, so call_trace returns the
   full version, including the extra layer in call_outs, for debugging
   purposes.  In the long run, you'd like call_trace to filter out all
   the intermediate calls to this object's call_out function. */

static mixed **call_trace(void) {
  mixed **trace, *call;
  int i;
  object driver;

  trace = ::call_trace();

  driver = ::find_object(DRIVER);
  
  for (i = sizeof(trace) - 1; --i >= 0; ) {
    if (sizeof(call = trace[i]) > TRACE_FIRSTARG &&
        creator != driver->creator(call[TRACE_PROGNAME])) {
      /* remove arguments */
      trace[i] = call[.. TRACE_FIRSTARG - 1];
    }
  }

  return trace;
}
#endif


/* Previous_object actually doesn't care about the redefined call_other.
   Since it only cares about calls via call_other(), not calling static
   functions defined by one's own AUTO object, its result is unchanged.
*/
#if 0
static object previous_object(varargs int n) {
}
#endif
