#!/usr/bin/perl -w

use strict;

my (@basefiles, $headerdata, $footerdata);

#open(FILE, "<pageheader.html") or die "Can't open page header: $!";
#$headerdata = join("", <FILE>);
#close(FILE);

#open(FILE, "<pagefooter.html") or die "Can't open page footer: $!";
#$footerdata = join("", <FILE>);
#close(FILE);

#@basefiles = split /\s+/, `ls *.base.html`;
@basefiles = split /\s+/, `find . -name "*.base.html"`;

#print "Basefiles: " . join(", ", @basefiles) . "\n";

my ($filename, $outfilename, $outdir, $contents, %filestate);
FILENAME: foreach $filename (@basefiles) {
    my ($mtime1, $mtime2, $mtime3, $mtime4, $outtmp);

    die("Can't parse filename $filename as base HTML!")
	unless($filename =~ /^(.*)\.base\.html$/i);
    $outtmp = $1;
    $outfilename = $1 . ".html";

    if($outtmp =~ /^(.*)\/[^\/]+/) {
	$outdir = $1;
    } else {
	die "Can't parse dir from '$outtmp'";
    }

    # By default, expect to update
    $mtime2 = 0;
    $mtime3 = 0;
    $mtime4 = 0;

    # Check to see if file needs updating
    ($_,$_,$_,$_,$_,$_,$_,$_,$_,$mtime1,$_,$_,$_)
	= stat($filename);
    ($_,$_,$_,$_,$_,$_,$_,$_,$_,$mtime2,$_,$_,$_)
	= stat($outfilename) if -e $outfilename;
    ($_,$_,$_,$_,$_,$_,$_,$_,$_,$mtime3,$_,$_,$_)
	= stat("$outdir/pageheader.html") if -e "$outdir/pageheader.html";
    ($_,$_,$_,$_,$_,$_,$_,$_,$_,$mtime4,$_,$_,$_)
	= stat("$outdir/pagefooter.html") if -e "$outdir/pagefooter.html";

    next FILENAME if($mtime2 > $mtime1
		     and $mtime3 > $mtime1
		     and $mtime4 > $mtime1);

    open(FILE, "<$filename") or die "Can't open HTML file $filename: $!";
    $contents = join("", <FILE>);
    close(FILE);

    my ($headerdata, $footerdata, $new_hd, $new_fd, $new_cont);

    open(FILE, "<$outdir/pageheader.html") or die "Can't open page header '$outdir/pageheader.html': $!";
    $headerdata = join("", <FILE>);
    close(FILE);
    open(FILE, "<$outdir/pagefooter.html") or die "Can't open page footer '$outdir/pagefooter.html': $!";
    $footerdata = join("", <FILE>);
    close(FILE);

    print "Writing file '$outfilename'.\n";
    open(FILE, ">$outfilename") or die "Can't write to $outfilename: $!";

    $new_cont = extract_metadata($contents, \%filestate);
    $new_hd = $headerdata;
    $new_fd = $footerdata;
    $new_hd =~ s/\@\@TITLE\@\@/$filestate{TITLE}/g;
    $new_fd =~ s/\@\@TITLE\@\@/$filestate{TITLE}/g;

    print FILE $new_hd;
    print FILE $new_cont;
    print FILE $new_fd;
    close(FILE);

    %filestate = ();
}

sub extract_metadata {
    my $block = shift;
    my $stateref = shift;

    if($block =~ /<titledef/) {

	if($block =~ /^(.*)<titledef (.*?)>(.*)$/s) {
	    my $attribs = $2;

	    $block = $1 . $3;

	    if($attribs =~ /text="(.*?)"/) {
		$stateref->{TITLE} = $1;
	    } else {
		die "Couldn't parse attribs of titledef block!\n"
		    . "Attribs: '$attribs'";
	    }
	} else {
	    die "Parse error getting title definition from block of content!\n"
		. "Content: '$contents'";
	}

	print "Title is '$stateref->{TITLE}'\n";
    }

    return $block;
}
