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

#ifndef __ZMQ_XREQ_HPP_INCLUDED__
#define __ZMQ_XREQ_HPP_INCLUDED__

#include "socket_base.hpp"
#include "fq.hpp"
#include "lb.hpp"

namespace zmq
{

    class xreq_t : public socket_base_t
    {
    public:

        xreq_t (class app_thread_t *parent_);
        ~xreq_t ();

        //  Overloads of functions from socket_base_t.
        void xattach_pipes (class reader_t *inpipe_, class writer_t *outpipe_);
        void xdetach_inpipe (class reader_t *pipe_);
        void xdetach_outpipe (class writer_t *pipe_);
        void xkill (class reader_t *pipe_);
        void xrevive (class reader_t *pipe_);
        int xsetsockopt (int option_, const void *optval_, size_t optvallen_);
        int xsend (zmq_msg_t *msg_, int flags_);
        int xflush ();
        int xrecv (zmq_msg_t *msg_, int flags_);
        bool xhas_in ();
        bool xhas_out ();

    private:

        //  Messages are fair-queued from inbound pipes. And load-balanced to
        //  the outbound pipes.
        fq_t fq;
        lb_t lb;

        xreq_t (const xreq_t&);
        void operator = (const xreq_t&);
    };

}

#endif
