#!/usr/bin/perl -w

use strict;

my @read_files;
my %ordered_content;
my $read_title;
my $read_section;
my $read_top;
my $read_error;
my $read_sequence;
my $top_sequence = 0;

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
	$read_sequence = "";
	
	if ($read_error) {
		return;
	}
	
	$contents = join ("", <FILE>);
	close FILE;
	
	$contents =~ m/\@\@TITLE ([^@]*)\@\@/
		and $read_title = $1;

	if ($contents =~ m/\@\@SEQUENCE ([^@]*)\@\@/) {
		$read_sequence = $1;
		
		if ($top_sequence < $read_sequence) {
			$top_sequence = $read_sequence;
		}
	}
	
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
			
			if (!($read_sequence eq "")) {
				$ordered_content{$read_sequence}
					= "File \"" . $test . "\" \"" . $read_title . "\"\n";
			} else {
				$index .= "File \"" . $test . "\" \"" . $read_title . "\"\n";
			}
		}
	}
}

my $sequence = 0;

while ($sequence <= $top_sequence) {
	if (exists $ordered_content{$sequence}) {
		$index .= $ordered_content{$sequence};
	}
	$sequence++;
}

print $index;
