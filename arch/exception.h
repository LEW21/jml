/* exception.h                                                     -*- C++ -*-
   Jeremy Barnes, 26 January 2005
   Copyright (c) 2005 Jeremy Barnes.  All rights reserved.
   
   This file is part of "Jeremy's Machine Learning Library", copyright (c)
   1999-2005 Jeremy Barnes.
   
   This program is available under the GNU General Public License, the terms
   of which are given by the file "license.txt" in the top level directory of
   the source code distribution.  If this file is missing, you have no right
   to use the program; please contact the author.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
   for more details.

   ---

   Defines our exception class.
*/

#ifndef __utils__exception_h__
#define __utils__exception_h__

#include <string>
#include <exception>

namespace ML {

class Exception : public std::exception {
public:
    Exception(const std::string & msg);
    Exception(const char * msg, ...);
    Exception(int errnum, const std::string & msg, const char * function = 0);
    virtual ~Exception() throw();
    
    virtual const char * what() const throw();
    
private:
    std::string message;
};


} // namespace ML


#endif /* __utils__exception_h__ */