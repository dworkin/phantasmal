#include <kernel/kernel.h>

#include <phantasmal/log.h>

#include <config.h>
#include <gameconfig.h>

/* This is the AUTO object for scripts.  It's meant to prevent them
 * from doing anything unpleasant, especially without permission. */

static object find_object(string path) {
  string objname, scriptname;
  object obj;

  if(sscanf("%s:%s", objname, scriptname) == 2) {
    obj = ::find_object("/usr/" + objname + "/script/" + scriptname);
    if(obj)
      return obj;
    else
      return nil;
  }

  return nil;
}

static object compile_object(string path, varargs string source) {
  if(source)
    error("Scripts should not compile new objects from source!");

  error("No compiling allowed yet...");
}

static object clone_object(string path, varargs string uid) {
  error("Scripts should not clone new heavyweight objects!");
}

static int destruct_object(mixed obj) {
  error("Scripts should not destruct objects!");
}

static object new_object(mixed obj, varargs string uid) {
  return ::new_object(obj, uid);
}

static mixed **call_trace() {
  error("Scripts aren't allowed call traces yet!");
}

static mixed *status(varargs mixed obj) {
  error("Scripts aren't allowed any status yet!");
}

static object this_user() {
  error("Not set yet!");
}

static object *users() {
  error("Not set yet!");
}

static void swapout() {
  error("Scripts can't call swapout!");
}

static void dump_state() {
  error("Scripts can't call dump_state!");
}

static void shutdown() {
  error("Scripts can't call shutdown!");
}

static mixed call_limited(string function, mixed args...) {
  error("Scripts can't call call_limited yet!");
}

static int call_out(string function, mixed delay, mixed args...) {
  error("Scripts can't call call_out!  Use TimeD instead.");
}

static mixed remove_call_out(int handle) {
  error("Scripts can't call remove_call_out!");
}

static void add_event(string name) {
  error("Scripts can't use events!");
}

static void remove_event(string name) {
  error("Scripts can't use events!");
}

static string *query_events() {
  error("Scripts can't use events!");
}

static void subscribe_event(object obj, string name) {
  error("Scripts can't use events!");
}

static void unsubscribe_event(object obj, string name) {
  error("Scripts can't use events!");
}

static object *query_subscribed_event(string name) {
  error("Scripts can't use events!");
}

static void event(string name, mixed args...) {
  error("Scripts can't use events!");
}

static void event_except(string name, object *exclude, mixed args...) {
  error("Scripts can't use events!");
}

static string read_file(string path, varargs int offset, int size) {
  error("Scripts can't use files!");
}

static int write_file(string path, string str, varargs int offset) {
  error("Scripts can't use files!");
}

static int remove_file(string path) {
  error("Scripts can't use files!");
}

static int rename_file(string from, string to) {
  error("Scripts can't use files!");
}

static mixed **get_dir(string path) {
  error("Scripts can't use files!");
}

static mixed *file_info(string path) {
  error("Scripts can't use files!");
}

static int make_dir(string path) {
  error("Scripts can't use files!");
}

static int remove_dir(string path) {
  error("Scripts can't use files!");
}

static int restore_object(string path) {
  error("Scripts can't use files!");
}

static void save_object(string path) {
  error("Scripts can't use files!");
}

static string editor(varargs string cmd) {
  error("Scripts can't use the editor!");
}

static string query_editor(object obj) {
  error("Scripts can't use the editor!");
}

static string query_ip_name(object user) {
  error("Scripts can't check network data!");
}

static string query_ip_number(object user) {
  error("Scripts can't check network data!");
}

static void block_input(int flag) {
  error("Scripts can't block network input!");
}

/* Test function -- scripts should do *something*... */
static void write_log(string msg) {
  ::find_object(LOGD)->write_syslog("Script: " + msg);
}

static mixed call_other(mixed obj, string function, mixed args...) {
  /* May want to restrict or error-check call_other later. */

  return ::call_other(obj, function, args...);
}
