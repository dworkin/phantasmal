#!/usr/bin/perl -w

use strict;
use Cwd;

my $output_path = "./html";

my %priv_objs;   # Privileged LPC objects that define API funcs

my %all_api_files;
my @the_api_files = `find . -name "*.api"`;

my %obj_filenames;  # Output files for particular object names

my %func_names;     # Function names defined in various places

#################################################
# Impromptu 'main'-type code

read_all_files(@the_api_files);
@the_api_files = ([]);   # Garbage collection

verify_format();
set_new_fields();

unless(-d $output_path) {
    mkdir($output_path);
}

# Tmp variables and an object list
my ($obj, $obj_index, @objs, $fileref, $file_index);
@objs = sort keys %priv_objs;

# For each API entry, index it
$obj_index = 1;
$file_index = 1;
foreach $obj (@objs) {
    foreach $fileref (@{$priv_objs{$obj}}) {
	$fileref->{output_html} = "file_idx_${obj_index}_$file_index.html";
	$func_names{$fileref->{func_name}} = $fileref->{output_html};
	$file_index++;
    }

    $obj_filenames{$obj} = "obj_idx$obj_index.html";
    $obj_index++;
}

# For each /usr/XXX/blah/foo.c object, output its files
foreach $obj (@objs) {
    print "* Object $obj\n";
    #print join(", ", map {$_->{func_name}} @{$priv_objs{$obj}}) . "\n";

    html_for_object($obj, @{$priv_objs{$obj}});
}

html_for_index();


##############################################################
# Helper Functions
##############################################################

sub read_all_files {
    #Open each .api file in turn
    my @api_files = @_;

    my ($file, $line, @lines);
    foreach $file (@api_files) {
	my %filemap;
	my ($entry, $entname, $last_line_ws);

	chomp $file;
	open(FILE, $file) or die "Can't open '$file': $!";

	# Read each file in its entirety
	@lines = <FILE>;
	$last_line_ws = 1;
	$entry = "";
      LINE: foreach $line (@lines) {
	  # Parse the files.  Each entry should start with an all-caps
	  # keyword on a line of its own.  Each later line should start
	  # with at least one whitespace character.

	  if($last_line_ws && $line =~ /^([A-Z]+(\s+[A-Z]+)*)\s*$/) {
	      die "First line must be all-caps characters!"
		  unless (defined($entname) or $entry eq "");
	      if(defined($entname)) {$filemap{$entname} = $entry;}

	      $entname = $1;
	      $entry = "";
	      $last_line_ws = 0;
	  } elsif ($line =~ /^\s*$/) {
	      $entry .= $line;
	      $last_line_ws = 1;
	  } elsif ($line =~ /^\s+/) {
	      $entry .= $line;
	      $last_line_ws = 0;
	  } else {
	      die "Unrecognized line: '$line'";
	  }

      }
	$filemap{$entname} = $entry;
	close(FILE);
	$filemap{filename} = $file;
	#print "Finished parsing file: '$file'\n";

	$all_api_files{$file} = \%filemap;
    }
}


sub verify_format {
    my $file;

    #
    # Now that the files have been read and roughly parsed, check that
    # they contain all necessary fields.
    #
    foreach $file (keys %all_api_files) {
	my $fileref;

	$fileref = $all_api_files{$file};

	unless(defined($fileref->{FILE})) {
	    die "File $fileref->{filename} doesn't define field FILE!";
	}
	unless(defined($fileref->{NAME})) {
	    die "File $fileref->{filename} doesn't define field NAME!";
	}
	unless(defined($fileref->{PROTOTYPE})) {
	    die "File $fileref->{filename} doesn't define field PROTOTYPE!";
	}
	unless(defined($fileref->{"CALLED BY"})) {
	    die "File $fileref->{filename} doesn't define field CALLED BY!";
	}
	unless(defined($fileref->{"DESCRIPTION"})) {
	    die "File $fileref->{filename} doesn't define field DESCRIPTION!";
	}
	unless(defined($fileref->{"RETURN VALUE"})) {
	    die "File $fileref->{filename} doesn't define field RETURN VALUE!";
	}
	unless(defined($fileref->{"ERRORS"})) {
	    die "File $fileref->{filename} doesn't define field ERRORS!";
	}
	unless(defined($fileref->{"SEE ALSO"})) {
	    die "File $fileref->{filename} doesn't define field SEE ALSO!";
	}
    }
}


sub set_new_fields {
    my ($file, $ref);

    #
    # Set the information we're actually going to use
    #
    foreach $file (keys %all_api_files) {
	my $fileref;

	$fileref = $all_api_files{$file};

	unless($fileref->{FILE} =~ /^\s*(.*)\s*$/) {
	    die "Unrecognized format for entry FILE, '$fileref->{FILE}'"
		. ", file '$fileref->{filename}'!";
	}
	$fileref->{def_file} = $1;

	unless($fileref->{def_file}
	       =~ /\/(([a-zA-Z0-9]+\/)+)([a-zA-Z0-9]+)\.c/) {
	    die "Filename '$fileref->{def_file}' doesn't parse in API file "
		. "'$fileref->{filename}'!";
	}
	my $filename = '/' . $1 . "$3";
	unless(defined($priv_objs{$filename})) {
	    $priv_objs{$filename} = ([]);
	}
	$ref = $priv_objs{$filename};
	$priv_objs{$filename} = ([ @$ref, $fileref ]);

	unless($fileref->{NAME} =~ /^\s*([a-zA-Z0-9_]+)\s*-\s*(.*)$/s) {
	    die "Unrecognized format for entry NAME, '$fileref->{NAME}'" .
		", file '$fileref->{filename}'!";
	}
	$fileref->{func_name} = $1;
	$fileref->{func_desc} = $2;

	$fileref->{prototype} = $fileref->{PROTOTYPE};

	$fileref->{called_by} = $fileref->{"CALLED BY"};

	$fileref->{desc} = $fileref->{DESCRIPTION};
	$fileref->{desc} =~ s/\n\n/<\/p>\n<p>/g;

	$fileref->{return} = $fileref->{"RETURN VALUE"};
	$fileref->{return} =~ s/\n\n/<\/p>\n<p>/g;

	$fileref->{errors} = $fileref->{ERRORS};

	$fileref->{see_also} = $fileref->{"SEE ALSO"};
    }
}

################################# html_for_object ####################

# This outputs an HTML file not only for the object itself, but for
# all APIs within the object

sub html_for_object {
    my (@filerefs, $fileref, $obj_name);

    $obj_name = shift;
    @filerefs = sort {$a->{func_name} cmp $b->{func_name}} @_;

    foreach $fileref (@filerefs) {
	#print "  File: $fileref->{filename}\n";
	html_for_file($fileref);
    }

    html_for_obj($obj_name, @filerefs);
}

################################# html_for_file ####################

# HTML for specific API file entries
sub html_for_file {
    my $fileref = shift;

    my $tmpname = ">$output_path/$fileref->{output_html}";

    open(FILE, $tmpname) or die "Can't open file $tmpname: $!";

    print FILE <<"EOF";

<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">
<html>
  <head>
    <title> Phantasmal API Documentation </title>
    <style type="text/css">
    <!--
      p {font-family: serif, font-weight: normal; color: black;
         font-size: 12pt }
      h3 {font-family: serif; font-weight: bold; color: #FF20FF;
          font-size: 18pt}
      h4 {font-family: serif; font-weight: bold; color: #0000FF;
          font-size: 16pt}
     -->
    </style>
  </head>

  <body text="#000000" bgcolor="#DDDDDD" link="#0000EF" vlink="#51188E"
        alink="#FF0000">

    <h3 align="center"> API Function: $fileref->{func_name} </h3>

    <dl>
      <dt> Summary: </dt>
      <dd> $fileref->{func_desc} <br/> <br/> </dd>

      <dt> Defined In LPC Object: </dt>
      <dd> $fileref->{def_file} <br/> <br/> </dd>

      <dt> Prototype: </dt>
      <dd> $fileref->{prototype} <br/> <br/> </dd>

      <dt> Can Be Called By: </dt>
      <dd> $fileref->{called_by} <br/> <br/> </dd>

      <dt> Description: </dt>
      <dd> <p> $fileref->{desc} </p> </dd>

      <dt> Return Value:</dt>
      <dd> <p>$fileref->{return} </p> </dd>

      <dt> Errors:</dt>
      <dd> $fileref->{errors} <br/> <br/> </dd>

      <dt> See Also:</dt>
EOF
    ;

    # "See also" entry
    print FILE "      <dd> ";
    my @see_also = split(/,/, $fileref->{see_also});

    @see_also = map {s/^\s+//; $_} map {s/\s+$//; $_} @see_also;
    @see_also = map {s/\s+/ /; $_} @see_also;

    @see_also = map {
	if(defined($func_names{$_})) {
	    "<a href=\"" . $func_names{$_} . "\"> $_ </a>";
	} else { $_ }
    } @see_also;
    print FILE join(",\n      ", @see_also);

    print FILE "</dd>\n";

    print FILE <<"EOF";
    </dl>

    <a href="http://sourceforge.net">
      <img src="http://sourceforge.net/sflogo.php?group_id=48659&type=6"
           width="210" height="62" border="0"
           alt="SourceForge.net Logo"></a>
  </body>
</html>

EOF
    ;
    close(FILE);
}

################################# html_for_obj ####################

# HTML for object entry, for objects like /usr/System/obj/objectd.c.
# This doesn't output all the API entries, html_for_object does
# that.
#
sub html_for_obj {
    my ($obj_name, @filerefs) = @_;
    my ($tmpname, $fileref);

    $tmpname = ">$output_path/$obj_filenames{$obj_name}";
    open(FILE, $tmpname)
	or die "Can't open obj index file $tmpname: $!";

    print FILE <<"EOF";
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">
<html>
  <head>
    <title> Phantasmal API Documentation </title>
    <style type="text/css">
    <!--
      p {font-family: serif, font-weight: normal; color: black;
         font-size: 12pt }
      h3 {font-family: serif; font-weight: bold; color: #3000FF;
          font-size: 16pt}
     -->
    </style>
  </head>

  <body text="#000000" bgcolor="#DDDDDD" link="#0000EF" vlink="#51188E"
        alink="#FF0000">

    <h3 align="center"> APIs in $obj_name </h3>

    <ul>
EOF
    ;

    my @tmprefs = sort {
	$a->{func_name} cmp $b->{func_name};
    } @filerefs;

    foreach $fileref (@tmprefs) {
	print FILE "      <li><a href=\"$fileref->{output_html}\">"
	    . $fileref->{prototype} . "</a></li>\n";
    }

    print FILE <<"EOF";
    </ul>

    <a href="http://sourceforge.net">
      <img src="http://sourceforge.net/sflogo.php?group_id=48659&type=6"
           width="210" height="62" border="0"
           alt="SourceForge.net Logo"></a>
  </body>
</html>

EOF

    close(FILE);
}

################################# html_for_index ####################

# This makes an index.html file for all objects.
#
sub html_for_index {
    my ($obj_name);

    open(FILE, ">$output_path/index.html") or die "Can't open index.html: $!";

    print FILE <<"EOF";
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">
<html>
  <head>
    <title> Phantasmal API Documentation </title>
    <style type="text/css">
    <!--
      p {font-family: serif, font-weight: normal; color: black;
         font-size: 12pt }
      h3 {font-family: serif; font-weight: bold; color: #3000FF;
          font-size: 16pt}
     -->
    </style>
  </head>

  <body text="#000000" bgcolor="#DDDDDD" link="#0000EF" vlink="#51188E"
        alink="#FF0000">

    <h3 align="center"> Phantasmal API Objects </h3>

EOF
    ;

    my (%userbucket, $user);
    $userbucket{"-"} = ([ ]);

    for $obj_name (sort keys %obj_filenames) {
	if($obj_name =~ /^\/usr\/([A-Za-z0-9_]+)\//) {
	    $user = $1;

	    # Add to user bucket
	    if(defined($userbucket{$user})) {
		$userbucket{$user} = ([ @{$userbucket{$user}}, $obj_name ]);
	    } else {
		$userbucket{$user} = ([ $obj_name ]);
	    }
	} else {
	    # Add to 'userless' bucket
	    $userbucket{"-"} = ([ @{$userbucket{"-"}}, $obj_name ]);
	}
    }

    foreach $user (keys %userbucket) {
	print_index_user($user, @{$userbucket{$user}});
    }

    print FILE <<"EOF";

    <a href="http://sourceforge.net">
      <img src="http://sourceforge.net/sflogo.php?group_id=48659&type=6"
           width="210" height="62" border="0"
           alt="SourceForge.net Logo"></a>
  </body>
</html>

EOF

    close(FILE);
}

sub print_index_user {
    my $user = shift;
    my @filenames = @_;

    my $obj_name;

    print FILE "    <b> Files under /usr/$user </b>\n";
    print FILE "    <ul>\n";
    for $obj_name (sort keys %obj_filenames) {
	print FILE "      <li> <a href=\"$obj_filenames{$obj_name}\">"
	    . "$obj_name </a> </li>\n";
    }
    print FILE "    </ul>\n";
}
