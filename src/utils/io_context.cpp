#ifndef DOCTEST_CONFIG_DISABLE

#include "io_context.hpp"

#include <doctest/doctest.h>

#include <array>

#include <cerrno>
#include <cstdio>

#include <fcntl.h>
#include <sys/fcntl.h>
#include <unistd.h>

IOContext::IOContext() {
  REQUIRE_NE(EOF, fflush(stdout));
  REQUIRE_NE(EOF, fflush(stderr));
  std::array files{stdin, stdout, stderr};
  for (unsigned fd_no = 0; fd_no < 3; ++fd_no) {
    [[maybe_unused]] int fd;
    mapping_[fd_no] = fileno(files[fd_no]);
    saved_[fd_no] = dup(mapping_[fd_no]);
    REQUIRE_NE(-1, pipe(pipes_[fd_no]));
    REQUIRE_NE(-1, fcntl(pipes_[fd_no][0], F_SETFL, O_NONBLOCK));
    REQUIRE_NE(-1, fcntl(pipes_[fd_no][1], F_SETFL, O_NONBLOCK));
    fd = pipes_[fd_no][fd_no != 0];
    REQUIRE_NE(-1, dup2(fd, mapping_[fd_no]));
    REQUIRE_NE(-1, close(fd));
  }
}

IOContext::~IOContext() {
  REQUIRE_NE(EOF, fflush(stdout));
  REQUIRE_NE(EOF, fflush(stderr));
  for (unsigned fd_no = 0; fd_no < 3; ++fd_no) {
    [[maybe_unused]] int fd = pipes_[fd_no][fd_no == 0];
    REQUIRE_NE(-1, close(fd));
    REQUIRE_NE(-1, dup2(saved_[fd_no], mapping_[fd_no]));
    REQUIRE_NE(-1, close(saved_[fd_no]));
  }
}

[[nodiscard]] std::string IOContext::StdOut() const {
  REQUIRE_NE(EOF, fflush(stdout));
  std::string out;
  std::array<char, 32> buf;
  while (true) {
    if (long sz = read(pipes_[STDOUT_FILENO][0], buf.data(), buf.size()); sz > 0) {
      out += std::string_view{buf.data(), buf.data() + sz};
    } else if (sz == -1 and errno == EAGAIN) {
      break;
    }
  }
  return out;
}

[[nodiscard]] std::string IOContext::StdErr() const {
  REQUIRE_NE(EOF, fflush(stderr));
  std::string out;
  std::array<char, 32> buf;
  while (true) {
    if (long sz = read(pipes_[STDERR_FILENO][0], buf.data(), buf.size()); sz > 0) {
      out += std::string_view{buf.data(), buf.data() + sz};
    } else if (sz == -1 and errno == EAGAIN) {
      break;
    }
  }
  return out;
}

void IOContext::StdIn(std::string_view data) const {
  REQUIRE_NE(-1, write(pipes_[STDIN_FILENO][1], data.data(), data.size()));
}

#endif
