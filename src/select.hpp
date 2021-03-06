/*
    Copyright (c) 2007-2009 FastMQ Inc.

    This file is part of 0MQ.

    0MQ is free software; you can redistribute it and/or modify it under
    the terms of the Lesser GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    0MQ is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    Lesser GNU General Public License for more details.

    You should have received a copy of the Lesser GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __ZMQ_SELECT_HPP_INCLUDED__
#define __ZMQ_SELECT_HPP_INCLUDED__

#include "platform.hpp"

#include <stddef.h>
#include <vector>

#ifdef ZMQ_HAVE_WINDOWS
#include "winsock2.h"
#elif defined ZMQ_HAVE_OPENVMS
#include <sys/types.h>
#include <sys/time.h>
#else
#include <sys/select.h>
#endif

#include "fd.hpp"
#include "thread.hpp"
#include "atomic_counter.hpp"

namespace zmq
{

    //  Implements socket polling mechanism using POSIX.1-2001 select()
    //  function.

    class select_t
    {
    public:

        typedef fd_t handle_t;

        select_t ();
        ~select_t ();

        //  "poller" concept.
        handle_t add_fd (fd_t fd_, struct i_poll_events *events_);
        void rm_fd (handle_t handle_);
        void set_pollin (handle_t handle_);
        void reset_pollin (handle_t handle_);
        void set_pollout (handle_t handle_);
        void reset_pollout (handle_t handle_);
        void add_timer (struct i_poll_events *events_);
        void cancel_timer (struct i_poll_events *events_);
        int get_load ();
        void start ();
        void stop ();

    private:

        //  Main worker thread routine.
        static void worker_routine (void *arg_);

        //  Main event loop.
        void loop ();

        struct fd_entry_t
        {
            fd_t fd;
            struct i_poll_events *events;
        };

        //  Set of file descriptors that are used to retreive
        //  information for fd_set.
        typedef std::vector <fd_entry_t> fd_set_t;
        fd_set_t fds;

        fd_set source_set_in;
        fd_set source_set_out;
        fd_set source_set_err;

        fd_set readfds;
        fd_set writefds;
        fd_set exceptfds;

        //  Maximum file descriptor.
        fd_t maxfd;

        //  If true, at least one file descriptor has retired.
        bool retired;

        //  List of all the engines waiting for the timer event.
        typedef std::vector <struct i_poll_events*> timers_t;
        timers_t timers;

        //  If true, thread is shutting down.
        bool stopping;

        //  Handle of the physical thread doing the I/O work.
        thread_t worker;

        //  Load of the poller. Currently number of file descriptors
        //  registered with the poller.
        atomic_counter_t load;

        select_t (const select_t&);
        void operator = (const select_t&);
    };

}

#endif

