#include <type.h>

#define NUM_WIDTH 0
#define NUM_PRECISION 1
#define NUM_INDENT 2
#define NUM_NOTHING -1
#define ALIGN_LEFT 0
#define ALIGN_RIGHT 1
#define ALIGN_CENTRE 2
#define ALIGN_JUSTIFIED 3
#define TYPE_NORMAL 0
#define TYPE_COLUMN 1
#define TYPE_TABLE 2

inherit "/kernel/lib/strings";

string *result;
string format;
mixed *args;
int posn;
int output_line_index;
mixed *numbers;
int alignment;
int type;
int err;
string pad_char;

private int error(int flag, string error_type) {
  if (flag) {
    debug_message("sprintf error: "+error_type);
    err=1;
  }
  return flag;
}

string repeat_string(string str, int length) {
  /* Returns a string of the given length consisting of repetitions of str */

  string result;
  int current_length;

  if (length>0) {
    result=str;
    current_length=strlen(result);
    while(current_length<=length) {
      result+=result;
      current_length+=current_length;
    }
    return result[..length-1];
  } else {
    return "";
  }
}

string make_spaces(int num) {
  return repeat_string(" ", num);
}

private string justify_text(string s, int width) {
  string *bits;
  int extra;
  int size;
  int x, y, r;

  x=strlen(s)-1;
  while (s!=" " && s[x]==' ') {
    s=s[..x-1];
    x--;
  }
  extra=width-strlen(s);
  if (extra<=0) {
    return s;
  }
  bits=explode(s, " ");
  size=sizeof(bits)-1;
  if (size<1) {
    return s;
  }
  r=extra; x=y=0;
  while(y<extra) {
    while (r>=0 && y<extra) {
      bits[x]+=" ";
      r=r-size;
      y++;
    }
    while (r<0) {
      r=r+extra;
      x++;
    }
  }
  return implode(bits, " ");
}

private void add_strings(string *str, int use_width, int line_index, int new_line_width, int final_width) {
  /* Add the strings in str to result.  use_width is the effective field
     width - they will all be padded with spaces on the right to bring
     the size up to width. 
     new_line_width is the number of spaces to pad any new lines that get
     added.
     If numbers[NUM_INDENT]>0, the first string will be indented by that
     amount.  If <0, all but the first string will be indented.  It is
     up to the caller to ensure that the strings are sufficiently shorter
     than use_width to allow indenting.
     If alignment==ALIGN_JUSTIFIED, the last string in the array will be
     set left aligned.  Text should be added one paragraph at a time.
     After setting, all strings will be truncated to use_width. */

  int i;
  string this_line;
  int spaces;
  string blank;
  int indent;
  string indent_spaces;
  int indent_first;
  int indent_this;

  if (sizeof(str)==0) {
    return;
  }
  if (use_width==0) {
    /* They didn't specify a field width, so use the max of precision and
       the longest string */
    use_width=numbers[NUM_PRECISION][0];
    for (i=0;i<sizeof(str);i++) {
      if (strlen(str[i])>use_width) use_width=strlen(str[i]);
    }
  }
  if (numbers[NUM_INDENT]>0) {
    indent_first=1;
    indent=numbers[NUM_INDENT];
  } else {
    indent_first=0;
    indent=-numbers[NUM_INDENT];
  }
  indent_spaces=repeat_string(pad_char, indent);
  /* Expand result to fit all of the lines */
  blank=repeat_string(pad_char, new_line_width);
  while ((sizeof(str)+output_line_index+line_index)>sizeof(result)) result+=({ blank });
  blank=repeat_string(pad_char, final_width-use_width);
  for (i=0;i<sizeof(str);i++) {
    spaces=use_width-strlen(str[i]);
    indent_this=(i==0 && indent_first) || (i!=0 && !indent_first);
    switch(alignment) {
      case ALIGN_RIGHT:
	if (indent_this) {
	  this_line=repeat_string(pad_char, spaces-indent)+str[i]+indent_spaces;
	} else {
	  this_line=repeat_string(pad_char, spaces)+str[i];
	}
	break;
      case ALIGN_LEFT:
	if (indent_this) {
	  this_line=indent_spaces+str[i]+repeat_string(pad_char, spaces-indent);
	} else {
	  this_line=str[i]+repeat_string(pad_char, spaces);
	}
	break;
      case ALIGN_CENTRE:
	/* Indenting doesn't make sense for centred text, so ignore it */
	this_line=repeat_string(pad_char, spaces/2)+str[i]+repeat_string(pad_char, use_width-spaces/2);
	break;
      case ALIGN_JUSTIFIED:
	if (i==sizeof(str)-1) {
	  if (indent_this) {
	    this_line=indent_spaces+str[i];
	    this_line=this_line+repeat_string(pad_char, use_width-strlen(this_line));
	  } else {
	    this_line=str[i];
	    this_line=this_line+repeat_string(pad_char, use_width-strlen(this_line));
	  }
	} else {
	  if (indent_this) {
	    this_line=indent_spaces+justify_text(str[i], use_width-indent);
	  } else {
	    this_line=justify_text(str[i], use_width);
	  }
	  /* pad in case the justify failed (eg. this line was a single word) */
	  this_line=this_line+repeat_string(pad_char, use_width-strlen(this_line));
	}
	break;
    }
    /* Truncate to the effective field width, pad to the real field width, and
       add it to result */
    result[output_line_index+line_index+i]+=this_line[0..use_width-1]+blank;
  }
}

int add_paragraph(string str, int line_index, int new_line_width) {
  /* This does the bulk of the work for column mode.  line_index is the
     line to add the data to.  It's an offset from output_line_index again,
     just to confuse you.  The return value is the line index for the next
     paragraph (if there is one) - the next line after the last one we
     use. */

  string *words;
  int word_index;
  string *lines;
  string this_line;
  int available_width;

  if (!error(numbers[NUM_WIDTH]==0, "column mode must have a field width")) {
    if (str!="") {
      lines=({ });
      if (numbers[NUM_PRECISION][0]==0) {
        numbers[NUM_PRECISION][0]=numbers[NUM_WIDTH];
      }
      if (strlen(str)<=numbers[NUM_PRECISION][0]) {
        lines=({ str });
      } else {
        words=explode(str, " ");
        word_index=0;
        while (word_index<sizeof(words)) {
          this_line="";
          available_width=numbers[NUM_PRECISION][0];
          if ((numbers[NUM_INDENT]>0 && sizeof(lines)==0) || (numbers[NUM_INDENT]<0 && sizeof(lines)!=0)) {
	    available_width-=abs(numbers[NUM_INDENT]);
          }
          while (word_index<sizeof(words) && strlen(this_line)+strlen(words[word_index])<available_width) {
	    if (this_line!="") {
	      this_line+=" ";
	    }
	    this_line+=words[word_index++];
          }
          lines+=({ this_line });
        }
      }
      add_strings(lines, numbers[NUM_PRECISION][0], line_index, new_line_width, numbers[NUM_WIDTH]);
      line_index+=sizeof(lines);
    }
  }
  return line_index;
}

private void tidy_line_lengths() {
  /* Make sure all lines in result are the same length */
  
  int desired_length;
  int this_length;
  int last_length;
  int i;
  string blank;

  if (sizeof(result)) {
    desired_length=strlen(result[output_line_index]);
    last_length=desired_length;
    blank="";
    for (i=output_line_index;i<sizeof(result);i++) {
      this_length=strlen(result[i]);
      if (last_length!=this_length) {
	blank=repeat_string(pad_char, desired_length-this_length);
	last_length=this_length;
      }
      result[i]+=blank;
    }
  }
}

private void do_string(mixed arg) {
  string *bits;
  int i;
  int current_line;
  int new_line_width;

  /* !!! When column mode is given multiple column widths, it should use
     all of them, wrapping text from the bottom of one to the top of the
     next.  */
  error(typeof(arg)!=T_STRING, "expecting a string argument");
  bits=explode(arg, "\n");
  switch (type) {
    case TYPE_NORMAL:
      for (i=0;i<sizeof(bits);i++) {
        add_strings(({bits[i]}), numbers[NUM_WIDTH], i, 0, numbers[NUM_WIDTH]);
      }
      break;
    case TYPE_COLUMN:
      current_line=0;
      new_line_width=strlen(result[output_line_index]);
      for (i=0;i<sizeof(bits);i++) {
	current_line=add_paragraph(bits[i], current_line, new_line_width);
      }
      break;
    default:
      error(1, "Illegal mode for a string argument");
  }
  tidy_line_lengths();
}

private void do_array(mixed arg) {
  int *widths;
  int num_columns, num_rows;
  int i;
  int gap;
  int total_width;
  string *this_column;
  int new_line_width;
  int widest;
  string blank;

  error(typeof(arg)!=T_ARRAY, "expecting an array argument");
  if (sizeof(numbers[NUM_PRECISION])==1) {
    if (numbers[NUM_PRECISION][0]==0) {
      /* They haven't told us anything.  This is the hard case. */
      error(numbers[NUM_WIDTH]==0, "must specify a field width");
      /* For now, just find the widest string and make all columns that size.
         It should make each column just wide enough to fit the longest string
         that will be going in it. */
      widest=0;
      for (i=0;i<sizeof(arg);i++) {
        if (strlen(arg[i])>widest) {
  	  widest=strlen(arg[i]);
        }
      }
      num_columns=numbers[NUM_WIDTH]/(widest+1);
      if (num_columns==0) {
        num_columns=1;
      }
      widths=({ });
      for (i=0;i<num_columns;i++) {
        widths+=({ numbers[NUM_WIDTH]/num_columns });
      }
    } else {
      /* They've given one precision, which we will take as the number of
         columns */
      error(numbers[NUM_WIDTH]==0, "must specify a field width");
      num_columns=numbers[NUM_PRECISION][0];
      widths=({ });
      for (i=0;i<num_columns;i++) {
        widths+=({ numbers[NUM_WIDTH]/num_columns });
      }
    }
  } else {
    /* They've given us the column widths.  Life is easy */
    num_columns=sizeof(numbers[NUM_PRECISION]);
    widths=numbers[NUM_PRECISION];
  }
  /* We now have the number of columns and the array of column widths. */
  total_width=0;
  for (i=0;i<num_columns;i++) {
    total_width+=widths[i];
  }
  num_rows=(sizeof(arg)+num_columns-1)/num_columns;
  new_line_width=strlen(result[output_line_index]);
  gap=(numbers[NUM_WIDTH]-total_width)/num_columns;
  for (i=0;i<num_columns;i++) {
    if (i*num_rows<sizeof(arg)) {
      if (((i+1)*num_rows)>=sizeof(arg)) {
        this_column=arg[i*num_rows..];
      } else {
        this_column=arg[i*num_rows..(i+1)*num_rows-1];
      }
      add_strings(this_column, widths[i], 0, new_line_width, widths[i]+gap);
    }
  }
  blank=repeat_string(pad_char, numbers[NUM_WIDTH]-total_width-num_columns*gap);
  for (i=0;i<num_rows;i++) {
    result[output_line_index+i]+=blank;
  }
  tidy_line_lengths();
}

private void do_integer(mixed arg) {
  error(typeof(arg)!=T_INT, "expecting an integer argument");
  add_strings(({arg+""}), numbers[NUM_WIDTH], 0, 0, numbers[NUM_WIDTH]);
  tidy_line_lengths();
}

private void do_float(mixed arg) {
  string *str;
  int int_part;
  int prec;

  error(typeof(arg)!=T_FLOAT, "expecting a float argument");
  str=({ "" });
  if (arg<0.0) {
    str[0]+="-";
    arg=-arg;
  }
  int_part=(int)floor(arg);
  str[0]+=int_part+".";
  arg=arg-(float)int_part;
  prec=numbers[NUM_PRECISION][0];
  if (prec==0) {
    prec=numbers[NUM_WIDTH]-strlen(str[0]);
  }
  arg=arg*pow(10.0, (float)prec);
  str[0]+=(int)arg;
  add_strings(str, numbers[NUM_WIDTH], 0, 0, numbers[NUM_WIDTH]);
  tidy_line_lengths();
}

private int get_number() {
  int number;
  int negative;

  number=0;
  if (format[posn]=='-') {
    posn++;
    negative=1;
  } else {
    negative=0;
  }
  while (format[posn]>='0' && format[posn]<='9') {
    number=number*10+format[posn]-'0';
    posn++;
  }
  if (negative) {
    return -number;
  } else {
    return number;
  }
}

string sprintf(string format_string, mixed args_array...) {
  int i;
  int arg_index;
  int done;
  int expecting;

  result=({ "" });
  format=format_string;
  args=args_array;
  posn=0;
  arg_index=0;
  err=0;
  output_line_index=0;
  while (!err && posn<strlen(format)) {
    switch (format[posn]) {
      case '%':
	if (!error(posn>=strlen(format), "unexpected end of format string")) {
	  if (format[posn+1]=='%') {
	    while (sizeof(result)<=output_line_index) {
	      result+=({ "" });
	    }
            result[output_line_index]+=format[posn..posn];
            for (i=output_line_index+1;i<sizeof(result);i++) {
              result[i]+=" ";
            }
            posn=posn+2;
	  } else {
	    numbers=({ 0, ({ }), 0 });
	    alignment=ALIGN_LEFT;
	    type=TYPE_NORMAL;
	    expecting=NUM_WIDTH;
	    done=0;
	    pad_char=" ";
	    posn++;
	    while (!err && !done) {
	      if (!error(posn>=strlen(format), "unexpected end of format string")) {
	        switch (format[posn]) {
	          case '0': case '1': case '2': case '3': case '4':
	          case '5': case '6': case '7': case '8': case '9': case '-':
	            if (!error(expecting==NUM_NOTHING, "unexpected number")) {
		      if (expecting==NUM_PRECISION) {
		        numbers[expecting]+=({ get_number() });
		      } else {
		        error(numbers[expecting]!=0, "repeated number");
	                numbers[expecting]=get_number();
		      }
	            }
	            expecting=NUM_NOTHING;
	            break;
	          case '*': 
	            if (!error(expecting==NUM_NOTHING, "unexpected '*'")) {
		      if (!error(arg_index>=sizeof(args), "too few arguments")) {
		        if (expecting==NUM_PRECISION) {
		          if (typeof(args[arg_index])==T_ARRAY) {
		            numbers[expecting]+=args[arg_index++];
		          } else {
		            error(typeof(args[arg_index])!=T_INT, "expecting an integer or array argument");
		            numbers[expecting]+=({ args[arg_index++] });
		          }
		        } else {
		          error(typeof(args[arg_index])!=T_INT, "expecting an integer argument");
		          error(numbers[expecting]!=0, "repeated number");
	                  numbers[expecting]=args[arg_index++];
		        }
		      }
		    }
		    posn++;
	            expecting=NUM_NOTHING;
	            break;
	          case '.': 
	            error(expecting!=NUM_NOTHING && expecting!=NUM_WIDTH, "missing number");
	            expecting=NUM_PRECISION; 
	            posn++; 
	            break;
	          case 'i': 
	            error(expecting!=NUM_NOTHING && expecting!=NUM_WIDTH, "missing number");
	            expecting=NUM_INDENT; 
	            posn++; 
	            break;
	          case 'p':
	            error(expecting!=NUM_NOTHING && expecting!=NUM_WIDTH, "missing number");
		    posn++;
		    if (!error(posn>=strlen(format), "unexpected end of format string")) {
		      pad_char=format[posn..posn];
		      posn++;
		      if (pad_char=="*") {
		        if (!error(arg_index>=sizeof(args), "too few arguments")) {
		          if (!error(typeof(args[arg_index])!=T_STRING, "expecting a string argument")) {
		            pad_char=args[arg_index++];
		          }
		        }
		      }
		    }
		    break;
	          case '>': 
	            error(expecting!=NUM_NOTHING && expecting!=NUM_WIDTH, "missing number");
	            error(alignment!=ALIGN_LEFT, "multiple alignments");
	            alignment=ALIGN_RIGHT; 
	            posn++;
	            break;
	          case '!': 
	            error(expecting!=NUM_NOTHING && expecting!=NUM_WIDTH, "missing number");
	            error(alignment!=ALIGN_LEFT, "multiple alignments");
	            alignment=ALIGN_CENTRE; 
	            posn++;
	            break;
	          case '|': 
	            error(expecting!=NUM_NOTHING && expecting!=NUM_WIDTH, "missing number");
	            error(alignment!=ALIGN_LEFT, "multiple alignments");
	            alignment=ALIGN_JUSTIFIED; 
	            posn++;
	            break;
	          case '=': 
	            error(expecting!=NUM_NOTHING && expecting!=NUM_WIDTH, "missing number");
	            error(type!=TYPE_NORMAL, "multiple formats");
	            type=TYPE_COLUMN; 
	            posn++; 
	            break;
	          case '#': 
	            error(expecting!=NUM_NOTHING && expecting!=NUM_WIDTH, "missing number");
	            error(type!=TYPE_NORMAL, "multiple formats");
	            type=TYPE_TABLE; 
	            posn++; 
	            break;
                  case 's': case 'a': case 'd': case 'f':
	            error(expecting!=NUM_NOTHING && expecting!=NUM_WIDTH, "missing number");
	            done=1;
	            break;
	          default:
	            error(1, "unknown format charcter "+format[posn..posn]);
	        }
	      }
	    }
	    if (sizeof(numbers[NUM_PRECISION])==0) {
	      numbers[NUM_PRECISION]=({ 0 });
	    }
            if (!error(arg_index>=sizeof(args), "too few arguments")) {
	      if (posn<strlen(format)) {
	        switch (format[posn++]) {
	          case 's': 
	            do_string(args[arg_index++]); 
	            break;
	          case 'a': 
	            do_array(args[arg_index++]); 
	            break;
	          case 'd': 
	            do_integer(args[arg_index++]); 
	            break;
	          case 'f': 
	            do_float(args[arg_index++]); 
	            break;
	        }
	      }
	    }
          }
	}
	break;
      case '\n':
	output_line_index=sizeof(result);
	result+=({ "" });
	posn++;
        break;
      default:
	while (sizeof(result)<=output_line_index) {
	  result+=({ "" });
	}
        result[output_line_index]+=format[posn..posn];
        for (i=output_line_index+1;i<sizeof(result);i++) {
          result[i]+=" ";
        }
        posn++;
    }
  }
  if (err) {
    return nil;
  } else {
    return implode(result, "\n");
  }
}
