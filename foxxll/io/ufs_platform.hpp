/***************************************************************************
 *  foxxll/io/ufs_platform.hpp
 *
 *  Platform porting code local the I/O file implementations. This header is
 *  not part of FOXXLL's public interface and must only be used inside the
 *  library.
 *
 *  Part of FOXXLL. See http://foxxll.org
 *
 *  Copyright (C) 2013 Timo Bingmann <tb@panthema.net>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef FOXXLL_IO_UFS_PLATFORM_HEADER
#define FOXXLL_IO_UFS_PLATFORM_HEADER

#include <foxxll/config.hpp>

#if FOXXLL_WINDOWS || defined(__MINGW32__)
  #ifndef NOMINMAX
    #define NOMINMAX
  #endif
  #include <windows.h>
// this is not foxxll/io/io.hpp !
  #include <io.h>
#else
  #include <unistd.h>
#endif

// these exist on Windows and Unixs
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

// required for ::remove()
#include <cstdio>

// for systems that don't know anything about block devices.
#ifndef S_ISBLK
  #define S_ISBLK(x) 0
#endif

// for systems with missing flags
#ifndef O_SYNC
  #define O_SYNC 0
#endif
#ifndef O_RSYNC
  #define O_RSYNC 0
#endif
#ifndef O_DSYNC
  #define O_DSYNC 0
#endif

#if defined(__linux__)
  #if !defined(O_DIRECT)
    #error O_DIRECT is not defined while __linux__ is - PLEASE REPORT THIS BUG
  #endif
// FIXME: In which conditions is this not defined? Why only i386 and alpha? Why not amd64?
  #if !defined(O_DIRECT) && (defined(__alpha__) || defined(__i386__))
    #define O_DIRECT 040000       /* direct disk access */
  #endif
#endif

#ifndef O_DIRECT
  #define O_DIRECT O_SYNC
#endif

// use 64-bit functions on Windows
#if FOXXLL_WINDOWS
  #ifndef lseek
    #define lseek _lseeki64
  #endif
  #ifndef off_t
    #define off_t int64_t
  #endif
#endif

#endif // !FOXXLL_IO_UFS_PLATFORM_HEADER

/**************************************************************************/
