#!/usr/bin/perl -w

# This script exists to automate setting up a Phantasmal unpack-and-go
# distribution.  The idea is that you have a code directory, under which
# are your dgd and phantasmal directories.  In your phantasmal directory
# are the mudlib, testgame and tools directories, and possibly others
# like website as well.  You've synced those from CVS.  Then you run
# this script, which will very painlessly set up an unpack-and-go distrib
# from your current CVS versions of Phantasmal and Testgame, and from
# whatever version of DGD you have in the specified directory.
#
# It's up to you, naturally, to make sure that these things are all
# compatible.  But you can at least let the script package them up for
# you.

# TODO:
# Figure out what to do if DGD isn't already built.  Normally, there
# should be a dgd/bin/driver binary.

use strict;
use Cwd;
use File::Spec;

my $input;
my $builderdir = cwd();
my ($phantasmaldir, $driverdir, $testgamedir);
my $outdir = "phantest";

print "Current dir is $builderdir.\n";

# Change dir to .../phantasmal/... (we hope)
chdir "..";

sub check_directory {
    my ($firstdir, $filename, $en_name) = @_;
    my $cpath;

    if(-f $firstdir . "/" . $filename) {
	do {
	    $cpath = cwd() . "/" . $firstdir;
	    print "Use $cpath as the $en_name directory (y/n)? ";
	    $input = <STDIN>;
	    chomp $input;
	    $input = lcfirst(substr($input, 0, 1));
	} while ($input ne 'n' and $input ne 'y');

	if(substr($input, 0) eq 'y') {
	    return $cpath;
	}
    }

    while(1) {
	print "What path should I use for the $en_name directory? ";
	$input = <STDIN>;
	chomp $input;
	if(-f $input . "/" . $filename) {
	    return $input;
	} else {
	    print "That doesn't appear to be a $en_name directory.\n"
		. "Enter a valid directory, or \"quit\" to abort.\n";
	}
	if($input =~ /quit/i) {
	    die "Script aborted at user request";
	}
    }
}

$phantasmaldir = check_directory("mudlib", "phantasmal.dgd",
				 "Phantasmal MUDLib");

$testgamedir = check_directory("testgame", "testgame.dgd",
			       "Test Game");

chdir("..");
$driverdir = check_directory("dgd", "src/dgd.c",
			     "DGD Driver");
chdir("phantasmal");


my $kerneldir = $driverdir . "/mud";

# Okay, we should have all the paths set up correctly.
while(-e "$outdir") {
    print "Delete old bundle? (y/n) ";
    $input = <STDIN>;
    $input = lcfirst(substr($input, 0, 1));
    if($input eq "y") {
	system("rm -rf $outdir");
	last;
    } elsif ($input eq "n") {
	last;
    }
}

unless(-f "$driverdir/bin/driver") {
    # TODO:  just build DGD here if we need to
    die "You haven't built DGD yet!";
}

print "Copying TestGame from $testgamedir...\n";
system("cp -r $testgamedir $outdir");
print "Copying DGD from $driverdir...\n";
system("cp -r $driverdir $outdir/dgd");
print "Copying Phantasmal from $phantasmaldir...\n";
system("cp -r $phantasmaldir $outdir/phantasmal");

print "Moving Phantasmal user directories...\n";
system("rm -rf $outdir/usr/System $outdir/usr/common");
system("mv $outdir/phantasmal/usr/System $outdir/usr/System");
system("mv $outdir/phantasmal/usr/common $outdir/usr/common");

print "Moving Kernel Library...\n";
system("rm -rf $outdir/kernel/*");
system("mv $outdir/dgd/mud/kernel/data $outdir/kernel/data");
system("mv $outdir/dgd/mud/kernel/sys $outdir/kernel/sys");
system("mv $outdir/dgd/mud/kernel/obj $outdir/kernel/obj");
system("mv $outdir/dgd/mud/kernel/lib $outdir/kernel/lib");

print "Moving include directories...\n";
system("rm -rf $outdir/include/kernel $outdir/include/phantasmal");
system("mv $outdir/phantasmal/include/phantasmal $outdir/include/");
system("mkdir $outdir/include/kernel");

print "Moving Kernel Library headers...\n";
system("mv $outdir/dgd/mud/include/kernel/*.h $outdir/include/kernel/");

print "Moving DGD binary directory...\n";
system("mv $outdir/dgd/bin $outdir");

print "Cleaning Phantasmal & DGD dirs...\n";
system("rm -rf $outdir/phantasmal");
system("rm -rf $outdir/dgd");

print "Cleaning cvs dirs...\n";
system("find $outdir -name CVS -type d -print > /tmp/distrib_tmp.txt");
system("rm -rf `cat /tmp/distrib_tmp.txt`");
system("rm -f $outdir/README $outdir/INSTALL");
system("rm -f $outdir/usr/README $outdir/kernel/README $outdir/include/README");

print "Moving bundle-specific files...\n";
system("mv $outdir/bundled/* $outdir/");
system("rmdir $outdir/bundled");

print "*** Finished! ***\n";
