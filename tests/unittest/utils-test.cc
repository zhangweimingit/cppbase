#include "unittest.hpp"
#include "utils/utils.hpp"
#include "utils/fs_utils.hpp"
#include "timestamp.hpp"

#include <set>
#include <string>
#include <fstream>
#include <iterator>
#include <algorithm>

#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>


using std::string;

TEST(UtilTest, TimeStamp) {
	ASSERT_GT(cppbase::TimeStamp::get_cur_secs(), 0);
}

TEST(UtilTest, Str2Lower) {
	string str = "IKUAI8.COM";
	cppbase::StrToLower(str);	
	EXPECT_EQ("ikuai8.com", str);
}

TEST(UtilTest, Str2Upper) {
	string str = "ikuai8.com";
	cppbase::StrToUpper(str);
	EXPECT_EQ("IKUAI8.COM", str);
}

TEST(UtilTest, Dir) {
	int stamp = (int)time(NULL);
	string dir = "/tmp/" + std::to_string(stamp);
    cppbase::create_dir(dir);

	struct stat statbuf;
	EXPECT_EQ(stat(dir.c_str(), &statbuf), 0);
	EXPECT_EQ(S_ISDIR(statbuf.st_mode), 1);

	std::set<string> files;
	for (int i = 0; i < 10; ++i) {
		std::fstream fs;
		fs.open(dir + "/" + std::to_string(i), std::ios::out);
		fs << "##test line";
		fs.close();
		files.insert(std::to_string(i));
	}

//	std::generate_n(std::inserter(files, files.begin()), 10, [&i]() { return std::to_string(i++); });
	std::vector<string> ls_files;
	cppbase::get_dir_files(dir, ls_files);
	for (auto iter = ls_files.begin(); iter != ls_files.end(); ++iter) {
		files.erase(*iter);
		::remove((dir + "/" + *iter).c_str());
	}

	EXPECT_EQ(files.size(), 0);

	::remove(dir.c_str());
}
