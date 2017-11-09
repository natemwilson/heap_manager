#---------------------------------------------------------------------
# Makefile
# Author: Bob Dondero
#---------------------------------------------------------------------

# This is not a proper Makefile!  It does not maintain .o files to
# allow for partial builds. Instead it simply automates the issuing
# of commands for each step of the assignment.

#---------------------------------------------------------------------
# Build rules for non-file targets
#---------------------------------------------------------------------

all: step1 step2 step3 step4 step5 step6 step7

clean:
	rm -f test1bad* test1d test1 test1good
	rm -f test2bad* test2d test2 test2good
	rm -f testgnu testbase

#---------------------------------------------------------------------
# Build rules for the steps of the assignment
#---------------------------------------------------------------------

step1:
	gcc217 -g testheapmgr.c heapmgr1bada.o checker1.c chunk.c \
	-o test1bada
	gcc217 -g testheapmgr.c heapmgr1badb.o checker1.c chunk.c \
	-o test1badb
	gcc217 -g testheapmgr.c heapmgr1badc.o checker1.c chunk.c \
	-o test1badc
	gcc217 -g testheapmgr.c heapmgr1badd.o checker1.c chunk.c \
	-o test1badd
	gcc217 -g testheapmgr.c heapmgr1bade.o checker1.c chunk.c \
	-o test1bade
	gcc217 -g testheapmgr.c heapmgr1badf.o checker1.c chunk.c \
	-o test1badf
	gcc217 -g testheapmgr.c heapmgr1badg.o checker1.c chunk.c \
	-o test1badg

step2:
	gcc217 -g testheapmgr.c heapmgr1.c checker1.c chunk.c \
	-o test1d
	gcc217 -D NDEBUG -O testheapmgr.c heapmgr1.c chunk.c \
	-o test1
	gcc217 -D NDEBUG -O testheapmgr.c heapmgr1good.o chunk.c \
	-o test1good

step3:
	splint testheapmgr.c heapmgr1.c checker1.c chunk.c
	critTer checker1.c
	critTer heapmgr1.c

step4:
	gcc217 -g testheapmgr.c heapmgr2bada.o checker2.c chunk.c \
	-o test2bada
	gcc217 -g testheapmgr.c heapmgr2badb.o checker2.c chunk.c \
	-o test2badb
	gcc217 -g testheapmgr.c heapmgr2badc.o checker2.c chunk.c \
	-o test2badc
	gcc217 -g testheapmgr.c heapmgr2badd.o checker2.c chunk.c \
	-o test2badd
	gcc217 -g testheapmgr.c heapmgr2bade.o checker2.c chunk.c \
	-o test2bade
	gcc217 -g testheapmgr.c heapmgr2badf.o checker2.c chunk.c \
	-o test2badf
	gcc217 -g testheapmgr.c heapmgr2badg.o checker2.c chunk.c \
	-o test2badg
	gcc217 -g testheapmgr.c heapmgr2badh.o checker2.c chunk.c \
	-o test2badh

step5:
	gcc217 -g testheapmgr.c heapmgr2.c checker2.c chunk.c \
	-o test2d
	gcc217 -D NDEBUG -O testheapmgr.c heapmgr2.c chunk.c \
	-o test2
	gcc217 -D NDEBUG -O testheapmgr.c heapmgr2good.o chunk.c \
	-o test2good

step6:
	splint testheapmgr.c heapmgr2.c checker2.c chunk.c
	critTer checker2.c
	critTer heapmgr2.c

step7:
	gcc217 -D NDEBUG -O testheapmgr.c heapmgrgnu.c \
	-o testgnu
	gcc217 -D NDEBUG -O testheapmgr.c heapmgrbase.c chunkbase.c \
	-o testbase
