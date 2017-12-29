#ifndef CPPBASE_FILESYSTEM_FILE_H_
#define CPPBASE_FILESYSTEM_FILE_H_

#include "noncopyable.hpp"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

namespace cppbase {
  namespace fs {
      class File : public noncopyable {
      public:
          File();
          File(int fd);
          File(const char* path, int flags = O_RDONLY, mode_t mode = 0666);
          File(File&&);

          ~File();

          // create a tmp file
          static File mktemp(const char* name);

          // return the file's fd
          int fd() const { return fd_; }

          // return if File has open one file
          operator bool() const { return fd_ != -1; }

          // close the file
          void close();

      private:
          int fd_;
      };
  }// namespace fs
}// namespace cppbase


#endif
