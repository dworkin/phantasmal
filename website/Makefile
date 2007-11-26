ROOT=..
DIRS=API Design DGD Innsmouth Operation Setup Tutorial

rhtml:
prereq: api book
pclean: cleanapi cleanbook

dist: prereq rhtml
dclean: rclean pclean
include $(ROOT)/website/Makefile.in

.PHONY: api cleanapi book cleanbook prereq pclean dist dclean

api:
	make api -C API
cleanapi:
	make cleanapi -C API
book:
	make book -C DGD/Book
cleanbook:
	make cleanbook -C DGD/Book
