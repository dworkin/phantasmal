#!/usr/bin/perl -w

use strict;

my @sections;
my $buffer;
my $section_line;
my $outfilename = shift @ARGV;
my $depth = shift @ARGV;

# generate the index in the sidebar
my $index = "<table summary=\"index\">\n";
open(FILE, "<pageindex.full")
	or die "Can't open index.full: $!";
$buffer = join("", <FILE>);
my @index_lines = split("\n", $buffer);
close FILE;

my $line;

my $indent;
my $old_indent = 0;

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
		
		$old_indent = $indent;
		
		$indexline .= "<tr>" .
			("<td><img src=\"\@\@WEBDIR\@\@images/16x1_transparent.png\" alt=\"\"/></td>" x $indent) .
			"<td colspan=\"" .
			(1 + $depth - $indent) . "\">";
		
		if ($name eq $outfilename) {
			$indexline .= $label;
		} else {
			$indexline .= "<a href=\"" . $name . "\">" . $label . "</a>";
		}
		
		$indexline .= "</td></tr>\n";
		
		$index .= $indexline;
	}
}

$index .= "</table>\n";

print $index;