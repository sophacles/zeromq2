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

#include "../bindings/c/zmq.h"

#include "sub.hpp"
#include "err.hpp"

zmq::sub_t::sub_t (class app_thread_t *parent_) :
    socket_base_t (parent_),
    all_count (0)
{
    options.requires_in = true;
    options.requires_out = false;
}

zmq::sub_t::~sub_t ()
{
}

void zmq::sub_t::xattach_pipes (class reader_t *inpipe_,
    class writer_t *outpipe_)
{
    zmq_assert (inpipe_ && !outpipe_);
    fq.attach (inpipe_);
}

void zmq::sub_t::xdetach_inpipe (class reader_t *pipe_)
{
    zmq_assert (pipe_);
    fq.detach (pipe_);
}

void zmq::sub_t::xdetach_outpipe (class writer_t *pipe_)
{
    //  SUB socket is read-only thus there should be no outpipes.
    zmq_assert (false);
}

void zmq::sub_t::xkill (class reader_t *pipe_)
{
    fq.kill (pipe_);
}

void zmq::sub_t::xrevive (class reader_t *pipe_)
{
    fq.revive (pipe_);
}

int zmq::sub_t::xsetsockopt (int option_, const void *optval_,
    size_t optvallen_)
{
    if (option_ == ZMQ_SUBSCRIBE) {
        std::string subscription ((const char*) optval_, optvallen_);
        if (subscription == "*")
            all_count++;
        else if (subscription [subscription.size () - 1] == '*')
            prefixes.insert (subscription.substr (0, subscription.size () - 1));
        else
            topics.insert (subscription);
        return 0;
    }
    
    if (option_ == ZMQ_UNSUBSCRIBE) {
        std::string subscription ((const char*) optval_, optvallen_);
        if (subscription == "*") {
            if (!all_count) {
                errno = EINVAL;
                return -1;
            }
            all_count--;
        }
        else if (subscription [subscription.size () - 1] == '*') {
            subscriptions_t::iterator it = prefixes.find (
                subscription.substr (0, subscription.size () - 1));
            if (it == prefixes.end ()) {
                errno = EINVAL;
                return -1;
            }
            prefixes.erase (it);
        }
        else {
            subscriptions_t::iterator it = topics.find (subscription);
            if (it == topics.end ()) {
                errno = EINVAL;
                return -1;
            }
            topics.erase (it);
        }
        return 0;
    }

    errno = EINVAL;
    return -1;
}

int zmq::sub_t::xsend (zmq_msg_t *msg_, int flags_)
{
    errno = ENOTSUP;
    return -1;
}

int zmq::sub_t::xflush ()
{
    errno = ENOTSUP;
    return -1;
}

int zmq::sub_t::xrecv (zmq_msg_t *msg_, int flags_)
{
    while (true) {

        //  Get a message using fair queueing algorithm.
        int rc = fq.recv (msg_, flags_);

        //  If there's no message available, return immediately.
        if (rc != 0 && errno == EAGAIN)
            return -1;

        //  If there is no subscription return -1/EAGAIN.
        if (!all_count && prefixes.empty () && topics.empty ()) {
            errno = EAGAIN;
            return -1; 
        }

        //  If there is at least one "*" subscription, the message matches.
        if (all_count)
            return 0;

        //  Check the message format.
        //  TODO: We should either ignore the message or drop the connection
        //  if the message doesn't conform with the expected format.
        unsigned char *data = (unsigned char*) zmq_msg_data (msg_);
        zmq_assert (*data <= zmq_msg_size (msg_) - 1);
        std::string topic ((const char*) (data + 1), *data);

        //  Check whether the message matches at least one prefix subscription.
        for (subscriptions_t::iterator it = prefixes.begin ();
              it != prefixes.end (); it++)
            if (it->size () <= topic.size () &&
                  *it == topic.substr (0, it->size ()))
                return 0;

        //  Check whether the message matches an exact match subscription.
        subscriptions_t::iterator it = topics.find (topic);
        if (it != topics.end ())
            return 0;
    }
}

bool zmq::sub_t::xhas_in ()
{
    //  TODO:  This is more complex as we have to ignore all the messages that
    //         don't fit the filter.
    zmq_assert (false);
    return false;
}

bool zmq::sub_t::xhas_out ()
{
    return false;
}
