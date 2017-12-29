#include "base/utils/file.h"
#include "base/utils/scope_guard.hpp"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdexcept>

namespace cppbase {
  namespace fs {
  File::File() : fd_(-1) {}  
  File::File(int fd) : fd_(fd) {}

  File::File(const char* path, int flags, mode_t mode)
      : fd_(::open(path, flags, mode))
  {
      if (fd_ == -1) {
          std::string err(path);
          throw std::runtime_error(err + " open failed, " + strerror(errno));
      }
  }

  File::File(File&& other)
  {
      fd_ = other.fd_;
      other.fd_ = -1; // other's fd is invalid now, we make it guarantee that only one File owns the file's fd
  }

  File::~File()
  { close(); }

  File File::mktemp(const char* template_name)
  {
      std::string name = template_name;
      name += "XXXXXX";

      auto buf = strdup(name.c_str());
      SCOPE_EXIT { free(buf); };

      return File(mkstemp(buf));
  }

  void File::close()
  {
      if (fd_ != -1) {
          ::close(fd_);
      }
  }

  }// namespace fs
}// namespace cppbase
