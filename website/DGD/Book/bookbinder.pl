#!/usr/bin/perl -w

use strict;

###
### Using bookbinder.pl
###

# First and foremost, using bookbinder requires section files.  These
# contain the HTML or XML content that will eventually become the
# book's pages.  Each section within a section file is delimited by
# the line "@@SECTION SECNAME words in the title", where SECNAME is a
# single word (no whitespace inside it) indicating the chapter title.
# All remaining words in the line are considered to be the title of
# the section.

# Dots (periods, full stops) in chapter names are treated specially.
# Dotted suffixes denote a subsection within that chapter.  For
# instance, Intro.Arrays.Insertion might denote the introductory
# chapter, in the section on arrays, in the subsection on Insertion.
# By default, chapters are ordered by the ASCII string of the section
# name, and subsections are ordered in the same manner within a
# section.  This can be overridden by the book.toc file.

# There needs to be a section.list file which gives a list of all
# section files by filename.  Section files are parsed individually.
# section.list is just a list of filenames separated by newlines.  If
# section.list is omitted, all files in the directory whose name ends
# in ".sec" will be parsed as section files.

# The book.toc file overrides ordering of chapters.  If it doesn't
# exist, chapters and subsections use the default (ASCII) order.
# Book.toc is made up of a number of entries.  Each entry starts with
# a section name followed by a colon, like "Intro:" or
# "Intro.Arrays:".  Then on successive lines of the entry, the immediate
# subsections of that section name are given, in the planned order for
# the book.  For instance, one entry might be:

# Intro.Overview:
# Tools
# Arrays
# Mappings
# Objects
# Conclusion

# In the above example, Intro.Overview.Tools, Intro.Overview.Arrays
# and so on would all need to be valid subsections, and occur
# somewhere within the section files.

# There is also one special entry in the Book.toc file.  Its first
# line is simply a colon followed by a newline.  On succeeding lines,
# the chapter names are given in the planned order.  If any entry,
# including the special entry, is omitted, the unspecified chapters or
# subsections will be ordered alphabetically by name.

# Since bookbinder will use pagecreate.pl as a postpass, there should
# be files called pageheader.html and pagefooter.html.  These will be
# used for final formatting of section files.  Currently bookbinder
# simply outputs a directory of .base.html files which can be used with
# pagecreate.pl rather than running pagecreate for you.


# Top-level vars

my $sectionlist;

if (-e "section.list") {
    $sectionlist = `cat section.list`;
} else {
    $sectionlist = `ls *.sec`;
}

my $toc_contents;

if(-e "book.toc") {
    open(FILE, "<book.toc") or die "Can't open book.toc for reading: $!";
    $toc_contents = join("", <FILE>);
    close(FILE);
} else {
    $toc_contents = "";
}


# Derived vars

my @sectionfiles;
@sectionfiles = split /\s+/, $sectionlist;

# Parse $toc_contents in %chapter_list
my %chapter_order;

{
    my @toc_lines = split /\s+/, $toc_contents;
    my ($line, $linenum, $secname, @secorder, $newsecname);

    $linenum = 0;
    $secname = undef;
  LINE: foreach $line (@toc_lines) {
      $linenum++;

      next LINE if ($line =~ /^\s*$/);  # Skip whitespace lines

      if($line =~ /^\s*(\S+)\s*:\s*$/) {
	  # Parsed a section name
	  $newsecname = $1;
	  if(defined($secname)) {
	      # Store old section info
	      $chapter_order{$secname} = [ @secorder ];
	      @secorder = ();
	  }
	  $secname = $newsecname;
	  next LINE;
      }

      if($line =~ /^\s*(\S+)\s*$/) {
	  # Regular line, add to list
	  my $entryline;

	  $entryline = $1;
	  if($entryline =~ /\./) {
	      die "Entry '$entryline' contains a period, "
		  . "book.toc line $linenum!";
	  }

	  @secorder = (@secorder, $entryline);
	  next LINE;
      }

      die "Unrecognized text on line $linenum.  Line: '$line'";
  }

    if(defined($secname)) {
	$chapter_order{$secname} = [ @secorder ];
    }
}


############## Top-level loop ####################################

# Parse content from sectionfiles into %chapter_jumble
my %chapter_jumble;

my $sectionfile;
foreach $sectionfile (@sectionfiles) {
    parse_section_file($sectionfile);
}

# Write out content into .base.html files



############## Supporting functions ##############################


# parse_section_file parses a .sec file and adds the chapter to the
# %chapter_jumble.  Eventually they'll be sorted and output, but this
# function doesn't do that.

sub parse_section_file {
    my $sectionfile = shift;
    my ($contents, @sections, $section);

    print "Found section file $sectionfile!\n";
    open(FILE, $sectionfile) or die "Can't open file $sectionfile: $!";
    $contents = join("", <FILE>);
    close(FILE);

    @sections = split /^\@\@/m,$contents;
  SECT: foreach $section (@sections) {
      next SECT if $section =~ /^\s*$/;

      if($section =~ /^SECTION\s+(\S+)\s+(.*)/) {
	  print "Found section $1 \n";
	  add_jumbled_chapter_name($1, $2);
      } else {
	  die "Unknown section: '$section' in file $sectionfile!";
      }
  }
    
}


# add_jumbled_chapter_name adds a new section entry to
# %chapter_jumble.  It makes sure that all parent names exist, and
# adds an entry for the full readable title of the entry so it can be
# found later.

# add_jumbled_chapter name takes two string (scalar) arguments.  The
# first is the chapter name in ChapName.SubSec format, the second is
# the readable title of the section.

sub add_jumbled_chapter_name {
    my (@subsec_list, $fullname, $sec, $ent, $title);

    $fullname = shift;
    $title = shift;
    @subsec_list = split /\./, $fullname;
    $sec = "";
    foreach $ent (@subsec_list) {
	$sec .= $ent;
	unless($chapter_jumble{$sec}) {
	    # Put in a placeholder name
	    $chapter_jumble{$sec} = "";
	}
	$sec .= ".";
    }

    $chapter_jumble{$fullname} = $title;
}
