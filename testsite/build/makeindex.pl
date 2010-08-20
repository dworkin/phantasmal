#!/usr/bin/perl -w

use strict;

my @sections;
my $buffer;
my $section_line;
my $outfilename = shift @ARGV;
my $depth = shift @ARGV;

# generate the index in the sidebar
my $index;
open(FILE, "<pageindex.full")
	or die "Can't open index.full: $!";
$buffer = join("", <FILE>);
my @index_lines = split("\n", $buffer);
close FILE;

my $line;

my $indent;
my $old_indent = 0;

$index .= "<ul>\n";

foreach $line (@index_lines) {
	my $word;
	
	$indent = 0;
	my $name;
	my $label;
	my $skipflag = 0;
	
	$line =~ s/([^ ]+) (.*)/$2/;
	$word = $1;
	
	if ($word eq "Depth") {
		$skipflag = 1;
	} elsif ($word eq "Section") {
		$line =~ m/\"([^\"]*)\"/;
		push @sections, $1;
		$skipflag = 1;
	} elsif ($word eq "Entry") {
		$line =~ m/([0-9]*) *\"([^\"]*)\" *\"([^\"]*)\"/;
		$indent = $1;
		$name = $2;
		$label = $3;
	}
	
	unless ($skipflag) {
		my $indexline = "";
		
		while ($old_indent < $indent) {
			$index .= "<ul>\n";
			$old_indent++;
		}
		
		while ($old_indent > $indent) {
			$index .= "</ul>\n";
			$old_indent--;
		}
		
		$indexline .= "<li>";
		
		if ($name eq $outfilename) {
			$indexline .= $label;
		} else {
			$indexline .= "<a href=\"" . $name . "\">" . $label . "</a>";
		}
		
		$indexline .= "</li>\n";
		
		$index .= $indexline;
	}
}

$indent = 0;

while ($old_indent > $indent) {
	$index .= "</ul>\n";
	$old_indent--;
}

$index .= "</ul>\n";

print $index;
