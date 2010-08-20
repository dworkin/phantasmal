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

open(FILE, "<" . $prefix . "template/page.html")
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

open(FILE, "<pageindex.full")
	or die "Can't open page index: $!";

my @breadcrumbs;
my $nolast = 0;

foreach (<FILE>) {
	if ($_ =~ m/Section "(.*)"/) {
		push @breadcrumbs,$1
	}
	if ($_ =~ m/Root .*/) {
		$nolast = 1;
	}
}
close FILE;

my $upcount = 0;
my $lastcrumb;

if ($filename eq "index.base.html") {
	$lastcrumb = pop @breadcrumbs;
	$upcount++;
}

my $breadcrumbs;

while (@breadcrumbs) {
	my $crumb = shift @breadcrumbs;
	$breadcrumbs .= "<a href=\"" . ("../" x $upcount)
		. "index.html\">" . $crumb . "</a> > ";
}

my $title;

if ($content =~ s/\@\@TITLE ([^@]*)\@\@//) {
	$title = $1;
} else {
	$title = "(untitled)";
	print STDERR "Warning: $filename doesn't have a title\n";
}

if ($filename eq "index.base.html") {
	pop @breadcrumbs;
	$upcount++;
}

if ($lastcrumb) {
	$breadcrumbs .= $lastcrumb;
} else {
	$breadcrumbs .= $title;
}

$content =~ s/\@\@SECTION ([^@]*)\@\@//;
$content =~ s/\@\@SEQUENCE ([^@]*)\@\@//;

my $index = `${prefix}build/makeindex.pl $outfilename $depth`;

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

$content = "<p>" . $breadcrumbs . "</p>\n\n" . $content;

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
