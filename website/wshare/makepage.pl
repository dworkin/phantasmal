#!/usr/bin/perl -w

use strict;

my $filename = $ARGV[0];
my $outfilename = $filename;
$outfilename =~ s/.base.html/.html/;

my $prefix = $ENV{"PREFIX"};
@_ = split("/", $prefix);
my $depth = @_ + 1;

my $buffer;	# general file buffer

my @sections;

open(FILE, "<" . $prefix . "wshare/page.html")
	or die "Can't open page template: $!";
my $template = join("", <FILE>);
close FILE;

open(FILE, "<" . $prefix . "template/banner.html")
	or die "Can't open banner template: $!";
my $banner = join("", <FILE>);
close FILE;

open(FILE, "<" . $prefix . "template/header.html")
	or die "Can't open header template: $!";
my $header = join("", <FILE>);
close FILE;

open(FILE, "<" . $prefix . "template/footer.html")
	or die "Can't open footer template: $!";
my $footer = join("", <FILE>);
close FILE;

open(FILE, "<" . $filename)
	or die "Can't open base file: $!";
my $content = join("", <FILE>);
close FILE;

open(FILE, "<" . $prefix . "template/title.html")
	or die "Can't open title template: $!";
my $titleline = join("", <FILE>);
close FILE;


my $title;

if ($content =~ s/\@\@TITLE ([^@]*)\@\@//) {
	$title = $1;
} else {
	$title = "(untitled)";
	print STDERR "Warning: $filename doesn't have a title\n";
}

$content =~ s/\@\@SECTION ([^@]*)\@\@//;

my $index = `${prefix}bin/makeindex.pl $outfilename $depth`;

# process any include directives
while($content =~ m/(\@\@INCLUDE ([^@]*)\@\@)/) {
	my $include;

	open(INCLUDEFILE, "<include/$2.in") or die "Can't open include file include/$2.in: $!";
	$include = join("", <INCLUDEFILE>);
	close(INCLUDEFILE);
	
	$include =~ s/&/&amp;/g;
	$include =~ s/\</&lt;/g;
	$include =~ s/\>/&gt;/g;
	
	chomp $include;
	$content =~ s/(\@\@INCLUDE ([^@]*)\@\@)/$include/;
}

my $output = $template;

$output =~ s/\@\@HEADER\@\@/$header/g;
$output =~ s/\@\@BANNER\@\@/$banner/g;
$output =~ s/\@\@INDEX\@\@/$index/g;
$output =~ s/\@\@CONTENT\@\@/$content/g;
$output =~ s/\@\@FOOTER\@\@/$footer/g;

$output =~ s/\@\@TITLELINE\@\@/$titleline/g;
$output =~ s/\@\@TITLE\@\@/$title/g;
$output =~ s/\@\@WEBDIR\@\@/$prefix/g;

open(FILE, ">" . $outfilename)
	or die "Can't open output file: $!";
print FILE $output;
close FILE;
