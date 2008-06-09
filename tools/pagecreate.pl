#!/usr/bin/perl -w

use strict;

sub xml_escape {
   my $input = shift;
   
   $input =~ s/&/&amp;/g;
   $input =~ s/</&lt;/g;
   $input =~ s/>/&gt;/g;
   
   return $input;
}

my (@basefiles, $fileout, $templatedata, $indexdata);

open(FILE, "<pagetemplate.html") or die "Can't open page template: $!";
$templatedata = join("", <FILE>);
close(FILE);

open(FILE, "<pageindex.html") or die "Can't open page index: $!";
$indexdata = join("", <FILE>);
close(FILE);

chomp $indexdata;

$fileout = `echo *.base.html`;
chomp $fileout;
if($fileout eq '*.base.html') {
    $fileout = "";
}
@basefiles = split /\s+/, $fileout;

#print "Basefiles: " . join(", ", @basefiles) . "\n";

my ($mtime_template);
($_,$_,$_,$_,$_,$_,$_,$_,$_,$mtime_template,$_,$_,$_)
    = stat("pagetemplate.html");

my ($filename, $outfilename, $contents, %filestate);
FILENAME: foreach $filename (@basefiles) {
    my ($mtime1, $mtime2);

    die("Can't parse filename $filename as base HTML!")
	unless($filename =~ /^(.*)\.base\.html$/i);
    $outfilename = $1 . ".html";

    # By default, expect to update
    $mtime2 = 0;

    # Check to see if file needs updating
    ($_,$_,$_,$_,$_,$_,$_,$_,$_,$mtime1,$_,$_,$_)
	= stat($filename);
    ($_,$_,$_,$_,$_,$_,$_,$_,$_,$mtime2,$_,$_,$_)
	= stat($outfilename) if -e $outfilename;

    next FILENAME if($mtime2 > $mtime1 and $mtime2 > $mtime_template);

    open(FILE, "<$filename") or die "Can't open HTML file $filename: $!";
    $contents = join("", <FILE>);
    close(FILE);

    print "Writing file '$outfilename'.\n";
    open(FILE, ">$outfilename") or die "Can't write to $outfilename: $!";

    my ($new_td, $new_cont, $message_cont);
    $new_cont = extract_metadata($contents, \%filestate);
    chomp $new_cont;
    $new_td = $templatedata;
    $new_td =~ s/ *\@\@TITLE\@\@ */$filestate{TITLE}/g;
    $new_td =~ s/ *\@\@CONTENT\@\@ */$new_cont/g;
    $new_td =~ s/\t* *\@\@INDEX\@\@ */$indexdata/g;
    $new_td =~ s/ *\@\@FILE\@\@ */$outfilename/g;
    
    # translate @@MESSAGE foo@@'s with the contents of messages/foo
    # with proper XML escaping
    
    while($new_td =~ /(\@\@MESSAGE ([^@]*)\@\@)/) {
        my $message;
        print "Message found: " . $2 . "\n";

	open(MESSAGEFILE, "<messages/$2.msg") or die "Can't open message file messages/$2.msg: $!";
	$message = join("", <MESSAGEFILE>);
	close(MESSAGEFILE);
	
	$message =~ s/&/&amp;/g;
	$message =~ s/\</&lt;/g;
	$message =~ s/\>/&gt;/g;
	
	chomp $message;
	$new_td =~ s/(\@\@MESSAGE ([^@]*)\@\@)/$message/;
    }
    
out:

    print FILE $new_td;
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
