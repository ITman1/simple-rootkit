/**
  Library name: Regular expression support
  ----------------------------------------------------------------------
  Author:   Radim Loskot
  Date:     18. 2. 2011
  Filename: regexp.h
  Descr:    Extends std library about primitive regular expression operations. 
 */

#include <regex.h>

#include <string>
#include <vector>

using namespace std;

namespace std {
	namespace regexp {
		/** 
		 * An enunumeration of possible errors that can occur 
		 * during working with regular expressions.
		 */
		enum regexp_errors {
			EREG_COMP = 201,	/**< enum error during compilation of regular expression. */  
			EREG_NULL = 202,	/**< enum empty regular expression has been skipped. */  
			EREG_EXEC = 203		/**< enum error during matching string to regular expression. */  
		};
		
		class RegExp {
			
			private:
				string expr_str;	/**< regular expression as a string */  
				regex_t regex;		/**< compiled regular expression  */  
				bool initialized;	/**< Signs whether is regular expression compiled */  
			
			protected:
				
			public:
			
				int flags;
				/** 
				 * String asigment into Regualar expression. Behaves similar to constructor.
				 * @param expr regular expression as string which will be compiled
				 * @return RegExp structure with updated compliled regular expression
				 */
				RegExp& operator=(const string& expr);
				/** 
				 * Checks whether pattern fits to regular expression and parses pattern in depending on subexpression.
				 * @param pattern string which is verified whether is correct to regular expression
				 * @return vector that contains all strings directly depending on number of subexpression in regular expression
				 */
				vector<string> exec(const string pattern);
				/** 
				 * Checks whether pattern fits to regular and returns result.
				 * @param pattern string which is verified whether is correct to regular expression
				 * @return true/false whether pattern fits to regular expression
				 */
				bool test(const string pattern);
				
				/** 
				 * Regular expression constructor.
				 * @param expr regular expression as string which will be compiled
				 * @param optflags some adjustments how should be regular expression evaluated
				 */
				RegExp(const string expr = ".*", int optflags = REG_EXTENDED);
				
				/** 
				 * Regular expression destructor.
				 */
				~RegExp() { if (initialized) regfree(&regex);}
				
		};
	}
}
