#!/usr/bin/perl -w

use strict;

my @read_files;
my $read_title;
my $read_section;
my $read_top;
my $read_error;

sub read_dir
{
	my $readme = shift;
	
	@read_files = split(" ", `cd $readme; ls *.base.html`);
}

sub read_file
{
	my $readme = shift;
	my $contents;
	
	$read_top = 0;
	$read_title = "Untitled document ($readme)";
	$read_error = !open (FILE, "<" . $readme);
	
	if ($read_error) {
		return;
	}
	
	$contents = join ("", <FILE>);
	close FILE;
	
	$contents =~ m/\@\@TITLE ([^@]*)\@\@/
		and $read_title = $1;
	
	if ($contents =~ m/\@\@SECTION ([^@]*)\@\@/) {
		$read_section = $1;
	} else {
		$read_section = $read_title;
	}
}

read_file("index.base.html");
read_dir(".");

my $test;
my @dir_list = split(" ", $ENV{"DIRS"});
my @file_list = @read_files;

my $index = "Section \"" . $read_section . "\"\n";

read_file("index.base.html");

$index .= "Root \"" . $read_section . "\"\n";

foreach $test (@dir_list) {
	read_file($test . "/index.base.html");
	
	unless ($read_error) {
		$index .= "Directory \"" . $test . "\" \"" . $read_section . "\"\n";
	}
}

foreach $test (@file_list) {
	unless ($test eq "index.base.html") {
		read_file($test);

		unless ($read_error) {
			$test =~ s/.base//;
			$index .= "File \"" . $test . "\" \"" . $read_title . "\"\n";
		}
	}
}

print $index;
