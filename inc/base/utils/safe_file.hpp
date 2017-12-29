/*
 * =====================================================================================
 *
 *       Filename:  safe_file.hpp
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2016年04月18日 17时02分22秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *        Company:  
 *
 * =====================================================================================
 */
#ifndef SAFE_FILE_HPP_
#define SAFE_FILE_HPP_

#include <string>
#include <fstream>
#include <sstream>

namespace cppbase {

class SafeIFile {
public:
	~SafeIFile()
	{
		if (input.is_open()) {
			input.close();
		}
	}

	bool open(const char *file_name, std::ios_base::openmode mode)
	{
		input.open(file_name, mode);
		return input.is_open();
	}
	
	bool open(const char *file_name)
	{
		return open(file_name, std::ofstream::in);
	}

	bool open(const std::string &file_name)
	{
		return open(file_name.c_str());
	}

	bool open(const std::string file_name, std::ios_base::openmode mode)
	{
		return open(file_name.c_str(), mode|std::ofstream::in);
	}

	std::istream & getline(std::string &str)
	{
		return std::getline(input, str);
	}

	bool read_whole_file(std::string &data)
	{
		if (!input.is_open()) {
			return false;
		}
	
		std::ostringstream ss;
		ss << input.rdbuf();
		data = ss.str();

		return true;
	}
	
	template <typename T>
	std::ostream& operator>> (T t)
	{
		return t>>input;
	}

	std::ifstream& get_ifstream()
	{
		return input;
	}
	
private:
	std::ifstream input;	
};

class SafeOFile {
public:
	~SafeOFile() 
	{
		if (of.is_open()) {
			of.close();
		}
	}

	bool open(const char *file_name)
	{
		if (of.is_open()) {
			of.close();
		}
	
		return open(file_name, std::ofstream::out);
	}

	bool open(const std::string &file_name) 
	{
		return open(file_name.c_str());
	}

	bool open(const char *file_name, std::ios_base::openmode mode)
	{
		of.open(file_name, mode|std::ofstream::out);
		return of.is_open();
	}

	bool open(const std::string &file_name, std::ios_base::openmode mode)
	{
		return open(file_name.c_str(), mode);
	}

	template <typename T>
	std::ostream& operator<< (T t)
	{
		return of << t;
	}

	uint32_t get_current_pos(void)
	{
		return of.tellp();
	}

	void close(void)
	{
		of.close();
	}
	
private:
	std::ofstream of;
};

}

#endif

