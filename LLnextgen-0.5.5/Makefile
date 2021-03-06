# Copyright (C) 2005,2006,2008 G.P. Halkes
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 3, as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

.POSIX:

# C-compiler flags
CFLAGS=-O2

# Installation prefix
prefix=/usr/local

# Regular expression support
# REGEX should have one of the values POSIX, OLDPOSIX, PCRE, or be unset. All
#  the named options use the POSIX regular expression API. POSIX includes the
#  "regex.h" header file, OLDPOSIX also includes the "sys/types.h" header file,
#  and PCRE uses the "pcreposix.h" header file. The latter also requires
#  linking with -lpcreposix for proper operation.
# REGEXLIBS should provide any options necessary for linking all the libraries
#  required for the regular expression code
REGEX=POSIX
REGEXLIBS=


INSTALL=install
bindir=$(prefix)/bin
docdir=$(prefix)/share/doc/LLnextgen-0.5.5
mandir=$(prefix)/share/man/man1

CSOURCES=src/argcount.c src/clash.c src/depend.c src/freeData.c src/generate.c src/generateCommon.c src/generateLLgenStyle.c src/generateTS.c src/globals.c src/io.c src/list.c src/main.c src/nonRuleAnalysis.c src/option.c src/os.c src/printInput.c src/ruleAnalysis.c src/scope.c src/set.c src/traceTokens.c src/clashStrings.c
LSOURCES=src/lexer.l
GSOURCES=src/grammar.g
OBJECTS=$(CSOURCES:.c=.o) $(LSOURCES:.l=.o) $(GSOURCES:.g=.o)

all: LLnextgen

.PHONY: all install clean dist-clean mr-proper

install: LLnextgen
	$(INSTALL) -d $(bindir)
	$(INSTALL) LLnextgen $(bindir)
	$(INSTALL) -d $(mandir)
	$(INSTALL) -m 644 doc/LLnextgen.1 $(mandir)
	$(INSTALL) -d $(docdir)
	$(INSTALL) -m 644 README COPYING TODO Changelog $(docdir)
	$(INSTALL) -m 644 doc/LLnextgen.ps.gz doc/LLnextgen.pdf $(docdir)
	$(INSTALL) -m 644 doc/calculator.g doc/threadsafe.g $(docdir)

clean:
	rm -rf src/*.o src/grammar.c src/grammar.h src/lexer.c

dist-clean: clean
	rm -rf LLnextgen

mr-proper: dist-clean
	rm -rf config.log Makefile

LLnextgen: $(OBJECTS)
	$(CC) $(LDFLAGS) -o LLnextgen $(OBJECTS) $(LDLIBS) $(REGEXLIBS)

src/lexer.c: src/lexer.c.dist
	cp src/lexer.c.dist src/lexer.c

src/grammar.c: src/grammar.c.dist
	cp src/grammar.c.dist src/grammar.c

src/grammar.h: src/grammar.h.dist
	cp src/grammar.h.dist src/grammar.h

src/grammar.o: src/grammar.h
src/io.o: src/grammar.h
src/lexer.o: src/grammar.h
src/main.o: src/grammar.h
src/printInput.o: src/grammar.h

.c.o:
	$(CC) $(CFLAGS) -DREGEX=$(REGEX) -c $< -o $@
