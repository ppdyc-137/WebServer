#pragma once

#include <cstddef>
#include <cstdlib>
#include <span>
#include <utility>

namespace sylar {
    struct BytesBuffer {
    public:
        BytesBuffer() noexcept = default;
        explicit BytesBuffer(std::size_t size) : data_(static_cast<char*>(valloc(size))), size_(size) {}

        BytesBuffer(BytesBuffer&& that) noexcept
            : data_(std::exchange(that.data_, nullptr)), size_(std::exchange(that.size_, 0)) {}

        BytesBuffer& operator=(BytesBuffer&& that) noexcept {
            if (this == &that) {
                return *this;
            }

            free(data_);
            data_ = std::exchange(that.data_, nullptr);
            size_ = std::exchange(that.size_, 0);
            return *this;
        }

        ~BytesBuffer() noexcept { free(data_); }

        void allocate(std::size_t size) {
            data_ = static_cast<char*>(valloc(size));
            size_ = size;
        }

        char* data() const noexcept { return data_; }
        std::size_t size() const noexcept { return size_; }

        char& operator[](std::size_t index) const noexcept { return data_[index]; }
        explicit operator bool() const noexcept { return static_cast<bool>(data_); }
        explicit operator std::span<char>() const noexcept { return {data(), size()}; }

    private:
        char* data_{};
        std::size_t size_{};
    };

} // namespace sylar
