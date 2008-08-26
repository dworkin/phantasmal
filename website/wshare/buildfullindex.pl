#!/usr/bin/perl -w

use strict;

my $buffer = "";
my $outbuffer = "";
my $watchword;
my $depth = 0;

my $header = "";
my $middle = "";
my $footer = "";
my $hitflag = 0;

my $read_keyword;
my $read_label;
my $read_name;
my $read_level;
my $read_indent;

my @upindex_lines = ();
my @index_lines;
my @sections = ();

my $index_buffer;

open FILE, "<pageindex.local" or die "Cannot open local base index: $!";
$buffer = join("", <FILE>);
close FILE;
@index_lines = split("\n", $buffer);

if (@ARGV) {
	$watchword = shift @ARGV;
	open FILE, "<../pageindex.full" or die "Cannot open parent's full index: $!";
	$buffer = join("", <FILE>);
	close FILE;
	
	@upindex_lines = split("\n", $buffer);
}

my $line;

foreach $line (@upindex_lines) {
	my $word;
	
	my $indent;
	my $name;
	my $label;
	my $skipflag = 0;
	
	$line =~ s/([^ ]+) (.*)/$2/;
	$word = $1;
	
	if ($word eq "Depth") {
		$depth = $2 + 1;
		$skipflag = 1;
	} elsif ($word eq "Section") {
		$line =~ m/\"([^\"]*)\"/;
		push (@sections,$1);
		$skipflag = 1;
	} elsif ($word eq "Entry") {
		$line =~ m/([0-9]*) *\"([^\"]*)\" *\"([^\"]*)\"/;
		$indent = $1;
		$name = $2;
		$label = $3;
	}

	if (not $skipflag and $name eq $watchword . "/index.html") {
		$hitflag = 1;
		$skipflag = 1;
	}
	
	unless ($skipflag) {
		my $outline;
		
		$outline = "Entry " . $indent . " \"../" .
			$name . "\" \"" .
			$label . "\"\n";

		unless ($hitflag) {
			$header .= $outline;
		} else {
			$footer .= $outline;
		}
	}
}

foreach $line (@index_lines) {
	my $word;
	my $name;
	my $label;
	my $indent;
	my $skipflag = 0;
	
	$line =~ s/([^ ]*) *(.*)/$2/;
	$word = $1;

	if ($word eq "Root") {
		$indent = 0;
		$line =~ m/\"([^\"]*)\"/;
		$name = "index.html";
		$label = $1;
	} elsif ($word eq "Section") {
		$line =~ m/\"([^\"]*)\"/;
		push (@sections,$1);
		$skipflag = 1;
	} elsif ($word eq "Directory") {
		$indent = 1;
		$line =~ m/\"([^\"]*)\" *\"([^\"]*)\"/ or die "Regex match failure";
		$name = $1 . "/index.html";
		$label = $2;
	} elsif ($word eq "File") {
		$indent = 1;
		$line =~ m/\"([^\"]*)\" *\"([^\"]*)\"/ or die "Regex match failure";
		$name = $1;
		$label = $2;
	} else {
		die "Malformed line";
	}
	
	unless ($skipflag) {
		$middle .= "Entry " . ($depth + $indent) . " \"" .
			$name . "\" \"" .
			$label . "\"\n";
	}
}

print "Depth " . $depth . "\n";

my $entry;

while (@sections) {
	print "Section \"" . shift(@sections) . "\"\n";
}

print $header;
print $middle;
print $footer;
