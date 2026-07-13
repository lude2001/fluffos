#ifndef LEX_STREAM_H
#define LEX_STREAM_H

#include "base/std.h"

#include <cinttypes>
#include <cstddef>
#include <cstdlib>
#include <algorithm>
#include <cstring>
#include <unistd.h>
#include <iostream>
#include <sstream>
#include <string>

class LexStream {
 public:
  LexStream() = default;
  virtual ~LexStream() = default;

  virtual size_t read(char* buffer, size_t size) = 0;
  virtual void close() = 0;
};

class FileLexStream : public LexStream {
 public:
  FileLexStream(int fd) : fd_(fd) {}
  ~FileLexStream() override {}

  size_t read(char* buffer, size_t size) override { return ::read(fd_, buffer, size); }
  void close() override {
    ::close(fd_);
    fd_ = 0;
  };

 private:
  int fd_;
};

class IStreamLexStream : public LexStream {
 public:
  IStreamLexStream(std::istream& is) : is_(is) {}
  ~IStreamLexStream() override {}

  size_t read(char* buffer, size_t size) override { return is_.readsome(buffer, size); }
  void close() override { is_.clear(); };

 private:
  std::istream& is_;
};

class StringLexStream : public LexStream {
 public:
  explicit StringLexStream(std::string data) : data_(std::move(data)) {}
  ~StringLexStream() override {}

  size_t read(char* buffer, size_t size) override {
    size_t available = data_.size() - pos_;
    size_t count = std::min(size, available);
    if (count > 0) {
      std::memcpy(buffer, data_.data() + pos_, count);
      pos_ += count;
    }
    return count;
  }

  void close() override { pos_ = data_.size(); }

 private:
  std::string data_;
  size_t pos_ = 0;
};

#endif /* end of include guard: LEX_STREAM_H */
