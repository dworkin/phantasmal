PREFIX=
export PREFIX

rhtml:
prereq: api book
pclean: cleanapi cleanbook

dist: prereq rhtml
dclean: reclean pclean
include $(PREFIX)Makefile.in

.PHONY: api cleanapi book cleanbook prereq pclean dist dclean cleancheck

api:
	cd API && ../bin/apidoc.pl
cleanapi:
	rm -f API/html/*.base.html
book:
	cd DGD/Book/src && ../../../bin/bookbinder.pl
cleanbook:
	rm -f DGD/Book/*.base.html
