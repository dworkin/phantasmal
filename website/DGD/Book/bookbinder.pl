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
# in ".sec.html" will be parsed as section files.

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

# There is also one special entry in the 'book.toc' file.  Its first
# line is simply a colon followed by a newline.  On succeeding lines,
# the chapter names are given in the planned order.  If any entry,
# including the special entry, is omitted, the unspecified chapters or
# subsections will be ordered alphabetically by name.

# Since bookbinder will use pagecreate.pl as a postpass, there should
# be files called pageheader.html and pagefooter.html.  These will be
# used for final formatting of section files.  Currently bookbinder
# simply outputs a directory of .base.html files which can be used with
# pagecreate.pl rather than running pagecreate for you.


# Argument-type vars

my $outdir = "html";

my $sectionlist;

if (-e "section.list") {
    $sectionlist = `cat section.list`;
} else {
    $sectionlist = `ls *.sec.html`;
}

my $toc_contents;

if(-f "book.toc") {
    open(FILE, "<book.toc") or die "Can't open book.toc for reading: $!";
    $toc_contents = join("", <FILE>);
    close(FILE);
} else {
    $toc_contents = "";
}


# Derived vars

my @sectionfiles;
@sectionfiles = split /\s+/, $sectionlist;

# Every entry in %chapter_order is a list reference.  The entries in
# the list are the subsection names of that section, in the order they
# should appear in the output.

my %chapter_order;

# Parse $toc_contents into %chapter_order
parse_toc_contents($toc_contents);


############## Top-level loop ####################################

# Parse content from sectionfiles into %chapter_jumble
# Every entry of %chapter_jumble is a list reference of the
# form [ $title, $content ].
my %chapter_jumble;

my $sectionfile;
foreach $sectionfile (@sectionfiles) {
    parse_section_file($sectionfile);
}

# Rearrange %chapter_jumble according to %chapter_order
sort_chapters();

# Write out content into .base.html files
write_out_html();


############## Supporting functions ##############################

# parse_toc_contents 

sub parse_toc_contents {
    my $toc_contents = shift;

    my @toc_lines = split /\s+/, $toc_contents;
    my ($line, $linenum, $secname, @secorder, $newsecname);

    $linenum = 0;
    $secname = undef;
  LINE: foreach $line (@toc_lines) {
      $linenum++;

      next LINE if ($line =~ /^\s*$/);  # Skip whitespace lines

      if($line =~ /^\s*(\S*)\s*:\s*$/) {
	  # Parsed a section name
	  $newsecname = ($1 ? $1 : "");
	  if(defined($secname)) {
	      # Store old section info
	      print "Setting order of '$secname' to " . join(", ", @secorder)
		  . "\n";
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
	print "Setting order of '$secname' to " . join(", ", @secorder)
	    . "\n";
    }
}


# parse_section_file parses a .sec.html file and adds the chapter to
# the %chapter_jumble.  Eventually they'll be sorted and output, but
# this function doesn't do that.

sub parse_section_file {
    my $sectionfile = shift;
    my ($contents, @sections, $section, $secname);

    open(FILE, $sectionfile) or die "Can't open file $sectionfile: $!";
    $contents = join("", <FILE>);
    close(FILE);

    # Add empty top-level entry
    $chapter_jumble{""} = [undef, undef];

    @sections = split /^\@\@/m,$contents;
  SECT: foreach $section (@sections) {
      next SECT if $section =~ /^\s*$/;

      if($section =~ /^SECTION\s+([A-Za-z0-9.\-?!]+)\s+(.*)$/m) {
          my $title;

	  $secname = $1;
	  $title = $2;
	  add_jumbled_chapter_name($secname, $title);
      } else {
	  die "Unknown section: '$section' in file $sectionfile!";
      }

      # Now cut the first line off
      die "Can't parse SECTION '$section'!"
	  unless $section =~ /^(.*?)$(.*)/ms;

    $section = $2;

    # Now add the section's contents to the jumble.
    $chapter_jumble{$secname}->[1] = $section;
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
    my (@subsec_list, $fullname, $sec, $ent, $title, $parent);

    $fullname = shift;
    $title = shift;
    @subsec_list = split /\./, $fullname;
    $sec = "";
    $parent = "";
    foreach $ent (@subsec_list) {
	$sec .= $ent;
	unless(defined($chapter_jumble{$sec})) {
	    # Put in a placeholder name
	    $chapter_jumble{$sec} = [ undef, undef ];
	    $chapter_jumble{$parent} = [@{$chapter_jumble{$parent}}, $sec];
	}
	$parent = $sec;
	$sec .= ".";
    }

    if(defined($chapter_jumble{$fullname}->[0])) {
	die "Repeated section $fullname in .sec.html files!";
    }
    $chapter_jumble{$fullname}->[0] = $title;
}


# sort_chapters takes no arguments, but uses %chapter_order and
# %chapter_jumble.  It modifies %chapter_jumble so that each parent
# entry's children are ordered according to %chapter_order, or
# alphabetically if there is no corresponding entry in %chapter_order.

# It also inserts extra data that is easier to calculate as a postpass
# than to insert as the sections are parsed, such as the filenames for
# section HTML files.

sub sort_chapters {
    my @sections = sort keys %chapter_jumble;

    my $section;
    foreach $section (@sections) {
	my ($title, $cont, @subsections) = @{$chapter_jumble{$section}};

	if(defined($chapter_order{$section})) {
	    # Give warnings for any elements that aren't defined properly
	    my (%count, %intersection, $elt);

	    foreach $elt (@subsections) {
		$count{$elt}++;
	    }
	    foreach $elt (@{$chapter_order{$section}}) {
		$count{$section eq "" ? $elt : "$section.$elt"}++;
	    }
	    foreach $elt (keys %count) {
		if($count{$elt} > 1) {
		    $intersection{$elt} = 1;
		}
	    }

	    foreach $elt (@subsections) {
		unless($intersection{$elt}) {
		    print "*** WARNING: Subsection '$elt' is in the section"
			. " files, but not book.toc!\n";
		}
	    }

	    foreach $elt (@{$chapter_order{$section}}) {
		my $eltname = $section eq "" ? $elt : "$section.$elt";

		unless($intersection{$eltname}) {
		    print "*** WARNING: Subsection $eltname is in "
			. "book.toc, but not the section files!\n";
		}
	    }

	    @subsections = keys %intersection;
	} else {
	    @subsections = sort @subsections;
	}

	my $filename = $section;
	if($section eq "") {
	    $filename = "index";
	} else {
	    $filename =~ s/\./_/;
	}

	$chapter_jumble{$section} = [$title, $cont, $filename,
				     @subsections];
    }

}


### Functions for .base.html output

# Section Indices are things like 2.3.4 or 3.7.1.3 to indicate where
# in the outline a given section occurs.  The %sec_index mapping maps
# section names to their indices.

my %sec_index;

# write_out_html iterates through the (now-sorted) %chapter_jumble
# mapping.  It must output sections appropriately, remembering that
# the order in the book.toc file may include sections that don't yet
# exist, and may fail to list sections that *do* exist.  Both cases
# should work, but generate warnings.

# Eventually, the current index.html will be replaced with a full
# table of contents.  It's possible that other pages like a copyright
# page and index will be added, but that's much later.

# It's easier to build up a Table of Contents as we go.  This
# variable does so, tracking through various subroutines.
my $toc_accum;
my $toc_indent;

sub write_out_html {
    my (@chapter_list, $chapter, $index);

    $toc_accum = "";
    $toc_indent = 0;

    # Write out various HTML files, collecting TOC entries as we go
    ($_, $_, $_, @chapter_list) = @{$chapter_jumble{""}};
    write_out_sections("", @chapter_list);

    open(FILE, ">$outdir/index.base.html")
	or die "Can't open index file ($outdir/index.base.html) "
	    . "for writing\n: $!";

    print FILE "<titledef text=\"Top Level\">\n\n";
    print FILE "<h1> Top-Level </h1>\n";
    print FILE "<ul>\n";

    print FILE $toc_accum;

    print FILE "</ul>\n";
    close(FILE);
}


# write_out_sections takes a list of sections, which must all have the
# same parent section.  Each will be written into its own section file,
# with appropriate links back and forth and appropriate lists of child
# sections.

sub write_out_sections {
    my $parent_section = shift;
    my @sections = @_;

    my ($prefix, $secnum, $index);
    $prefix = ($parent_section eq "" ? "" : $sec_index{$parent_section});
    unless(defined($prefix)) {
	die "Can't get prefix for parent $parent_section!";
    }

    my $section;
    $index = 0;
    foreach $section (@sections) {
	$index++;
	$secnum = ($prefix eq "") ? ("" . $index) : "$prefix.$index";
	$sec_index{$section} = $secnum;

	$toc_accum .= (" " x $toc_indent)
	    . "<ul> <!-- subsections of $parent_section -->\n";
	$toc_indent += 2;

	html_for_section($section, $secnum);

	# If there are subsections, write their HTML files also
	my @subsections;
	($_, $_, $_, @subsections) = @{$chapter_jumble{$section}};
	if(scalar(@subsections)) {
	    write_out_sections($section, @subsections);
	}

	$toc_indent -= 2;
	$toc_accum .= (" " x $toc_indent)
	    . "</ul> <!-- done with $parent_section -->\n";
    }
}

sub html_for_section {
    my ($secname, $secnum) = @_;

    my ($title, $content, $filename, @subsections)
	= @{$chapter_jumble{$secname}};

    unless(defined($title)) {
	die "No jumble entry for '$secname'?";
    }
    unless(defined($filename) and $filename ne "") {
	die "Can't get filename for section $secname!";
    }
    open(SECFILE, ">$outdir/$filename.base.html")
	or die "Can't open section file $filename: $!";

    print "Printing to SECFILE $filename.base.html...\n";

    print SECFILE "<titledef text=\"$title\">\n\n";

    print SECFILE "<h2> $secnum $title </h2>\n";
    print SECFILE $content;

    $toc_accum .= (" " x $toc_indent) . "<li> <a href=\"$filename.html\"> "
	. "$secnum $title </a> </li>\n";

    # List of subsections
    if(scalar(@subsections)) {
	print SECFILE "<ul>\n";
	my $subsection;
	my $subindex = 0;
	foreach $subsection (@subsections) {
	    $subindex++;

	    my ($sectitle, $subcont, $subfilename)
		= @{$chapter_jumble{$subsection}};

	    print SECFILE "  <li> <a href=\"$subfilename.html\"> "
		. "$secnum.$subindex $sectitle </a> </li>\n";

	}
	print SECFILE "</ul>\n";
    }

    # Link to previous section

    # Link to next section

    close(SECFILE);
}
