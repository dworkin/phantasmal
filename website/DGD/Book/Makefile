ROOT=../../..
DIRS=html
include $(ROOT)/website/Makefile.in

cleanbook:
	cd html && rm -f *.base.html
book:
	../../../tools/bookbinder.pl
