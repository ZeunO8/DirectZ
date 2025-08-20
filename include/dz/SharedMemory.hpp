#pragma once
#include <string>
#include <cstdint>
#include <cstddef>
#include <utility>
#if defined(_WIN32)
#define NOMINMAX
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#endif

namespace dz
{
    struct SharedMemory
    {
        std::string name;
        void *addr;
        size_t mapped_size;
        size_t object_size;
        bool read_only;
        int last_error;
#if defined(_WIN32)
        HANDLE hMap;
#else
        int fd;
        bool creator;
#endif

        SharedMemory()
            : name(), addr(nullptr), mapped_size(0), object_size(0), read_only(false), last_error(0)
#if defined(_WIN32)
              ,
              hMap(nullptr)
#else
              ,
              fd(-1), creator(false)
#endif
        {
        }

        ~SharedMemory()
        {
            Close();
        }

        SharedMemory(SharedMemory &&o) noexcept
            : name(std::move(o.name)), addr(o.addr), mapped_size(o.mapped_size), object_size(o.object_size), read_only(o.read_only), last_error(o.last_error)
#if defined(_WIN32)
              ,
              hMap(o.hMap)
#else
              ,
              fd(o.fd), creator(o.creator)
#endif
        {
            o.addr = nullptr;
            o.mapped_size = 0;
            o.object_size = 0;
            o.read_only = false;
            o.last_error = 0;
#if defined(_WIN32)
            o.hMap = nullptr;
#else
            o.fd = -1;
            o.creator = false;
#endif
        }

        SharedMemory &operator=(SharedMemory &&o)
        {
            if (this != &o)
            {
                Close();
                name = std::move(o.name);
                addr = o.addr;
                mapped_size = o.mapped_size;
                object_size = o.object_size;
                read_only = o.read_only;
                last_error = o.last_error;
#if defined(_WIN32)
                hMap = o.hMap;
                o.hMap = nullptr;
#else
                fd = o.fd;
                creator = o.creator;
                o.fd = -1;
                o.creator = false;
#endif
                o.addr = nullptr;
                o.mapped_size = 0;
                o.object_size = 0;
                o.read_only = false;
                o.last_error = 0;
            }
            return *this;
        }

        static std::string NormalizeName(const std::string &n)
        {
#if defined(_WIN32)
            return n;
#else
            if (n.empty())
                return "/sm";
            if (n[0] != '/')
                return "/" + n;
            return n;
#endif
        }

#if defined(_WIN32)
        static std::wstring ToWide(const std::string &s)
        {
            if (s.empty())
                return std::wstring();
            int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0);
            std::wstring w;
            w.resize(len);
            if (len)
                MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), w.data(), len);
            return w;
        }
#endif

        bool Create(const std::string &shm_name, size_t size)
        {
            Close();
            name = NormalizeName(shm_name);
            object_size = size;
            read_only = false;
            mapped_size = 0;
            addr = nullptr;
            last_error = 0;
            if (size == 0)
            {
                last_error = EINVAL;
                return false;
            }
#if defined(_WIN32)
            std::wstring w = ToWide(name);
            hMap = CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, static_cast<DWORD>((uint64_t)size >> 32), static_cast<DWORD>(size & 0xFFFFFFFFu), w.c_str());
            if (!hMap)
            {
                last_error = (int)GetLastError();
                return false;
            }
            return true;
#else
            creator = true;
            fd = shm_open(name.c_str(), O_CREAT | O_EXCL | O_RDWR, 0600);
            if (fd == -1)
            {
                last_error = errno;
                return false;
            }
            if (ftruncate(fd, (off_t)size) != 0)
            {
                last_error = errno;
                ::close(fd);
                fd = -1;
                shm_unlink(name.c_str());
                creator = false;
                return false;
            }
            return true;
#endif
        }

        bool Open(const std::string &shm_name, size_t size_hint)
        {
            Close();
            name = NormalizeName(shm_name);
            object_size = size_hint;
            mapped_size = 0;
            addr = nullptr;
            last_error = 0;
            if (size_hint == 0)
            {
                last_error = EINVAL;
                return false;
            }
#if defined(_WIN32)
            std::wstring w = ToWide(name);
            hMap = OpenFileMappingW(FILE_MAP_ALL_ACCESS, false, w.c_str());
            if (!hMap)
            {
                last_error = (int)GetLastError();
                return false;
            }
            return true;
#else
            creator = false;
            fd = shm_open(name.c_str(), O_RDWR, 0600);
            if (fd == -1)
            {
                last_error = errno;
                return false;
            }
            struct stat st;
            if (fstat(fd, &st) == 0 && st.st_size > 0)
                object_size = (size_t)st.st_size;
            return true;
#endif
        }

        bool Map(size_t length = 0, size_t offset = 0, bool as_read_only = false)
        {
            if (addr)
                return true;
            if (length == 0)
                length = object_size;
            if (length == 0)
            {
                last_error = EINVAL;
                return false;
            }
            read_only = as_read_only;
#if defined(_WIN32)
            DWORD acc = as_read_only ? FILE_MAP_READ : FILE_MAP_ALL_ACCESS;
            SIZE_T offLow = static_cast<SIZE_T>(offset & 0xFFFFFFFFu);
            DWORD offHigh = static_cast<DWORD>(((uint64_t)offset) >> 32);
            void *p = MapViewOfFile(hMap, acc, offHigh, (DWORD)offLow, length);
            if (!p)
            {
                last_error = (int)GetLastError();
                return false;
            }
            addr = p;
            mapped_size = length;
            return true;
#else
            int prot = as_read_only ? PROT_READ : (PROT_READ | PROT_WRITE);
            long page = ::sysconf(_SC_PAGESIZE);
            if (page <= 0)
                page = 4096;
            size_t page_sz = (size_t)page;
            size_t aligned_off = (offset / page_sz) * page_sz;
            size_t delta = offset - aligned_off;
            size_t map_len = length + delta;
            void *p = mmap(nullptr, map_len, prot, MAP_SHARED, fd, (off_t)aligned_off);
            if (p == MAP_FAILED)
            {
                last_error = errno;
                return false;
            }
            addr = (void *)((uintptr_t)p + delta);
            mapped_size = length;
            return true;
#endif
        }

        bool Unmap()
        {
            if (!addr)
                return true;
#if defined(_WIN32)
            void *base = addr;
            addr = nullptr;
            SIZE_T len = mapped_size;
            mapped_size = 0;
            if (!UnmapViewOfFile(base))
            {
                last_error = (int)GetLastError();
                return false;
            }
            return true;
#else
            long page = ::sysconf(_SC_PAGESIZE);
            if (page <= 0)
                page = 4096;
            size_t page_sz = (size_t)page;
            uintptr_t paddr = (uintptr_t)addr;
            uintptr_t base = paddr - (paddr % page_sz);
            size_t delta = paddr - base;
            size_t map_len = mapped_size + delta;
            void *base_ptr = (void *)base;
            addr = nullptr;
            mapped_size = 0;
            if (munmap(base_ptr, map_len) != 0)
            {
                last_error = errno;
                return false;
            }
            return true;
#endif
        }

        bool Close()
        {
            bool ok = true;
            if (addr)
                ok = Unmap() && ok;
#if defined(_WIN32)
            if (hMap)
            {
                if (!CloseHandle(hMap))
                {
                    last_error = (int)GetLastError();
                    ok = false;
                }
                hMap = nullptr;
            }
#else
            if (fd != -1)
            {
                if (::close(fd) != 0)
                {
                    last_error = errno;
                    ok = false;
                }
                fd = -1;
            }
            creator = false;
#endif
            name.clear();
            object_size = 0;
            read_only = false;
            return ok;
        }

        bool Destroy()
        {
#if defined(_WIN32)
            return true;
#else
            if (name.empty())
                return true;
            if (shm_unlink(name.c_str()) != 0)
            {
                last_error = errno;
                return false;
            }
            return true;
#endif
        }

        void *Data() const
        {
            return addr;
        }

        size_t Size() const
        {
            return mapped_size ? mapped_size : object_size;
        }

        bool Valid() const
        {
#if defined(_WIN32)
            return hMap != nullptr;
#else
            return fd != -1;
#endif
        }
    };
}