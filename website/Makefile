PAGEGEN=../tools/pagecreate.pl
DIRS=API Design DGD Innsmouth Operation Setup Tutorial

recursive:
prereq: api book
pclean: cleanapi cleanbook

dall: prereq recursive
dclean: pclean rclean
include Makefile.in

api:
	make api -C API
cleanapi:
	make cleanapi -C API
book:
	make book -C DGD/Book
cleanbook:
	make cleanbook -C DGD/Book
