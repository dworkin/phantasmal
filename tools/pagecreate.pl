#!/usr/bin/perl -w

use strict;

sub xml_escape {
   my $input = shift;
   
   $input =~ s/&/&amp;/g;
   $input =~ s/</&lt;/g;
   $input =~ s/>/&gt;/g;
   
   return $input;
}

my ($templatedata, $indexdata);

open(FILE, "<pagetemplate.html") or die "Can't open page template: $!";
$templatedata = join("", <FILE>);
close(FILE);

open(FILE, "<pageindex.html") or die "Can't open page index: $!";
$indexdata = join("", <FILE>);
close(FILE);

chomp $indexdata;

my ($mtime_template);
($_,$_,$_,$_,$_,$_,$_,$_,$_,$mtime_template,$_,$_,$_)
    = stat("pagetemplate.html");

my $filename;
my $outfilename;
my $contents;
my %filestate;

$filename = $ARGV[0];

die("Can't parse filename $filename as base HTML!")
	unless ($filename =~ /^(.*)\.base\.html$/i);

$outfilename = $1 . ".html";

print "pagecreate.pl: Processing $1\n";

{
    open(FILE, "<$filename") or die "Can't open HTML file $filename: $!";
    $contents = join("", <FILE>);
    close(FILE);

    print "Writing file '$outfilename'.\n";
    open(FILE, ">$outfilename") or die "Can't write to $outfilename: $!";

    my ($new_td, $new_cont, $message_cont);
    $new_cont = extract_metadata($contents, \%filestate);
    chomp $new_cont;
    $new_td = $templatedata;
    $new_td =~ s/\t* *\@\@TITLE\@\@ */$filestate{TITLE}/g;
    $new_td =~ s/\t* *\@\@CONTENT\@\@ */$new_cont/g;
    $new_td =~ s/\t* *\@\@INDEX\@\@ */$indexdata/g;
    $new_td =~ s/\t* *\@\@FILE\@\@ */$outfilename/g;
    
    while($new_td =~ /(\@\@INCLUDE ([^@]*)\@\@)/) {
        my $include;
        print "Include found: " . $2 . "\n";

	open(CHECKFILE,"<include/$2.check") and die "Duplicate inclusion of $2 while processing $filename: $!";
	close(CHECKFILE);
	open(CHECKFILE,">include/$2.check") or die "Can't create include check file $2.check: $!";
	close(CHECKFILE);

	open(INCLUDEFILE, "<include/$2.in") or die "Can't open include file include/$2.in: $!";
	$include = join("", <INCLUDEFILE>);
	close(INCLUDEFILE);
	
	$include =~ s/&/&amp;/g;
	$include =~ s/\</&lt;/g;
	$include =~ s/\>/&gt;/g;
	
	chomp $include;
	$new_td =~ s/(\@\@INCLUDE ([^@]*)\@\@)/$include/;
    }
    
out:

    print FILE $new_td;
    close(FILE);

    %filestate = ();
}

sub extract_metadata {
    my $block = shift;
    my $stateref = shift;
    
    $stateref->{TITLE} = "(untitled)";

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
	
	print "Warning: untitled document " if ($stateref->{TITLE} eq "(untitled)");
    }

    return $block;
}
