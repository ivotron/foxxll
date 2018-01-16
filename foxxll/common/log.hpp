/***************************************************************************
 *  foxxll/common/log.hpp
 *
 *  Part of FOXXLL. See http://foxxll.org
 *
 *  Copyright (C) 2004-2005 Roman Dementiev <dementiev@ira.uka.de>
 *  Copyright (C) 2009 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef FOXXLL_COMMON_LOG_HEADER
#define FOXXLL_COMMON_LOG_HEADER

#include <fstream>

#include <foxxll/singleton.hpp>

namespace foxxll {

class logger : public singleton<logger>
{
    friend class singleton<logger>;

    std::ofstream log_stream_;
    std::ofstream errlog_stream_;
    std::ofstream* waitlog_stream_;

    logger();
    ~logger();

public:
    inline std::ofstream & log_stream()
    {
        return log_stream_;
    }

    inline std::ofstream & errlog_stream()
    {
        return errlog_stream_;
    }

    inline std::ofstream * waitlog_stream()
    {
        return waitlog_stream_;
    }
};

} // namespace foxxll

#endif // !FOXXLL_COMMON_LOG_HEADER
