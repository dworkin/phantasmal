/*
 * $Id: harte-errord.c,v 1.1.1.1 2003/04/03 17:12:48 angelbob Exp $
 *
 * External interface:
 *
 *     void compile_error(string file, int line, string err);
 *     void runtime_error(string str, int caught, mixed **trace);
 *     void atomic_error(string str, int atom, mixed **trace);
 */

#include <kernel/kernel.h>
#include <trace.h>

private mapping compile_errors;

/*
 * Register this object as the new error manager.
 */
static void
create()
{
    compile_errors = ([ ]);
    find_object(DRIVER)->set_error_manager(this_object());
}

/*
 * Align the int/string by appending spaces.
 */
private string
lalign(mixed s, int width)
{
    int len;
    string str;

    s = (string)s;
    width -= strlen(s);
    if (width <= 0) {
        return s;
    }
    str = "                                                                ";
    len = 64;
    while (width > len) {
        len <<= 1;
        str += str;
    }
    return s + str[..width - 1];
}

/*
 * Align the int/string by inserting spaces.
 */
private string
ralign(mixed s, int width)
{
    int len;
    string str;

    s = (string)s;
    width -= strlen(s);
    if (width <= 0) {
        return s;
    }
    str = "                                                                ";
    len = 64;
    while (width > len) {
        len <<= 1;
        str += str;
    }
    return str[..width - 1] + s;
}

/*
 * Format one group of compile errors.
 */
private string
format_compile_error(string file, int timestamp, mapping lines)
{
    int    i, sz, *linenrs, total;
    string str, **errors;

    total   = 0;
    str     = "";

    sz      = map_sizeof(lines);
    linenrs = map_indices(lines);
    errors  = map_values(lines);
    for (i = 0, total = 0; i < sz; i++) {
        string linenr;

        linenr = ralign(linenrs[i], 5) + "   ";
        str += linenr + implode(errors[i], "\n" + linenr) + "\n";
	total += sizeof(errors[i]);
    }
    if (total == 1) {
	str = "Compile error in " + file + "\n" + str;
    } else {
	str = "Compile errors in " + file + "\n" + str;
    }
    return str;
}

/*
 * Flush compile errors to their respective files.
 */
private void
flush_compile_errors()
{
    int    i, sz;
    string *files;
    mixed  **data;

    sz = map_sizeof(compile_errors);
    if (!sz) {
	return;
    }
    files = map_indices(compile_errors);
    data  = map_values(compile_errors);
    for (i = 0; i < sz; i++) {
	string file, text;

	file = files[i];
	text = format_compile_error(file,
				    data[i][0],
				    data[i][1]) + "\n";
	find_object(DRIVER)->message(text);
    }
    compile_errors = ([ ]);
}

/*
 * Store compile error in mapping.
 */
void
compile_error(string file, int line, string err)
{
    mixed *data;

    data = compile_errors[file];
    if (data) {
        string *list;

        list = data[1][line];
        if (list) {
            data[1][line] = list + ({ err });
        } else {
            data[1][line] = ({ err });
        }
    } else {
        compile_errors[file] = ({ time(), ([ line: ({ err }) ]) });
    }
}

/*
 * Reformat a filepath.
 */
private string
format_path(string path)
{
    return sscanf(path, "/usr/%*s/") ? "~" + path[5..] : path;
}

/*
 * Process a call trace.
 */
private string
format_trace(mixed **trace, int marker)
{
    int i, j, sz, maxlen;
    string result, **lines;

    sz = sizeof(trace);
    lines = allocate(sz * 2);
    for (i = j = maxlen = 0; i < sz; i++) {
	int    linenr;
        string objname, progname, function, line, last_obj, last_prog;
        mixed  *elt;

        elt = trace[i];
        objname  = elt[TRACE_OBJNAME];
        progname = elt[TRACE_PROGNAME];
        function = elt[TRACE_FUNCTION];
        linenr   = elt[TRACE_LINE];

        if (objname != last_obj) {
            string name;
            object ob;

            line = "        " + format_path(objname);
            if ((ob = find_object(objname)) && (name = ob->query_name())) {
                line += " \"" + name + "\"";
            }
            lines[j++] = ({ line, nil });
            last_obj = last_prog = objname;
        }
        if (progname == AUTO && strlen(function) > 3) {
            switch (function[..2]) {
            case "bad":
            case "_F_":
            case "_Q_":
		continue;
            }
        }
        line = (linenr ? ralign(linenr, 5) : "     ") +
               (i == marker ? " > " : "   ") +
	       "   " +
               (progname == last_prog ? "-" : format_path(progname));
        last_prog = progname;
        if (strlen(line) > maxlen) {
            maxlen = strlen(line);
        }
        lines[j++] = ({ line, function });
    }
    result = "";
    for (i = 0; i < j; i++) {
        if (lines[i][1]) {
            result += lalign(lines[i][0], maxlen) + "   " + lines[i][1] + "\n";
        } else {
            result += lines[i][0] + "\n";
        }
    }
    return result;
}

/*
 * Process a runtime error trace.
 */
void
runtime_error(string str, int caught, mixed **trace)
{
    flush_compile_errors();

    find_object(DRIVER)->message(str + (caught ? " [caught]\n" : "\n") +
				 format_trace(trace[..sizeof(trace) - 2],
					      caught - 1) +
				 "\n");
}

/*
 * Process an atomic error trace.
 */
void
atomic_error(string str, int atom, mixed **trace)
{
    flush_compile_errors();

    find_object(DRIVER)->message(str + " [atomic]\n" +
				 format_trace(trace[atom..sizeof(trace) - 2],
					      - 1) +
				 "\n");
}
