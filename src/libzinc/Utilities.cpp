#include "Utilities.hpp"

namespace zinc
{

FileMemoryMap::FileMemoryMap()
{
#if _WIN32
    _fd = INVALID_HANDLE_VALUE;
    _mapping = INVALID_HANDLE_VALUE;
#else
    _fd = -1;
#endif
    _size = 0;
    _data = (void *) -1;
}

#if _WIN32

bool FileMemoryMap::open(const char* file_path)
{
    _fd = CreateFile(file_path, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, 0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL,
                     0);
    if (!_fd || _fd == INVALID_HANDLE_VALUE)
        return false;

    DWORD file_size_high = 0;
    _size = GetFileSize(_fd, &file_size_high);

    _mapping = CreateFileMapping(_fd, 0, PAGE_READWRITE, file_size_high, _size, 0);
    if (!_mapping || _mapping == INVALID_HANDLE_VALUE)
        return false;

    _data = MapViewOfFile(_mapping, FILE_MAP_WRITE, 0, 0, _size);
    if (!_data)
        return false;
    return true;
}

void FileMemoryMap::close()
{
    if (_data)
    {
        UnmapViewOfFile(_data);
        _data = 0;
    }
    if (_mapping && _mapping != INVALID_HANDLE_VALUE)
    {
        CloseHandle(_mapping);
        _mapping = INVALID_HANDLE_VALUE;
    }
    if (_fd && _fd != INVALID_HANDLE_VALUE)
    {
        CloseHandle(_fd);
        _fd = INVALID_HANDLE_VALUE;
    }
}

#else
bool FileMemoryMap::open(const char* file_path)
{
    struct stat st = {};
    _fd = ::open(file_path, O_RDWR);

    if (_fd == -1)
        return false;//"FileMapping could not open file");

    if (fstat(_fd, &st) == -1)
        return false;//"FileMapping could not get file size");

    _size = (size_t)st.st_size;

    _data = mmap(0, _size, PROT_READ | PROT_WRITE, MAP_SHARED, _fd, 0);
    if (_data == (void*)-1)
        return false;//"FileMapping could not get map file data");

    return true;
}
/// Close memory mapping.
void FileMemoryMap::close()
{
    if (_data != (void*)-1)
    {
        munmap(_data, _size);
        _data = (void*)-1;
    }

    if (_fd != -1)
    {
        ::close(_fd);
        _fd = -1;
    }
}
#endif

#if _WIN32
std::wstring to_wstring(const std::string &str)
{
    std::wstring result;
    auto needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), str.length(), 0, 0);
    if (needed > 0)
    {
        result.resize(needed);
        MultiByteToWideChar(CP_UTF8, 0, str.c_str(), str.length(), &result.front(), needed);
    }
    return result;
}

int truncate(const char *file_path, int64_t file_size)
{
    HANDLE hFile = CreateFileW(to_wstring(file_path).c_str(), GENERIC_WRITE, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    if (!hFile || hFile == INVALID_HANDLE_VALUE)
        return (int)GetLastError();

    LARGE_INTEGER distance;
    distance.QuadPart = file_size;
    if (SetFilePointerEx(hFile, distance, 0, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
    {
        CloseHandle(hFile);
        return INVALID_SET_FILE_POINTER;
    }

    if (!SetEndOfFile(hFile))
    {
        auto error = GetLastError();
        CloseHandle(hFile);
        return (int)error;
    }

    CloseHandle(hFile);
    return ERROR_SUCCESS;
}
#endif

int64_t round_up_to_multiple(int64_t value, int64_t multiple_of)
{
    auto remainder = value % multiple_of;
    if (value && remainder)
        value += multiple_of - remainder;
    return value;
}

int64_t get_file_size(const char *file_path)
{
#if _WIN32
    struct _stati64 st = {};
    if (_wstat32i64(to_wstring(file_path).c_str(), &st) == 0)
        return st.st_size;
#else
    struct stat64 st = {};
    if (stat64(file_path, &st) == 0)
        return st.st_size;
#endif
    return -1;
}

}