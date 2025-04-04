#include "stream.h"
#include <algorithm>

namespace sylar {
    char BorrowedStream::get() {
        if (bufempty()) {
            index_end_ = index_in_ = 0;
            fillbuf();
        }
        char c = buffer_in_[index_in_];
        ++index_in_;
        return c;
    }
    void BorrowedStream::get(std::span<char> s) {
        auto* p = s.data();
        auto n = s.size();
        std::size_t start = index_in_;
        while (true) {
            auto end = start + n;
            if (end <= index_end_) {
                p = std::copy(buffer_in_.data() + start, buffer_in_.data() + end, p);
                index_in_ = end;
                return;
            }
            p = std::copy(buffer_in_.data() + start, buffer_in_.data() + index_end_, p);
            index_end_ = index_in_ = 0;
            fillbuf();
            start = 0;
        }
    }
    void BorrowedStream::getline(std::string& s, char eol) {
        std::size_t start = index_in_;
        while (true) {
            for (std::size_t i = start; i < index_end_; ++i) {
                if (buffer_in_[i] == eol) {
                    s.append(buffer_in_.data() + start, i - start);
                    index_in_ = i + 1;
                    return;
                }
            }
            s.append(buffer_in_.data() + start, index_end_ - start);
            index_end_ = index_in_ = 0;
            fillbuf();
            start = 0;
        }
    }
    void BorrowedStream::getall(std::string& s) {
        std::size_t start = index_in_;
        try {
            while (true) {
                s.append(buffer_in_.data() + start, index_end_ - start);
                start = 0;
                index_end_ = index_in_ = 0;
                fillbuf();
            }
        } catch (std::system_error& e) {
            if (e.code() == Stream::eof()) {
                return;
            }
            throw;
        }
    }
    std::string BorrowedStream::getsome() {
        if (bufempty()) {
            index_end_ = index_in_ = 0;
            fillbuf();
        }
        auto buf = peek();
        std::string ret(buf.data(), buf.size());
        seenbuf(buf.size());
        return ret;
    }

    void BorrowedStream::ingore(std::size_t n) {
        auto start = index_in_;
        while (true) {
            auto end = start + n;
            if (end <= index_end_) {
                index_in_ = end;
                return;
            }
            auto m = index_end_ - index_in_;
            n -= m;
            index_end_ = index_in_ = 0;
            fillbuf();
            start = 0;
        }
    }
    void BorrowedStream::ignore(char eol) {
        std::size_t start = index_in_;
        while (true) {
            for (std::size_t i = start; i < index_end_; ++i) {
                if (buffer_in_[i] == eol) {
                    index_in_ = i + 1;
                    return;
                }
            }
            index_end_ = index_in_ = 0;
            fillbuf();
            start = 0;
        }
    }
    void BorrowedStream::ignore_all() {
        try {
            while (true) {
                index_end_ = index_in_ = 0;
                fillbuf();
            }
        } catch (std::system_error& e) {
            if (e.code() == Stream::eof()) [[unlikely]] {
                return;
            }
            throw;
        }
    }

    void BorrowedStream::put(char c) {
        if (buffull()) {
            flush();
        }
        buffer_out_[index_out_] = c;
        ++index_out_;
    }
    void BorrowedStream::put(std::span<char const> s) {
        const auto* p = s.data();
        const auto* const pe = s.data() + s.size();
        while (true) {
            if (static_cast<std::size_t>(pe - p) <= buffer_out_.size() - index_out_) {
                auto* b = buffer_out_.data() + index_out_;
                index_out_ += static_cast<std::size_t>(pe - p);
                std::memcpy(b, p, static_cast<size_t>(pe - p));
                break;
            }
            auto* b = buffer_out_.data() + index_out_;
            auto* const be = buffer_out_.data() + buffer_out_.size();
            index_out_ = buffer_out_.size();
            std::memcpy(b, p, static_cast<size_t>(be - b));
            flush();
            index_out_ = 0;
        }
    }
    void BorrowedStream::flush() {
        if (!buffer_out_) {
            alloc_bufout(STREAM_BUFFER_SIZE);
            return;
        }
        if (index_out_) [[likely]] {
            auto buf = std::span(buffer_out_.data(), index_out_);
            auto len = stream_->raw_write(buf);
            while (len > 0 && len != buf.size()) {
                buf = buf.subspan(len);
                len = stream_->raw_write(buf);
            }
            if (len == 0) [[unlikely]] {
                throw std::system_error(Stream::eof());
            }
            index_out_ = 0;
            stream_->raw_flush();
        }
    }

    void BorrowedStream::fillbuf() {
        if (!buffer_in_) {
            alloc_bufin(STREAM_BUFFER_SIZE);
        }
        auto n = stream_->raw_read(std::span(buffer_in_.data() + index_in_, buffer_in_.size() - index_in_));
        if (n == 0) [[unlikely]] {
            throw std::system_error(Stream::eof());
        }
        index_end_ = index_in_ + n;
    }

} // namespace sylar
