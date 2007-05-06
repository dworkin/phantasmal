#!/usr/bin/perl -w

use strict;

use File::Find;
use File::Spec;
use FindBin;
use Cwd;

my $pagecreate;
$pagecreate = $FindBin::Bin . "/pagecreate.pl";

sub wanted {
    my ($filename, $justname, $pageheader, $pagefooter);
    $filename = $File::Find::name;
    $justname = $_;
    if(-d $justname) {
	$pageheader = File::Spec->catfile($justname, "pageheader.html");
	$pagefooter = File::Spec->catfile($justname, "pagefooter.html");
	if(-f $pageheader and -f $pagefooter) {
	    #print "Calling pagecreate on '$filename'...\n";
	    system("cd $justname; $pagecreate");
	    #print "Done!\n";
	}
    }
}

find \&wanted, ".";
