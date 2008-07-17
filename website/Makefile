ROOT=..
DIRS=API Design DGD Innsmouth Operation Setup Tutorial Test

rhtml:
prereq: api book
pclean: cleanapi cleanbook

dist: prereq rhtml cleancheck
dclean: rclean pclean cleancheck
include $(ROOT)/website/Makefile.in

.PHONY: api cleanapi book cleanbook prereq pclean dist dclean cleancheck

cleancheck:
	rm -f include/*.check include/*/*.check
	rm -f */include/*.check */include/*/*.check
	rm -f */*/include/*.check */*/include/*/*.check
	rm -f */*/*/include/*.check */*/*/include/*/*.check
api:
	make api -C API
cleanapi:
	make cleanapi -C API
book:
	make book -C DGD/Book
cleanbook:
	make cleanbook -C DGD/Book
