#pragma once

#include "bytes_buffer.h"

#include <cstddef>
#include <cstdint>
#include <cstring>

#include <memory>
#include <span>
#include <system_error>

namespace sylar {

    inline constexpr std::size_t STREAM_BUFFER_SIZE = 8192;

    struct Stream {
        struct EOFException : std::exception {
            const char* what() const noexcept override { return "End of file"; }
        };

        virtual void raw_seek(std::uint64_t /*unused*/) {
            throw std::system_error(std::make_error_code(std::errc::invalid_seek));
        }

        virtual void raw_flush() {}

        virtual void raw_close() {}

        virtual std::size_t raw_read(std::span<char> /*unused*/) {
            throw std::system_error(std::make_error_code(std::errc::not_supported));
        }

        virtual std::size_t raw_write(std::span<char const> /*unused*/) {
            throw std::system_error(std::make_error_code(std::errc::not_supported));
        }

        Stream& operator=(Stream&&) = delete;
        virtual ~Stream() = default;
    };

    struct BorrowedStream {
        BorrowedStream() : stream_() {}

        explicit BorrowedStream(Stream* raw) : stream_(raw) {}

        virtual ~BorrowedStream() = default;
        BorrowedStream(BorrowedStream&&) = default;
        BorrowedStream& operator=(BorrowedStream&&) = default;

        char get();
        void get(std::span<char> s);
        void get(std::string& s, std::size_t n) {
            s.resize(s.size() + n);
            get({s.data() - n, n});
        }
        std::string get(std::size_t n) {
            std::string s;
            get(s, n);
            return s;
        }

        void getline(std::string& s, char eol);
        std::string getline(char eol) {
            std::string s;
            getline(s, eol);
            return s;
        }

        void getall(std::string& s);
        std::string getall() {
            std::string s;
            getall(s);
            return s;
        }

        void ingore(std::size_t n);
        void ignore(char eol);
        void ignore_all();

        void put(char c);
        void put(std::span<char const> s);
        void putline(std::string_view s) {
            put(s);
            put('\n');
            flush();
        }

        std::span<char const> peek() const noexcept { return {buffer_in_.data() + index_in_, index_end_ - index_in_}; }
        std::string getsome();

        void seek(std::uint64_t pos) {
            stream_->raw_seek(pos);
            index_in_ = 0;
            index_end_ = 0;
            index_out_ = 0;
        }
        void flush();
        void close() { stream_->raw_close(); }

        Stream& raw() const noexcept { return *stream_; }
        template <std::derived_from<Stream> Derived>
        Derived& raw() const {
            return dynamic_cast<Derived&>(*stream_);
        }

    private:
        void fillbuf();
        void seenbuf(std::size_t n) noexcept { index_in_ += n; }

        void alloc_bufin(std::size_t size) {
            if (!buffer_in_) [[likely]] {
                buffer_in_.allocate(size);
                index_in_ = 0;
                index_end_ = 0;
            }
        }
        void alloc_bufout(std::size_t size) {
            if (!buffer_out_) [[likely]] {
                buffer_out_.allocate(size);
                index_out_ = 0;
            }
        }

        bool bufempty() const noexcept { return index_in_ == index_end_; }
        bool buffull() const noexcept { return index_out_ == buffer_out_.size(); }

        BytesBuffer buffer_in_;
        std::size_t index_in_ = 0;
        std::size_t index_end_ = 0;
        BytesBuffer buffer_out_;
        std::size_t index_out_ = 0;
        Stream* stream_;
    };

    struct OwningStream : BorrowedStream {
        explicit OwningStream() = default;
        explicit OwningStream(std::unique_ptr<Stream> raw) : BorrowedStream(raw.get()), stream_(std::move(raw)) {}

        std::unique_ptr<Stream> release() noexcept { return std::move(stream_); }

    private:
        std::unique_ptr<Stream> stream_;
    };

    template <std::derived_from<Stream> Stream, class... Args>
    OwningStream make_stream(Args&&... args) {
        return OwningStream(std::make_unique<Stream>(std::forward<Args>(args)...));
    }

} // namespace sylar
