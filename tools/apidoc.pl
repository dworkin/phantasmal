#!/usr/bin/perl -w

use strict;
use Cwd;

my $output_dir;

$output_dir = "./html";

my %priv_dirs;   # Privileged directories that define API funcs
my %func_names;  # Function names defined in various places

my %all_api_files;
my @the_api_files = `find . -name "*.api"`;

read_all_files(@the_api_files);
@the_api_files = ([]);   # Garbage collection

verify_format();
set_new_fields();

# For each /usr/XXX/blah directory, output its files
my ($dir, $dir_index, @dirs, $fileref);
@dirs = sort keys %priv_dirs;
foreach $dir (@dirs) {
    print "In directory $dir:\n";

    html_for_directory($dir, $dir_index, @{$priv_dirs{$dir}});
}


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
	      $last_line_ws = 1;
	  } elsif ($line =~ /^\s+/) {
	      $entry .= $line;
	      $last_line_ws = 0;
	  } else {
	      die "Unrecognized line: '$line'";
	  }
	  $filemap{$entname} = $entry;

      }
	close(FILE);
	$filemap{filename} = $file;
	print "Finished parsing file: '$file'\n";

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
	       =~ /\/(([a-zA-Z0-9]+\/)+)[a-zA-Z0-9]+\.c/) {
	    die "Filename '$fileref->{def_file}' doesn't parse in API file "
		. "'$fileref->{filename}'!";
	}
	unless(defined($priv_dirs{$1})) {
	    $priv_dirs{$1} = ([]);
	}
	$ref = $priv_dirs{$1};
	$priv_dirs{$1} = ([ @$ref, $fileref ]);

	unless($fileref->{NAME} =~ /^\s*([a-zA-Z0-9_]+)\s*-\s*(.*)$/) {
	    die "Unrecognized format for entry NAME, '$fileref->{NAME}'" .
		", file '$fileref->{filename}'!";
	}
	$fileref->{func_name} = $1;
	$fileref->{func_desc} = $2;

	$fileref->{prototype} = $fileref->{PROTOTYPE};

	$fileref->{called_by} = $fileref->{"CALLED BY"};

	$fileref->{desc} = $fileref->{DESCRIPTION};

	$fileref->{return} = $fileref->{"RETURN VALUE"};

	$fileref->{errors} = $fileref->{ERRORS};

	$fileref->{see_also} = $fileref->{"SEE ALSO"};
    }
}

sub html_for_directory {
    my (@filerefs, $fileref, $dir_index, $dir_name);

    $dir_index = shift;
    $dir_name = shift;
    @filerefs = @_;

    foreach $fileref (@filerefs) {
	print "  File: $fileref->{filename}\n";
	
    }

    html_for_dir($dir_name, $dir_index, @filerefs);
}

sub html_for_dir {
    my ($dir_name, $dir_index, @filerefs) = @_;

    
}
