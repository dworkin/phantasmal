#!/usr/bin/perl -w

use strict;
use Cwd;

my @lines;
my $line;

open(CONFIGFILE, "./testgame.dgd") or die "Can't open testgame.dgd: $!";
@lines = <CONFIGFILE>;

open(OUTFILE, "./new_testgame.dgd") or die "Can't open new_testgame.dgd: $!";

while(@lines) {
    $line = shift @lines;
    if($line =~ /directory\s+=/) {
	print OUTFILE 'directory = "' . cwd() . "\"\n";
    } else {
	print OUTFILE $line;
    }
}

close(CONFIGFILE);
close(OUTFILE);

print "system(\"mv new_testgame.dgd testgame.dgd\");\n";
