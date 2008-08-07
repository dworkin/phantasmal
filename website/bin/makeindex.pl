#!/usr/bin/perl -w

use strict;

# arguments:
# 0: name of file to generate index for
# 1: webroot prefix
# 2: depth

my $index;	# the current index
my $subindex;	# the current subindex
my $level = 0;	# current level
my $prefix = "";

my $header = "";
my $footer = "";

my $begin = "";
my $continue = "";
my $end = "";

my $linebegin = "<table summary=\"index entry\"><tr>";
my $lineend = "</tr></table>";
my $pretab = "<td><img src=\"\@\@WEBDIR\@\@images/index_arrow.png\" /></td>";
my $posttab = "";
my $start = "<td>";
my $stop = "</td>";

my $filename = $ARGV[0];
my $depth = $ARGV[1];
my $prev;

# filled in by the get_config routine
my @read_dirs;
my @read_files;
my $read_title;
my $read_section;

# our own information
my @dirs;
my @files;
my $section;
my $title;

sub read_dir
{
	my $readme = shift;
	
	@read_files = split(" ", `cd $readme; ls *.base.html`);
}

sub read_file
{
	my $readme = shift;
	my $contents;
	
	$read_title = "";
	$read_section = "";
	@read_dirs = ();
	
	open (FILE, "<" . $readme)
		or die "Can't scan file: $!";
	
	$contents = join ("", <FILE>);
	close FILE;
	
	$contents =~ m/\@\@TITLE ([^@]*)\@\@/
		and $read_title = $1;
	$contents =~ m/\@\@SECTION ([^@]*)\@\@/
		and $read_section = $1;
	$contents =~ m/\@\@DIRS ([^@]*)\@\@/
		and @read_dirs = split(" ", $1);
	
	$read_section eq "" and $read_section = $read_title;
}

my @path = split("/", `pwd`);
push @path, $filename;

for (;$depth >= 0 ;) {
	my $test;
	
	$prefix = ("../" x $level);

	read_dir $prefix . ".";
	read_file $prefix . "index.base.html";
	
	$prev = $filename;
	$filename = pop @path;
	chomp $filename;

	@files = @read_files;
	@dirs = @read_dirs;
	
	$subindex = $index;
	$index = "";

	# Directories
	
	$index = "$begin\n";
	
	foreach $test (@dirs) {
		read_file $prefix . "$test/index.base.html";
		
		if ($test eq $filename and $level == 1 and $prev eq "index.base.html") {
			$index .= "\@\@PREFIX\@\@\@\@PRETAB\@\@" . "$start"
				. $read_section . "$stop\@\@POSTTAB\@\@\@\@POSTFIX\@\@\n";
		} else {
			$index .= "\@\@PREFIX\@\@\@\@PRETAB\@\@" . "$start<a href=\"" . $prefix . $test . "/index.html\">"
				. $read_section . "</a>$stop\@\@POSTTAB\@\@\@\@POSTFIX\@\@\n";
		}
		
		if (($test eq $filename) and ($level > 0)) {
			$index .= "$begin\n";
			$subindex =~ s/^\@\@PREFIX\@\@\@\@PRETAB\@\@/\@\@PREFIX\@\@\@\@PRETAB\@\@\@\@PRETAB\@\@/mg;
			$subindex =~ s/\@\@POSTTAB\@\@\@\@POSTFIX\@\@$/\@\@POSTTAB\@\@\@\@POSTTAB\@\@\@\@POSTFIX\@\@/mg;
			$index .= $subindex;
			$index .= "$end\n";
		}
	}
	
	if (@dirs and @files) {
		$index .= "$continue";
	}
	
	# Files
	foreach $test (@files) {
		my $out = $test;
		
		if ($test ne "index.base.html") {
			$out =~ s/.base.html/.html/;
			read_file $prefix . $test;
		
			if ($test eq $filename and $level == 0) {
				$index .= "\@\@PREFIX\@\@\@\@PRETAB\@\@" . "$start" . $read_title . "$stop\@\@POSTTAB\@\@\@\@POSTFIX\@\@\n";
			} else {
				$index .= "\@\@PREFIX\@\@\@\@PRETAB\@\@" . "$start<a href=\"" . ("../" x $level)
					. $out . "\">" . $read_title
					. "</a>$stop\@\@POSTTAB\@\@\@\@POSTFIX\@\@\n";
			}
		}
	}

	$index .= "$end\n";
	
	$subindex = $index;
	
	$level++;
	$depth--;
}

$level--;
$prefix = "../" x $level;

read_file $prefix . "index.base.html";

$index = "$header\n";
$index .= "$begin\n";

if ($level == 0 and $prev eq "index.base.html") {
	$index .= "\@\@PREFIX\@\@$start" . $read_section . "$stop\@\@POSTFIX\@\@\n";
} else {
	$index .= "\@\@PREFIX\@\@$start<a href=\"" . $prefix . "index.html\">" . $read_section . "</a>$stop\@\@POSTFIX\@\@\n";
}

$index .= $subindex;

$index =~ s/\@\@PRETAB\@\@/$pretab/mg;
$index =~ s/\@\@POSTTAB\@\@/$posttab/mg;
$index =~ s/\@\@PREFIX\@\@/$linebegin/mg;
$index =~ s/\@\@POSTFIX\@\@/$lineend/mg;

$index .= "$end\n";
$index .= "$footer\n";



print $index;
