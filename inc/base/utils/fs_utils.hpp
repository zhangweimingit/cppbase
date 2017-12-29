#ifndef FS_UTILS_HPP_
#define FS_UTILS_HPP_

#include <algorithm>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <limits>
#include <cassert>

#include <stdint.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>

#include "scope_guard.hpp"

namespace cppbase {
  namespace fs {

  int64_t file_size(const char* file_name)
  {
      struct stat buf;
      if (stat(file_name, &buf) == -1) return -1;

      return buf.st_size;
  }

  template <typename Container>
  inline bool read_file(const char* file_name, Container& out,
                 int64_t num_bytes = std::numeric_limits<int64_t>::max())
  {
      static_assert(sizeof(out[0]) == 1, "container with byte-sized elements");
      using std::ifstream;

      ifstream ifs(file_name, std::ios_base::ate);
      if (!ifs) {
          return false;
      }

      int64_t filesize = file_size(file_name);
      assert(filesize > 0);

      auto readsize = std::min<int64_t>(filesize, num_bytes);
      out.resize(readsize);

      ifs.seekg(0);
      if (!ifs.read(&out[0], readsize)) {
          return false;
      }

      return true;
  }

  template <typename Container>
  inline bool read_file(int fd, Container& out,
                 int64_t num_bytes = std::numeric_limits<int64_t>::max())
  {
      static_assert(sizeof(out[0]) == 1, "container with byte-sized elements");
      int64_t cur_read = 0;
      struct stat buf;
      if (fstat(fd, &buf) == -1) return false;

      assert(num_bytes > 0);
      out.resize(std::min<int64_t>(buf.st_size, num_bytes));

      while (cur_read < out.size()) {
          const auto sz = read(fd, &out[0] + cur_read, out.size() - cur_read);
          if (sz == -1) {
              if (errno == EINTR) {
                  continue;
              }
              return false;
          }

          if (sz == 0) {
              break;
          }
          cur_read += sz;
      }

      return true;
  }

  template <class UnaryFunction>
  inline void for_each_line(int fd, UnaryFunction f)
  {
      std::string buf;
      if (!read_file(fd, buf)) {
          return;
      }

      std::istringstream iss(buf);
      std::string line;
      while (std::getline(iss, line))
      {
          f(line);
      }
  }

  template <class Container>
  inline bool write_file(int fd, const Container& data,
                  int flags = O_WRONLY | O_TRUNC)
  {
      auto total_bytes = data.size();
      decltype(total_bytes) cur_write = 0;

      while (cur_write < total_bytes) {
          const auto sz = write(fd, &data[0] + cur_write, total_bytes - cur_write);
          if (sz == -1) {
              if (errno == EINTR) {
                  continue;
              }
              return false;
          }
          if (sz == 0) {
              break;
          }
          cur_write += sz;
      }

      return true;
  }

  template <class Container>
  inline bool write_file(const char* file_name, const Container& data,
                  int flags = O_WRONLY | O_CREAT | O_TRUNC)
  {
      int fd = open(file_name, flags, 0666);
      if (fd == -1) {
          return false;
      }

      SCOPE_EXIT { close(fd); };
      return write_file(fd, data, flags);
  }

  bool remove_file(const char* file_name)
  {
      return remove(file_name) == 0;
  }

  bool isdir(const char* path)
  {
      struct stat statbuf;
      if (stat(path, &statbuf) != 0)
          return false;

      return S_ISDIR(statbuf.st_mode);
  }

  // support recursive dir list
  void get_dir_files(const char* path, std::vector<std::string>& files)
  {
      if (!isdir(path)) return;

      auto dir = opendir(path);
      if (dir == NULL) {
          return;
      }
      chdir(path);

      SCOPE_EXIT {
          closedir(dir);
          chdir("..");
      };

      struct dirent* ent = NULL;
      while ((ent = readdir(dir))) {
          if (ent->d_type & DT_REG) {
              files.push_back(ent->d_name);
          } else if (ent->d_type & DT_DIR) {
              if (strncmp(".", ent->d_name, 1) == 0 ||
                  strncmp("..", ent->d_name, 2) == 0) {
                  continue;
              }

              std::vector<std::string> c;
              get_dir_files(ent->d_name, c);
              files.insert(files.end(), c.begin(), c.end());
          }
      }
  }

  bool create_dir(const char *dir)
  {
    if (-1 == mkdir(dir, 0777) && errno != EEXIST) {
        return false;
    }

    return true;
  }


  } // namespace fs
} // namespace cppbase

#endif
