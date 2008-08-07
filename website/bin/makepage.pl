#!/usr/bin/perl -w

use strict;

my $filename = $ARGV[0];
my $depth = $ARGV[1];

my $prefix = "../" x $depth;

my $template;
my $banner;
my $header;
my $footer;
my $content;
my $index;
my $output;
my $title;

open(FILE, "<" . $prefix . "template/page.html")
	or die "Can't open page template: $!";
$template = join("", <FILE>);
close FILE;

open(FILE, "<" . $prefix . "template/banner.html")
	or die "Can't open banner template: $!";
$banner = join("", <FILE>);
close FILE;

open(FILE, "<" . $prefix . "template/header.html")
	or die "Can't open header template: $!";
$header = join("", <FILE>);
close FILE;

open(FILE, "<" . $prefix . "template/footer.html")
	or die "Can't open footer template: $!";
$footer = join("", <FILE>);
close FILE;

$index = `${prefix}bin/makeindex.pl $filename $depth`;

open(FILE, "<" . $filename)
	or die "Can't open base file: $!";
$content = join("", <FILE>);
close FILE;

$content =~ s/\@\@TITLE ([^@]*)\@\@//;
$title = $1;
$content =~ s/\@\@SECTION ([^@]*)\@\@//;
$content =~ s/\@\@DIRS ([^@]*)\@\@//;

while($content =~ m/(\@\@INCLUDE ([^@]*)\@\@)/) {
	my $include;

	# sanity checks to make sure that an include file isn't used twice
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
	$content =~ s/(\@\@INCLUDE ([^@]*)\@\@)/$include/;
}

$output = $template;

$output =~ s/\@\@HEADER\@\@/$header/g;
$output =~ s/\@\@BANNER\@\@/$banner/g;
$output =~ s/\@\@INDEX\@\@/$index/g;
$output =~ s/\@\@CONTENT\@\@/$content/g;
$output =~ s/\@\@FOOTER\@\@/$footer/g;

$output =~ s/\@\@TITLE\@\@/$title/g;
$output =~ s/\@\@WEBDIR\@\@/$prefix/g;

$filename =~ s/.base.html/.html/;
open(FILE, ">" . $filename)
	or die "Can't open output file: $!";
print FILE $output;
close FILE;
