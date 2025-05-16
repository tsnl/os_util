#include <tsnl/log.hpp>
#include <tsnl/os_util/memory_mapped_file.hpp>

#include <utility>

#if TSNL_PLATFORM_WIN32
#include <errhandlingapi.h>
#include <fileapi.h>
#include <winbase.h>
#include <windows.h>

namespace tsnl::os_util {

memory_mapped_file::memory_mapped_file(std::filesystem::path const& file_path, access_mode mode) {
    int desired_access = 0;
    desired_access |= mode & access_mode::read ? GENERIC_READ : 0;
    desired_access |= mode & access_mode::write ? GENERIC_WRITE : 0;
    if (!desired_access) {
        log::fatal() << "No access mode specified for file: " << file_path.string();
        std::unreachable();
    }

    CreateFileA(file_path.string().c_str(), desired_access, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file_handle == INVALID_HANDLE_VALUE) {
        log::fatal() << "Failed to open file: " << file_path.string() << ": error: " << GetLastError();
        std::unreachable();
    }

    int64_t file_size;
    bool file_size_ok = GetFileSizeEx(file_handle, (PLARGE_INTEGER)&length);
    if (!file_size_ok) {
        log::fatal() << "Failed to get file size: " << file_path.string() << ": error: " << GetLastError();
        std::unreachable();
    }

    int protection = 0;
    if (mode & access_mode::write) {
        protection |= PAGE_READWRITE;
    } else if (mode & access_mode::read) {
        protection |= PAGE_READONLY;
    }
    if (!protection) {
        log::fatal() << "No access mode specified for file: " << file_path.string();
        std::unreachable();
    }

    if (mode & access_mode::commit) {
        protection |= SEC_COMMIT;
    }

    HANDLE file_mapping_object_handle = CreateFileMappingA(file_handle, NULL, protection, 0, 0, NULL);
    if (file_mapping_object_handle == INVALID_HANDLE_VALUE) {
        log::fatal() << "Failed to create file mapping object: " << file_path.string() << ": error: " << GetLastError();
        std::unreachable();
    }

    void* ptr = MapViewOfFile(file_mapping_object_handle, FILE_MAP_READ, 0, 0, 0);
    if (ptr == NULL) {
        log::fatal() << "Failed to map view of file: " << file_path.string() << ": error: " << GetLastError();
        std::unreachable();
    }

    file_handle_ = file_handle;
    file_mapping_object_handle_ = file_mapping_object_handle;
    mapping_base_address_ = ptr;
}

memory_mapped_file::~memory_mapped_file() {
    if (mapping_base_address_ != nullptr) {
        UnmapViewOfFile(mapping_base_address_);
    }
    if (file_mapping_object_handle_ != nullptr) {
        CloseHandle(file_mapping_object_handle_);
    }
    if (file_handle_ != nullptr) {
        CloseHandle(file_handle_);
    }
}

memory_mapped_file::memory_mapped_file(memory_mapped_file&& other) {
    file_handle_ = other.file_handle_;
    file_mapping_object_handle_ = other.file_mapping_object_handle_;
    mapping_base_address_ = other.mapping_base_address_;
    file_size_ = other.file_size_;

    other.file_handle_ = nullptr;
    other.file_mapping_object_handle_ = nullptr;
    other.mapping_base_address_ = nullptr;
}

auto memory_mapped_file::operator=(memory_mapped_file&& other) -> memory_mapped_file& {
    if (this != &other) {
        file_handle_ = other.file_handle_;
        file_mapping_object_handle_ = other.file_mapping_object_handle_;
        mapping_base_address_ = other.mapping_base_address_;
        file_size_ = other.file_size_;

        other.file_handle_ = nullptr;
        other.file_mapping_object_handle_ = nullptr;
        other.mapping_base_address_ = nullptr;
    }
    return *this;
}

auto memory_mapped_file::size() const -> size_t {
    return file_size_;
}

auto memory_mapped_file::access() -> std::span<std::byte> {
    return std::span(reinterpret_cast<std::byte*>(mapping_base_address_), file_size_);
}

auto memory_mapped_file::access() const -> std::span<std::byte const> {
    return std::span(reinterpret_cast<std::byte const*>(mapping_base_address_), file_size_);
}

} // namespace tsnl::os_util

#elif TSNL_PLATFORM_POSIX

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cstring>

namespace tsnl::os_util {

memory_mapped_file::memory_mapped_file(std::filesystem::path const& file_path, uint32_t flags) {
    int open_flags = 0;
    if ((flags & flag::read) && (flags & flag::write)) {
        open_flags |= O_RDWR;
    } else if (flags & flag::read) {
        open_flags |= O_RDONLY;
    } else if (flags & flag::write) {
        open_flags |= O_WRONLY;
    } else {
        log::fatal() << "No access mode specified for file: " << file_path.string();
        std::unreachable();
    }

    int fd = open(file_path.string().c_str(), open_flags);
    if (fd < 0) {
        log::fatal() << "Failed to open file: " << file_path.string() << ": error: " << strerror(errno);
        std::unreachable();
    }

    struct stat st;
    if (fstat(fd, &st) < 0) {
        log::fatal() << "Failed to get file size: " << file_path.string() << ": error: " << strerror(errno);
        std::unreachable();
    }
    if (st.st_size == 0) {
        log::fatal() << "File is empty: " << file_path.string();
        std::unreachable();
    }

    int prot = 0;
    prot |= flags & flag::read ? PROT_READ : 0;
    prot |= flags & flag::write ? PROT_WRITE : 0;
    if (!prot) {
        log::fatal() << "No access mode specified for file: " << file_path.string();
        std::unreachable();
    }

    int mmap_flags = MAP_PRIVATE;

    void* ptr = mmap(nullptr, st.st_size, prot, mmap_flags, fd, 0);
    if (ptr == MAP_FAILED) {
        log::fatal() << "Failed to map view of file: " << file_path.string() << ": error: " << strerror(errno);
        std::unreachable();
    }

    if (flags & flag::commit) {
        mlock(ptr, st.st_size);
    }

    int ec = close(fd);
    if (ec < 0) {
        log::fatal() << "Failed to close file descriptor: " << file_path.string() << ": error: " << strerror(errno);
        std::unreachable();
    }

    mapping_base_address_ = ptr;
    file_size_ = st.st_size;
}

memory_mapped_file::~memory_mapped_file() {
    if (mapping_base_address_ != nullptr) {
        munmap(mapping_base_address_, file_size_);
    }
}

memory_mapped_file::memory_mapped_file(memory_mapped_file&& other) {
    mapping_base_address_ = other.mapping_base_address_;
    file_size_ = other.file_size_;

    other.mapping_base_address_ = nullptr;
}

auto memory_mapped_file::operator=(memory_mapped_file&& other) -> memory_mapped_file& {
    if (this != &other) {
        mapping_base_address_ = other.mapping_base_address_;
        file_size_ = other.file_size_;

        other.mapping_base_address_ = nullptr;
    }
    return *this;
}

auto memory_mapped_file::size() const -> size_t {
    return file_size_;
}

auto memory_mapped_file::access() -> std::span<std::byte> {
    return std::span(reinterpret_cast<std::byte*>(mapping_base_address_), file_size_);
}

auto memory_mapped_file::access() const -> std::span<std::byte const> {
    return std::span(reinterpret_cast<std::byte const*>(mapping_base_address_), file_size_);
}

} // namespace tsnl::os_util

#endif
