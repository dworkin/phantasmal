#include <kernel/kernel.h>

#include <phantasmal/log.h>
#include <phantasmal/channel.h>
#include <phantasmal/lpc_names.h>

#include <trace.h>

inherit COMMON_AUTO;

private object log;
private string comp_err;
private int    reset_comp_err;
private string last_rt_err;
private string last_st;
private int    in_atomic_error;
private int    in_init_sequence;

static void create(varargs int clone)
{
  log = find_object(LOGD);
  reset_comp_err = 0;
  in_atomic_error = 0;
  in_init_sequence = 1;
}

void runtime_error(string error, int caught, mixed** trace)
{
  int size, i, len;
  string line, progname, function, str, objname, err_str;

  if(!SYSTEM() && !KERNEL())
    return;

  if(reset_comp_err) {
    log->write_syslog("Clearing comp_err in runtime_error!", LOG_VERBOSE);
    comp_err = nil;
    reset_comp_err = 0;
  } else {
    reset_comp_err = 1;
  }

  str = error;

  if(caught != 0) {
    str += " [caught]";
  }
  last_rt_err = str;

  err_str = str + "\n";
  str = "";

  if(sscanf(err_str, "Failed to compile%*s") > 0) {
    return;
  }

  size = sizeof(trace) - 1;
  for(i = 0; i < size; i++) {
    progname = trace[i][TRACE_PROGNAME];
    len = trace[i][TRACE_LINE];
    if(len == 0) {
      line = "    ";
    } else {
      line = "    " + (string)len;
      line = line[strlen(line) - 4 ..];
    }

    function = trace[i][TRACE_FUNCTION];
    len = strlen(function);
    if(progname == AUTO && i != size - 1 && len > 3) {
      switch(function[..2]) {
      case "bad":
      case "_F_":
      case "_Q_":
        continue;
      }
    }
    if(len < 17) {
      function += "                 "[len..];
    }

    objname = trace[i][TRACE_OBJNAME];
    if(progname != objname) {
      len = strlen(progname);
      if(len < strlen(objname) && progname == objname[.. len - 1]
         && objname[len] == '#') {
        objname = objname[len..];
      }
      str += line + " " + function + " " + progname + " (" + objname
        + ")\n";
    } else {
      str += line + " " + function + " " + progname + "\n";
    }
  }
  last_st = str;
  str = err_str + str;

  /* If the first character is a $, this is a silent error.  Store it as
   * the last error, but don't log it.
   */
  if (error[0] != '$') {
    if(caught) {
      log->write_syslog("Runtime error: " + str, LOG_WARNING);
    } else {
      log->write_syslog("Runtime error: " + str, LOG_ERROR);
    }
    send_message("Runtime error: " + str);
    if(in_init_sequence) {
      DRIVER->message("Runtime error: " + str);
    }
    if(caught == 0 && this_user()) {
      this_user()->message(str);
    }
    if(!caught && find_object(CHANNELD)) {
      CHANNELD->string_to_channel(CHANNEL_ERR, str);
    }
  }
}

void atomic_error(string error, int atom, mixed** trace)
{
  int size, i, len;
  string line, progname, function, str, objname, err_str;
  object obj;

  if(!SYSTEM() && !KERNEL())
    return;
  
  /* prevents recursion into atomic_error().  Without this the driver
   * will crash if an error occures within atomic_error().
   *
   * I discovered this from experience :(.
   * (kdunwoody)
   *
   * N.B.  I chose to write a driver message, but no log message since you
   * can't write to file from within an atomic, and this seems to count as
   * an atomic.
   */

  if (in_atomic_error) {
    DRIVER->message("Re-entering atomic_error()!  Bailing...\n");

    return;
  }

  in_atomic_error = 1;

  /* if the first character of the error message is '$', this is a silent
   * error -- do not record it */
  if (error[0] == '$') {
    return;
  }

  str = error;

  str += " [atomic]\n";

  err_str = str + "\n";
  str = "";


  size = sizeof(trace) - 1;
  for (i = atom; i < size; i++) {
    progname = trace[i][TRACE_PROGNAME];
    len = trace[i][TRACE_LINE];
    if (len == 0) {
      line = "    ";
    } else {
      line = "    " + len;
      line = line[strlen(line) - 4 ..];
    }
    
    function = trace[i][TRACE_FUNCTION];
    len = strlen(function);
    if (progname == AUTO && i != size - 1 && len > 3) {
      switch (function[.. 2]) {
      case "bad":
      case "_F_":
      case "_Q_":
        continue;
      }
    }
    if (len < 17) {
      function += "                 "[len ..];
    }

    objname = trace[i][TRACE_OBJNAME];
    if (progname != objname) {
      len = strlen(progname);
      if (len < strlen(objname) && progname == objname[.. len - 1] &&
          objname[len] == '#') {
        objname = objname[len ..];
      }
      str += line + " " + function + " " + progname + " (" + objname +
        ")\n";
    } else {
      str += line + " " + function + " " + progname + "\n";
    }
  }
  
  /* 
   * Don't set last_st -- it will just be rolled back almost as
   * soon as this function exits

   last_st = str; 

  */
  str = err_str + str;

  /* 
   * Re-throws the error with the full stack trace.  This was recommended
   * by Par Winzell of Skotos as being a good way to pass the stack trace
   * up to runtime_error(). (kdunwoody)
   */
  error(str);
  in_atomic_error = 0;
}

void compile_error(string file, int line, string error)
{
  if(!SYSTEM() && !KERNEL())
    return;

  if(reset_comp_err) {
    log->write_syslog("Clearing comp_err in compile_error!", LOG_VERBOSE);
    reset_comp_err = 0;
    comp_err = "";
  }
  if(!comp_err) comp_err = "";
  comp_err += file + ":" + line + "  " + error + "\n";

  log->write_syslog("Compile error: " + file + ":" + (string)line
                    + ": " + error);
  send_message("Compile error!");
  if(in_init_sequence) {
    DRIVER->message("Compile error: " + file + ": " + (string)line
                    + ": " + error + "\n");
  }
}

string last_compile_errors(void) {
  if(SYSTEM())
    return comp_err;

  return nil;
}

string last_runtime_error(void) {
  if(SYSTEM())
    return last_rt_err;

  return nil;
}

string last_stack_trace(void) {
  if(SYSTEM())
    return last_st;

  error("No stack trace for you!");
}

void clear_errors(void) {
  if(!SYSTEM()) {
    error("Only System callers can clear errord's errors!");
  }

  comp_err = last_rt_err = last_st = nil;
}

void done_with_init(void) {
  if(previous_program() == INITD)
    in_init_sequence = 0;
}
