#include <config.h>
#include <trace.h>
#include <log.h>
#include <kernel/kernel.h>

private object log;
private string comp_err;
private int    reset_comp_err;
private string last_rt_err;
private string last_st;

static void create(varargs int clone)
{
  log = find_object(LOGD);
  reset_comp_err = 0;
}

void runtime_error(string error, int caught, mixed** trace)
{
  int size, i, len;
  string line, progname, function, str, objname, err_str;
  object obj;

  if(reset_comp_err) {
    LOGD->write_syslog("Clearing comp_err in runtime_error!", LOG_NORMAL);
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

  log->write_syslog("Runtime error: " + str);
  send_message("Runtime error: " + str);
  if(caught == 0 && this_user() && (obj=this_user()->query_user())) {
    obj->message(str);
  }
}

void atomic_error(string error, int atom, mixed** trace)
{
  if(reset_comp_err) {
    comp_err = nil;
    reset_comp_err = 0;
  } else {
    reset_comp_err = 1;
  }

  log->write_syslog("Atomic error: " + error);
  send_message("Atomic error: " + error);
}

void compile_error(string file, int line, string error)
{
  if(reset_comp_err) {
    LOGD->write_syslog("Clearing comp_err in compile_error!", LOG_NORMAL);
    reset_comp_err = 0;
    comp_err = "";
  }
  if(!comp_err) comp_err = "";
  comp_err += file + ":" + line + "  " + error + "\n";

  log->write_syslog("Compile error: " + file + ":" + (string)line
		    + ": " + error);
  send_message("Compile error!");
}

string last_compile_errors(void) {
  return comp_err;
}

string last_runtime_error(void) {
  return last_rt_err;
}

string last_stack_trace(void) {
  return last_st;
}

void clear_errors(void) {
  if(!SYSTEM()) {
    error("Only System callers can clear errord's errors!");
  }

  comp_err = last_rt_err = last_st = nil;
}
