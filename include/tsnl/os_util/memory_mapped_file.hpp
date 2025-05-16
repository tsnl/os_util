#pragma once

#include <tsnl/os_util/platform.hpp>

#include <filesystem>
#include <span>

#include <cstdlib>

namespace tsnl::os_util {

/// Memory-mapped file access to an existing file.
class memory_mapped_file {
public:
    struct flag {
        static constexpr uint32_t read = 0x1;
        static constexpr uint32_t write = 0x2;
        static constexpr uint32_t commit = 0x4;

        flag() = delete;
        flag(flag const&) = delete;
        flag(flag&&) = delete;
    };

    explicit memory_mapped_file(std::filesystem::path const& file_path, uint32_t flags = flag::read);
    ~memory_mapped_file();

    memory_mapped_file(memory_mapped_file const&) = delete;
    memory_mapped_file(memory_mapped_file&&);

    auto operator=(memory_mapped_file const&) -> memory_mapped_file& = delete;
    auto operator=(memory_mapped_file&&) -> memory_mapped_file&;

    [[nodiscard]] auto size() const -> size_t;

    [[nodiscard]] auto access() -> std::span<std::byte>;
    [[nodiscard]] auto access() const -> std::span<std::byte const>;

private:
#if TSNL_PLATFORM_WIN32
    using HANDLE = void*;

    HANDLE file_handle_;
    HANDLE file_mapping_object_handle_;
    void* mapping_base_address_;
    size_t file_size_;
#endif

#if TSNL_PLATFORM_POSIX
    void* mapping_base_address_;
    size_t file_size_;
#endif
};

} // namespace tsnl::os_util
