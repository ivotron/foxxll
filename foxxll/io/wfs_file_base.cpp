/***************************************************************************
 *  foxxll/io/wfs_file_base.cpp
 *
 *  Part of FOXXLL. See http://foxxll.org
 *
 *  Copyright (C) 2005 Roman Dementiev <dementiev@ira.uka.de>
 *  Copyright (C) 2008, 2010 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *  Copyright (C) 2009, 2010 Johannes Singler <singler@kit.edu>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <tlx/logger.hpp>

#include <foxxll/common/error_handling.hpp>
#include <foxxll/io/wfs_file_base.hpp>

#if FOXXLL_WINDOWS

#ifndef NOMINMAX
  #define NOMINMAX
#endif
#include <windows.h>

namespace foxxll {

const char* wfs_file_base::io_type() const
{
    return "wfs_base";
}

static HANDLE open_file_impl(const std::string& filename, int mode)
{
    DWORD dwDesiredAccess = 0;
    DWORD dwShareMode = 0;
    DWORD dwCreationDisposition = 0;
    DWORD dwFlagsAndAttributes = 0;

    if (mode & file::RDONLY)
    {
        dwFlagsAndAttributes |= FILE_ATTRIBUTE_READONLY;
        dwDesiredAccess |= GENERIC_READ;
    }

    if (mode & file::WRONLY)
    {
        dwDesiredAccess |= GENERIC_WRITE;
    }

    if (mode & file::RDWR)
    {
        dwDesiredAccess |= (GENERIC_READ | GENERIC_WRITE);
    }

    if (mode & file::CREAT)
    {
        // ignored
    }

    if (mode & file::TRUNC)
    {
        dwCreationDisposition |= TRUNCATE_EXISTING;
    }
    else
    {
        dwCreationDisposition |= OPEN_ALWAYS;
    }

    if (mode & file::DIRECT)
    {
#if !FOXXLL_DIRECT_IO_OFF
        dwFlagsAndAttributes |= FILE_FLAG_NO_BUFFERING;
        // TODO: try also FILE_FLAG_WRITE_THROUGH option ?
#else
        if (mode & file::REQUIRE_DIRECT) {
            LOG1 << "Error: open()ing " << filename << " with DIRECT mode required, but the system does not support it.";
            return INVALID_HANDLE_VALUE;
        }
        else {
            LOG1 << "Warning: open()ing " << filename << " without DIRECT mode, as the system does not support it.";
        }
#endif
    }

    if (mode & file::SYNC)
    {
        // ignored
    }

    HANDLE file_des_ = ::CreateFileA(
            filename.c_str(), dwDesiredAccess, dwShareMode, nullptr,
            dwCreationDisposition, dwFlagsAndAttributes, nullptr
        );

    if (file_des_ != INVALID_HANDLE_VALUE)
        return file_des_;

#if !FOXXLL_DIRECT_IO_OFF
    if ((mode& file::DIRECT) && !(mode & file::REQUIRE_DIRECT))
    {
        LOG1 << "CreateFile() error on path=" << filename << " mode=" << mode << ", retrying without DIRECT mode.";

        dwFlagsAndAttributes &= ~FILE_FLAG_NO_BUFFERING;

        HANDLE file_des2 = ::CreateFileA(
                filename.c_str(), dwDesiredAccess, dwShareMode, nullptr,
                dwCreationDisposition, dwFlagsAndAttributes, nullptr
            );

        if (file_des2 != INVALID_HANDLE_VALUE)
            return file_des2;
    }
#endif

    FOXXLL_THROW_WIN_LASTERROR(io_error, "CreateFile() path=" << filename << " mode=" << mode);
}

wfs_file_base::wfs_file_base(const std::string& filename, int mode)
    : file_des_(INVALID_HANDLE_VALUE),
      mode_(mode), filename_(filename), locked_(false)
{
    file_des_ = open_file_impl(filename, mode);
    need_alignment_ = (mode& file::DIRECT) != 0;

    if (!(mode & NO_LOCK))
    {
        lock();
    }

    if (!(mode_ & RDONLY) && (mode & DIRECT))
    {
        char buf[32768], * part;
        if (!GetFullPathNameA(filename.c_str(), sizeof(buf), buf, &part))
        {
            LOG1 << "wfs_file_base::wfs_file_base(): GetFullPathNameA() error for file " << filename;
            bytes_per_sector_ = 512;
        }
        else
        {
            part[0] = char();
            DWORD bytes_per_sector_;
            if (!GetDiskFreeSpaceA(buf, nullptr, &bytes_per_sector_, nullptr, nullptr))
            {
                LOG1 << "wfs_file_base::wfs_file_base(): GetDiskFreeSpaceA() error for path " << buf;
                bytes_per_sector_ = 512;
            }
            else
                bytes_per_sector_ = bytes_per_sector_;
        }
    }
}

wfs_file_base::~wfs_file_base()
{
    close();
}

void wfs_file_base::close()
{
    std::unique_lock<std::mutex> fd_lock(fd_mutex_);

    if (file_des_ == INVALID_HANDLE_VALUE)
        return;

    if (!CloseHandle(file_des_))
        FOXXLL_THROW_WIN_LASTERROR(io_error, "CloseHandle() of file fd=" << file_des_);

    file_des_ = INVALID_HANDLE_VALUE;
}

void wfs_file_base::lock()
{
    std::unique_lock<std::mutex> fd_lock(fd_mutex_);
    if (locked_)
        return;  // already locked
    if (LockFile(file_des_, 0, 0, 0xffffffff, 0xffffffff) == 0)
        FOXXLL_THROW_WIN_LASTERROR(io_error, "LockFile() fd=" << file_des_);
    locked_ = true;
}

file::offset_type wfs_file_base::_size()
{
    LARGE_INTEGER result;
    if (!GetFileSizeEx(file_des_, &result))
        FOXXLL_THROW_WIN_LASTERROR(io_error, "GetFileSizeEx() fd=" << file_des_);

    return result.QuadPart;
}

file::offset_type wfs_file_base::size()
{
    std::unique_lock<std::mutex> fd_lock(fd_mutex_);
    return _size();
}

void wfs_file_base::set_size(offset_type newsize)
{
    std::unique_lock<std::mutex> fd_lock(fd_mutex_);
    offset_type cur_size = _size();

    if (!(mode_ & RDONLY))
    {
        LARGE_INTEGER desired_pos;
        desired_pos.QuadPart = newsize;

        bool direct_with_bad_size = (mode_& file::DIRECT) && (newsize % bytes_per_sector_);
        if (direct_with_bad_size)
        {
            if (!CloseHandle(file_des_))
                FOXXLL_THROW_WIN_LASTERROR(io_error, "closing file (call of ::CloseHandle() from set_size) ");

            file_des_ = INVALID_HANDLE_VALUE;
            file_des_ = open_file_impl(filename_, WRONLY);
        }

        if (!SetFilePointerEx(file_des_, desired_pos, nullptr, FILE_BEGIN))
            FOXXLL_THROW_WIN_LASTERROR(
                io_error,
                "SetFilePointerEx() in wfs_file_base::set_size(..) oldsize=" << cur_size <<
                    " newsize=" << newsize << " "
            );

        if (!SetEndOfFile(file_des_))
            FOXXLL_THROW_WIN_LASTERROR(
                io_error, "SetEndOfFile() oldsize=" << cur_size <<
                    " newsize=" << newsize << " "
            );

        if (direct_with_bad_size)
        {
            if (!CloseHandle(file_des_))
                FOXXLL_THROW_WIN_LASTERROR(io_error, "closing file (call of ::CloseHandle() from set_size) ");

            file_des_ = INVALID_HANDLE_VALUE;
            file_des_ = open_file_impl(filename_, mode_ & ~TRUNC);
        }
    }
}

void wfs_file_base::close_remove()
{
    close();
    ::DeleteFileA(filename_.c_str());
}

} // namespace foxxll

#endif // FOXXLL_WINDOWS

/**************************************************************************/
