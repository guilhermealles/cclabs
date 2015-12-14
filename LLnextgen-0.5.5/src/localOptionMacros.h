/* Copyright (C) 2005,2006,2008 G.P. Halkes
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

#ifndef LOCAL_OPTIONMACROS_H
#define LOCAL_OPTIONMACROS_H

#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <limits.h>

/** Retrieve the necessary information from the current option string. */
#define PREPROCESS_OPTION() \
	size_t optlength; \
	char *optarg; \
	if ((optarg = strchr(optstring, '=')) == NULL) { \
		optlength = strlen(optstring); \
	} else { \
		optlength = optarg - optstring; \
		optarg++; \
		if (*optarg == 0) { \
			fatal("Option %.*s has zero length argument\n", (int) optlength, optstring); \
		} \
	} \
	if (optlength > INT_MAX) optlength = INT_MAX; 
/* The last line above is to make sure the cast to int in error messages does
   not overflow. */

/** Internal macro to check whether the requirements regarding option arguments
		have been met. */
#define CHECK_ARG(argReq) \
	switch(argReq) { \
		case NO_ARG: \
			if (optarg != NULL) { \
				fatal("Option %.*s does not take an argument\n", (int) optlength, optstring); \
			} \
			break; \
		case REQUIRED_ARG: \
			if (optarg == NULL) { \
				fatal("Option %.*s requires an argument\n", (int) optlength, optstring); \
			} \
			break; \
		default: \
			break; \
	}

/** Check for a long style (--option) option.
	@param longName The name of the long style option.
	@param argReq Whether or not an argument is required/allowed. One of NO_ARG,
		OPTIONAL_ARG or REQUIRED_ARG.
*/
#define LOCAL_OPTION(longName, argReq) if (strlen(longName) == optlength && strncmp(optstring, longName, optlength) == 0) { CHECK_ARG(argReq) {
/** Signal the end of processing for the previous LOCAL_OPTION. */
#define LOCAL_END_OPTION } continue; }

/** Check for presence of an option and set the variable if so.
	@param longName The name of the option.
	@param var The variable to set.
*/
#define LOCAL_BOOLEAN_OPTION(longName, var) LOCAL_OPTION(longName, NO_ARG) var = true; LOCAL_END_OPTION

/** Check for presence of an option and set the variable if so.
	@param longName The name of the option.
	@param var The variable to set.
*/
#define LOCAL_OPTION_USED(longName, var) LOCAL_OPTION(longName, OPTIONAL_ARG) var = true; }}

/** Check an option argument for an integer value.
	@param var The variable to store the result in.
	@param min The minimum allowable value.
	@param max The maximum allowable value.
*/
#define PARSE_INT(var, min, max) do {\
	char *endptr; \
	long value; \
	errno = 0; \
	\
	value = strtol(optarg, &endptr, 10); \
	if (*endptr != 0) { \
		fatal("Garbage after value for %.*s option\n", (int) optlength, optstring); \
	} \
	if (errno != 0 || value < min || value > max) { \
		fatal("Value for %.*s option (%ld) is out of range\n", (int) optlength, optstring, value); \
	} \
	var = (int) value; } while(0)

enum {
	NO_ARG,
	OPTIONAL_ARG,
	REQUIRED_ARG
};

#endif
