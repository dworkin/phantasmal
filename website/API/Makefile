ROOT=../..
DIRS=html

include $(ROOT)/website/Makefile.in

api:
	../../tools/apidoc.pl
cleanapi:
	rm -f html/*.base.html
