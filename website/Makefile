ROOT=.
DEPTH=0
DIRS=API Design DGD Innsmouth Operation Setup Tutorial Test

rhtml:
prereq: api book
pclean: cleanapi cleanbook

dist: prereq rhtml
dclean: rclean pclean
include $(ROOT)/Makefile.in

.PHONY: api cleanapi book cleanbook prereq pclean dist dclean cleancheck

api:
	cd API && ../bin/apidoc.pl
cleanapi:
	rm -f API/html/*.base.html
book:
	cd DGD/Book/src && ../../../bin/bookbinder.pl
cleanbook:
	rm -f DGD/Book/*.base.html
