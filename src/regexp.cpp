/**
  Library name: Regular expression support
  ----------------------------------------------------------------------
  Author:   Radim Loskot
  Date:     18. 2. 2011
  Filename: regexp.cpp
  Descr:    Extends std library about primitive regular expression operations. 
 */

#include <errno.h>
#include <iostream>

#include "regexp.h"

using namespace std::regexp;

/** 
 * Regular expression constructor.
 * @param expr regular expression as string which will be compiled
 * @param optflags some adjustments how should be regular expression evaluated
 */
RegExp::RegExp(string expr, int optflags) {
	flags = optflags;
	initialized = false;
	if (!expr.empty()) {
		if (expr_str != expr) {
			expr_str = expr;
			// Block reporting only success/fail
			// For that purpose was programmed test function
			flags &= ~REG_NOSUB;
			if(regcomp( &regex, expr.c_str(), flags)!= 0) {
				errno = EREG_COMP;		// Compilation failure
				regfree( &regex);
			} else {
				initialized = true;		// Success
			}
		}		
	} else {
		errno = EREG_NULL;
	}
}

/** 
 * String asigment into Regualar expression. Behaves similar to constructor.
 * @param expr regular expression as string which will be compiled
 * @return RegExp structure with updated compliled regular expression
 */
RegExp& RegExp::operator=(const string& expr) {
	if (!expr.empty()) {
		if (expr_str != expr) {
			expr_str = expr;
			if (initialized) regfree(&regex);
			// Block reporting only success/fail
			// For that purpose was programmed test function
			flags &= ~REG_NOSUB;
			if(regcomp( &regex, expr.c_str(), flags)!= 0) {
				errno = EREG_COMP;		// Compilation failure
				regfree( &regex);
				initialized = false;
			} else {
				initialized = true;		// Success
			}
		}		
	} else {
		errno = EREG_NULL;
	}

	return *this;
}

/** 
 * Checks whether pattern fits to regular expression and parses pattern in depending on subexpression.
 * @param pattern string which is verified whether is correct to regular expression
 * @return vector that contains all strings directly depending on number of subexpression in regular expression
 */
vector<string> RegExp::exec(const string pattern) {

	vector<string> matches;
	vector<regmatch_t> pmatch(regex.re_nsub + 1);
	
	int status = regexec( &regex, pattern.c_str(), regex.re_nsub + 1, &pmatch[0], 0);
	if(status == 0) {					// Pattern fits
		unsigned int i = 0;
		for (i = 0; i <= regex.re_nsub; i++) {
			if (pmatch[i].rm_so > -1) {	// There is part which fits to subexpression
				matches.push_back(pattern.substr(pmatch[i].rm_so, pmatch[i].rm_eo - pmatch[i].rm_so));	
			} else {					// Nothing fits to this subexpression
				matches.push_back("");
			}
		}
	} else {
		errno = EREG_EXEC;	
	}

	return matches;
}

/** 
 * Checks whether pattern fits to regular and returns result.
 * @param pattern string which is verified whether is correct to regular expression
 * @return true/false whether pattern fits to regular expression
 */
bool RegExp::test(const string pattern) {
	int status;
	regex_t re;

	if (regcomp(&re, expr_str.c_str(), flags|REG_NOSUB) != 0) {
		errno = EREG_COMP;
		return false;
	}
	status = regexec(&re, pattern.c_str(), 0, NULL, 0);
	regfree(&re);
	if (status != 0) {
		return false;
	}
	
	return true;
}
