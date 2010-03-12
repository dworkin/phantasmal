PREFIX=
export PREFIX

rhtml:
prereq:
pclean:

dist: prereq rhtml
dclean: reclean pclean
include $(PREFIX)build/Makefile.in

.PHONY: prereq pclean dist dclean cleancheck bclean aclean api book

api:
	cd API && ../build/apidoc.pl
aclean:
	cd API && rm -f html/*.base.html

book:
	cd DGD/Book/src && ../../../build/bookbinder.pl
bclean:
	cd DGD/Book/src && rm -f ../*.base.html
