#!/usr/bin/perl -w

use strict;
use Cwd;

my @lines;
my $line;

open(CONFIGFILE, "testgame.dgd") or die "Can't open testgame.dgd: $!";
@lines = <CONFIGFILE>;

open(OUTFILE, ">new_testgame.dgd") or die "Can't open new_testgame.dgd: $!";

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
system("mv new_testgame.dgd testgame.dgd");

print "Installation done.  If you get an error saying to run the install\n";
print "script, you should type \"mv new_testgame.dgd testgame.dgd\" to fix\n";
print "it.  Otherwise, you're good to go.\n\n";

print "To run the MUD, try typing \"./start_mud.pl\".  If that doesn't work\n";
print "then type \"perl start_mud.pl\".  If you don't have Perl installed,\n";
print "you'll need to either install it or learn to run the MUD by hand.\n";
print "It's pretty easy either way.  Enjoy!\n";
