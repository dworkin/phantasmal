#!/usr/bin/perl -w

use strict;

my (@basefiles, $headerdata, $footerdata);

open(FILE, "<pageheader.html") or die "Can't open page header: $!";
$headerdata = join("", <FILE>);
close(FILE);

open(FILE, "<pagefooter.html") or die "Can't open page footer: $!";
$footerdata = join("", <FILE>);
close(FILE);

@basefiles = split /\s+/, `ls *.base.html`;

#print "Basefiles: " . join(", ", @basefiles) . "\n";

my ($filename, $outfilename, $contents, %filestate);
foreach $filename (@basefiles) {
    open(FILE, "<$filename") or die "Can't open HTML file $filename: $!";
    $contents = join("", <FILE>);
    close(FILE);

    die("Can't parse filename $filename as base HTML!")
	unless($filename =~ /^(.*)\.base\.html$/i);

    $outfilename = $1 . ".html";
    print "Writing file '$outfilename'.\n";
    open(FILE, ">$outfilename") or die "Can't write to $outfilename: $!";

    my ($new_hd, $new_fd, $new_cont);
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

	if($block =~ /^((.|\n)*)<titledef (.*)>((.|\n)*)$/) {
	    my $attribs = $3;

	    $block = $1 . $4;

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
