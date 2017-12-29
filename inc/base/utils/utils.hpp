/*
 * =====================================================================================
 *
 *       Filename:  utils.hpp
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  08/05/16 12:16:39
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *        Company:  
 *
 * =====================================================================================
 */
#ifndef UTILS_HPP_
#define UTILS_HPP_

#include <algorithm>
#include <string>

namespace cppbase {

#define ARRAY_SIZE(x)	(sizeof(x)/sizeof(x[0]))

static void inline StrToLower(std::string &str)
{
	std::transform(str.begin(), str.end(), str.begin(), ::tolower);
}

static void inline StrToUpper(std::string &str)
{
	std::transform(str.begin(), str.end(), str.begin(), ::toupper);
}

}

#endif

