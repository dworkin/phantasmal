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

use strict;
use File::Spec;
use Cwd;

my $input;
#my $builderdir = File::Spec->curdir();
my $builderdir = cwd();
my ($phantasmaldir, $driverdir, $testgamedir);
my $outdir = "game_bundle";

print "Current dir is $builderdir.\n";

# Change dir to parent, where (we hope) other Phantasmal modules will also
# have been checked out.
chdir ".." || die "Can't change to parent dir!";

sub check_directory {
    my ($dirlistref, $filename, $en_name) = @_;
    my ($cpath, $dirname);
    my @dirnames = @$dirlistref;

    foreach $dirname (@dirnames) {
	$cpath = File::Spec->catfile($builderdir, $dirname);
	$cpath = File::Spec->canonpath($cpath);
	if(-f File::Spec->catfile($cpath, $filename)) {
	    do {
		print "Use $cpath as the $en_name directory [Y/n]? ";
		$input = <STDIN>;
		chomp $input;
		if($input eq "") { $input = "y"; }
		$input = lcfirst(substr($input, 0, 1));
	    } while ($input ne 'n' and $input ne 'y');

	    if(substr($input, 0) eq 'y') {
		return $cpath;
	    }
	} else {
	    #print "Not finding file '$filename' in '$cpath'.\n";
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

$phantasmaldir = check_directory([".", "mudlib", "phantasmal", "../mudlib",
                                  "../phantasmal"],
				 "phantasmal.dgd",
				 "Phantasmal MUDLib");

$testgamedir = check_directory([".", "testgame", "mudlib", "phantasmal",
                                "../testgame", "../mudlib", "../phantasmal"],
			       "usr/game/obj/user.c",
			       "Bundled Game");

$driverdir = check_directory([".", "dgd", "../dgd", "../../dgd", "~/dgd"],
			     "src/dgd.c",
			     "DGD Driver");


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
	print "(Overwriting old bundle)\n";
	last;
    }
}

unless(-f File::Spec->catfile($driverdir, "bin", "driver")) {
    # TODO:  just build DGD here if we need to
    print "Looking for '" . File::Spec->catfile($driverdir, "bin", "driver")
	. "'\n";
    print "No DGD driver!  Configure src/Makefile and 'make; make install'\n";
    die "You haven't built DGD yet!";
}

print "Copying Bundled Game from $testgamedir...\n";
system("cp -r $testgamedir $outdir");
print "Copying DGD from $driverdir...\n";
system("cp -r $driverdir $outdir/dgd");
print "Copying Phantasmal from $phantasmaldir...\n";
system("cp -r $phantasmaldir $outdir/phantasmal");

print "Moving Phantasmal directories...\n";
system("rm -rf $outdir/usr/System $outdir/usr/common");
system("mv $outdir/phantasmal/usr/System $outdir/usr/System");
system("mv $outdir/phantasmal/usr/common $outdir/usr/common");
system("rm -rf $outdir/include/kernel $outdir/include/phantasmal");
system("mv $outdir/phantasmal/include/phantasmal $outdir/include/");

print "Moving Kernel Library...\n";
system("rm -rf $outdir/kernel/*");
system("mv $outdir/dgd/mud/kernel/data $outdir/kernel/data");
system("mv $outdir/dgd/mud/kernel/sys $outdir/kernel/sys");
system("mv $outdir/dgd/mud/kernel/obj $outdir/kernel/obj");
system("mv $outdir/dgd/mud/kernel/lib $outdir/kernel/lib");

system("mkdir $outdir/include/kernel");
system("mv $outdir/dgd/mud/include/kernel/*.h $outdir/include/kernel/");

print "Moving DGD binary directory...\n";
system("rm -rf $outdir/bin");
system("mv $outdir/dgd/bin $outdir");

print "Cleaning up non-game files & dirs...\n";
system("rm -rf $outdir/phantasmal");
system("rm -rf $outdir/dgd");

# This won't survive cleaning (at least, not intact), so kill it now
# and recopy it after cleaning is done.
system("rm -rf $outdir/bundled");

system("rm -f $outdir/usr/game/users/*.pwd");
system("rm -f $outdir/kernel/data/access.data");

system("rm -f $outdir/tmp/swap $outdir/log/System.log "
       . " $outdir/bin/driver.old "
       . " $outdir/include/float.h $outdir/include/limits.h "
       . " $outdir/include/status.h $outdir/include/type.h "
       . " $outdir/include/trace.h");

# Remove CVS dirs.  Use a tempfile since find wants to descend into those
# directories, and we've just removed them.
system("find $outdir -name CVS -type d -print > $outdir/distrib_tmp.txt");
system("rm -rf `cat $outdir/distrib_tmp.txt`");
system("rm -f $outdir/distrib_tmp.txt");

# Remove all README, INSTALL, UPDATES and .cvsignore files
system("find $outdir -name README -exec rm \\{\\} \\;");
system("find $outdir -name .cvsignore -exec rm \\{\\} \\;");

# Remove all customization stuff from regular Phantasmal
system("rm -rf $outdir/doc $outdir/docs $outdir/PROBLEMS");
system("rm -f  $outdir/SETUP $outdir/UPDATES $outdir/INSTALL");
system("rm -f  $outdir/Changelog $outdir/TESTED_VERSIONS");

system("find . -name \"*~\" -exec rm \\{\\} \\;");
system("find . -name \"#*\" -exec rm \\{\\} \\;");
system("find . -name \".#*\" -exec rm \\{\\} \\;");

print "Moving bundle-specific files...\n";
system("cp -r $testgamedir/bundled/* $outdir/");
# Get rid of CVS dir we picked up from $testgamedir/bundled
system("rm -rf $outdir/CVS");

print "Copying version information...\n";
system("rm -f $outdir/VERSIONS");
system("echo 'DGD Version:' >> $outdir/VERSIONS");
system("cat $driverdir/src/version.h >> $outdir/VERSIONS");
system("echo 'Phantasmal Version:' >> $outdir/VERSIONS");
system("cat $phantasmaldir/include/phantasmal/version.h >> $outdir/VERSIONS");
system("echo 'Bundled Game Version:' >> $outdir/VERSIONS");
system("cat $testgamedir/include/version.h >> $outdir/VERSIONS");

print "*** Finished! ***\n";
print "New bundled Phantasmal should be available in '../$outdir'.\n";
