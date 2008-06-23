#!/usr/bin/perl -w

use strict;

my ($content,@preformatted,$preindex);

$preindex = 0;

print "Processing " . $ARGV[0] . "\n";

open(FILE, "<$ARGV[0]") or die "File open error";
$content = join("", <FILE>);
close FILE;

$content =~ s/\<titledef ([^\>]*)\/\>/\<!-- TITLEDEF $1 --\>/;
$content =~ s/^/  /mg;

open(FILE, ">$ARGV[0].tmp");
print FILE "<p><!-- START --></p>" . $content . "<p><!-- END --></p>";
close FILE;

system "tidy -asxhtml -q -i -w 75 -m $ARGV[0].tmp" or die "Tidy subshell error";

open(FILE, "<$ARGV[0].tmp") or die "File open error";
my $garbage = <FILE>;
$content = join("", <FILE>);
close FILE;

$content =~ s/^  //mg;
$content =~ s/\n\<\/pre\>/\<\/pre\>/mg;
$content =~ s/.*\<p\>\<!-- START --\>\<\/p\>\n*//s;
$content =~ s/\n*\<p\>\<!-- END --\>\<\/p\>.*//s;
$content =~ s/\<!-- TITLEDEF ([^\>]*) --\>/\<titledef $1\/\>/;

open(FILE, ">$ARGV[0].tmp") or die "File open error";
print FILE $content . "\n";
close FILE;
rename "$ARGV[0].tmp","$ARGV[0].out.html"
