#!/usr/bin/perl -w

use strict;
use Cwd;

my @lines;
my $line;

my $filename = "testgame.dgd";

open(CONFIGFILE, $filename) or die "Can't open $filename: $!";
@lines = <CONFIGFILE>;

open(OUTFILE, ">new_$filename") or die "Can't open new_$filename: $!";

while(@lines) {
    $line = shift @lines;
    if($line =~ /directory\s+=/) {
	print OUTFILE 'directory = "' . cwd() . "\";\n";
    } else {
	print OUTFILE $line;
    }
}

close(CONFIGFILE);
close(OUTFILE);

print "New .dgd file created.  Copying it into place.\n";
system("mv new_$filename $filename");

print "Installation done.  If you get an error saying to run the install\n";
print "script, you should type \"mv new_$filename $filename\" to fix\n";
print "it.  Otherwise, you're good to go.\n\n";

print "To run the MUD, try typing \"./start_mud\".  If that doesn't work\n";
print "then type \"bin\\driver $filename\" on Windows, or the same thing\n";
print "with a slash instead of a backslash on Unix.\n";
