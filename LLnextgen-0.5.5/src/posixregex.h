/* Copyright (C) 2006,2008 G.P. Halkes
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 3, as
   published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef POSIXREGEX_H 
#define POSIXREGEX_H 

/* This header file will include the appropriate header file(s) to use the
   POSIX regular expression API. It will also define the macro USE_REGEX if
   the API is available. Code that uses the API should be enclosed in an
   #ifdef USE_REGEX block. It should NOT test for the REGEX macro. To prevent
   accidental checks for the REGEX macro, it is undefined in this header file.
*/

/* Define constants so that REGEX actually has a value when defined with
   REGEX=POSIX, etc. */
#define POSIX 1
#define OLDPOSIX 2
#define PCRE 3

/* Undefine REGEX if it is defined with an empty value. This way we can still
   test with #ifdef REGEX. This could be integrated below, but I like this
   better.
*/
#if defined(REGEX) && REGEX+0==0
#undef REGEX
#endif

/* Check what we need to include, if anything. */
#ifdef REGEX
#	define USE_REGEX
#	if REGEX==POSIX
#		include <regex.h>
#	elif REGEX==OLDPOSIX 
#		include <sys/types.h>
#		include <regex.h>
#	elif REGEX==PCRE
#		include <pcreposix.h>
#	else
#		undef USE_REGEX
#	endif
/* Undefine REGEX so we don't test for it elsewhere. */
#	undef REGEX
#endif

#endif
