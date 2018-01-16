/***************************************************************************
 *  foxxll/common/addressable_queues.hpp
 *
 *  Part of FOXXLL. See http://foxxll.org
 *
 *  Copyright (C) 2010-2011 Raoul Steffen <R-Steffen@gmx.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef FOXXLL_COMMON_ADDRESSABLE_QUEUES_HEADER
#define FOXXLL_COMMON_ADDRESSABLE_QUEUES_HEADER

#include <cassert>
#include <functional>
#include <list>
#include <map>
#include <set>
#include <utility>

namespace foxxll {

//! An internal fifo queue that allows removing elements addressed with (a copy
//! of) themselves.
//! \tparam KeyType Type of contained elements.
template <typename KeyType>
class addressable_fifo_queue
{
    using container_type = std::list<KeyType>;
    using container_iterator = typename container_type::iterator;
    using meta_type = std::map<KeyType, container_iterator>;
    using meta_iterator = typename meta_type::iterator;

    container_type vals;
    meta_type meta;

public:
    //! Type of handle to an entry. For use with insert and remove.
    using handle = meta_iterator;

    //! Create an empty queue.
    addressable_fifo_queue() { }
    ~addressable_fifo_queue() { }

    //! Check if queue is empty.
    //! \return If queue is empty.
    bool empty() const
    { return vals.empty(); }

    //! Insert new element. If the element is already in, it is moved to the
    //! back.
    //! \param e Element to insert.
    //! \return pair<handle, bool> Iterator to element; if element was newly
    //! inserted.
    std::pair<handle, bool> insert(const KeyType& e)
    {
        container_iterator ei = vals.insert(vals.end(), e);
        std::pair<handle, bool> r = meta.insert(std::make_pair(e, ei));
        if (! r.second)
        {
            // element was already in
            vals.erase(r.first->second);
            r.first->second = ei;
        }
        return r;
    }

    //! Erase element from the queue.
    //! \param e Element to remove.
    //! \return If element was in.
    bool erase(const KeyType& e)
    {
        handle mi = meta.find(e);
        if (mi == meta.end())
            return false;
        vals.erase(mi->second);
        meta.erase(mi);
        return true;
    }

    //! Erase element from the queue.
    //! \param i Iterator to element to remove.
    void erase(handle i)
    {
        vals.erase(i->second);
        meta.erase(i);
    }

    //! Access top element in the queue.
    //! \return Const reference to top element.
    const KeyType & top() const
    { return vals.front(); }

    //! Remove top element from the queue.
    //! \return Top element.
    KeyType pop()
    {
        assert(! empty());
        const KeyType e = top();
        meta.erase(e);
        vals.pop_front();
        return e;
    }
};

//! An internal priority queue that allows removing elements addressed with (a
//! copy of) themselves.
//! \tparam KeyType Type of contained elements.
//! \tparam PriorityType Type of Priority.
template <typename KeyType, typename PriorityType, class Cmp = std::less<PriorityType> >
class addressable_priority_queue
{
    struct cmp // like < for pair, but uses Cmp for < on first
    {
        bool operator () (const std::pair<PriorityType, KeyType>& left,
                          const std::pair<PriorityType, KeyType>& right) const
        {
            Cmp c;
            return c(left.first, right.first) ||
                   ((! c(right.first, left.first)) && left.second < right.second);
        }
    };

    using container_type = std::set<std::pair<PriorityType, KeyType>, cmp>;
    using container_iterator = typename container_type::iterator;
    using meta_type = std::map<KeyType, container_iterator>;
    using meta_iterator = typename meta_type::iterator;

    container_type vals;
    meta_type meta;

public:
    //! Type of handle to an entry. For use with insert and remove.
    using handle = meta_iterator;

    //! Create an empty queue.
    addressable_priority_queue() { }
    ~addressable_priority_queue() { }

    //! Check if queue is empty.
    //! \return If queue is empty.
    bool empty() const
    { return vals.empty(); }

    //! Insert new element. If the element is already in, it's priority is updated.
    //! \param e Element to insert.
    //! \param o Priority of element.
    //! \return pair<handle, bool> Iterator to element; if element was newly inserted.
    std::pair<handle, bool> insert(const KeyType& e, const PriorityType o)
    {
        std::pair<container_iterator, bool> s = vals.insert(std::make_pair(o, e));
        std::pair<handle, bool> r = meta.insert(std::make_pair(e, s.first));
        if (! r.second && s.second)
        {
            // was already in with different priority
            vals.erase(r.first->second);
            r.first->second = s.first;
        }
        return r;
    }

    //! Erase element from the queue.
    //! \param e Element to remove.
    //! \return If element was in.
    bool erase(const KeyType& e)
    {
        handle mi = meta.find(e);
        if (mi == meta.end())
            return false;
        vals.erase(mi->second);
        meta.erase(mi);
        return true;
    }

    //! Erase element from the queue.
    //! \param i Iterator to element to remove.
    void erase(handle i)
    {
        vals.erase(i->second);
        meta.erase(i);
    }

    //! Access top (= min) element in the queue.
    //! \return Const reference to top element.
    const KeyType & top() const
    { return vals.begin()->second; }

    //! Remove top (= min) element from the queue.
    //! \return Top element.
    KeyType pop()
    {
        assert(! empty());
        const KeyType e = top();
        meta.erase(e);
        vals.erase(vals.begin());
        return e;
    }
};

} // namespace foxxll

#endif // !FOXXLL_COMMON_ADDRESSABLE_QUEUES_HEADER
