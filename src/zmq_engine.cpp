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

#include <new>

#include "zmq_engine.hpp"
#include "zmq_connecter.hpp"
#include "io_thread.hpp"
#include "i_inout.hpp"
#include "config.hpp"
#include "err.hpp"

zmq::zmq_engine_t::zmq_engine_t (io_thread_t *parent_, fd_t fd_,
      const options_t &options_, bool reconnect_, const char *address_) :
    io_object_t (parent_),
    inpos (NULL),
    insize (0),
    decoder (in_batch_size, NULL, 0),
    outpos (NULL),
    outsize (0),
    encoder (out_batch_size, false),
    inout (NULL),
    options (options_),
    reconnect (reconnect_)
{
    if (reconnect)
        address = address_;

    //  Initialise the underlying socket.
    int rc = tcp_socket.open (fd_, options.sndbuf, options.rcvbuf);
    zmq_assert (rc == 0);
}

zmq::zmq_engine_t::~zmq_engine_t ()
{
}

void zmq::zmq_engine_t::plug (i_inout *inout_)
{
    zmq_assert (!inout);

    encoder.set_inout (inout_);
    decoder.set_inout (inout_);

    handle = add_fd (tcp_socket.get_fd ());
    set_pollin (handle);
    set_pollout (handle);

    inout = inout_;

    //  Flush all the data that may have been already received downstream.
    in_event ();
}

void zmq::zmq_engine_t::unplug ()
{
    rm_fd (handle);
    encoder.set_inout (NULL);
    decoder.set_inout (NULL);
    inout = NULL;
}

void zmq::zmq_engine_t::in_event ()
{
    //  If there's no data to process in the buffer...
    if (!insize) {

        //  Retrieve the buffer and read as much data as possible.
        decoder.get_buffer (&inpos, &insize);
        insize = tcp_socket.read (inpos, insize);

        //  Check whether the peer has closed the connection.
        if (insize == (size_t) -1) {
            insize = 0;
            error ();
            return;
        }
    }

    //  Push the data to the decoder.
    size_t processed = decoder.process_buffer (inpos, insize);

    //  Adjust the buffer.
    inpos += processed;
    insize -= processed;

    //  Stop polling for input if we got stuck.
    if (processed < insize)
        reset_pollin (handle);

    //  Flush all messages the decoder may have produced.
    inout->flush ();
}

void zmq::zmq_engine_t::out_event ()
{
    //  If write buffer is empty, try to read new data from the encoder.
    if (!outsize) {

        outpos = NULL;
        encoder.get_data (&outpos, &outsize);
        
        //  If there is no data to send, stop polling for output.
        if (outsize == 0) {
            reset_pollout (handle);
            return;
        }
    }

    //  If there are any data to write in write buffer, write as much as
    //  possible to the socket.
    int nbytes = tcp_socket.write (outpos, outsize);

    //  Handle problems with the connection.
    if (nbytes == -1) {
        error ();
        return;
    }

    outpos += nbytes;
    outsize -= nbytes;
}

void zmq::zmq_engine_t::revive ()
{
    set_pollout (handle);

    //  Speculative write: The assumption is that at the moment new message
    //  was sent by the user the socket is probably available for writing.
    //  Thus we try to write the data to socket avoiding polling for POLLOUT.
    //  Consequently, the latency should be better in request/reply scenarios.
    out_event ();
}

void zmq::zmq_engine_t::error ()
{
    zmq_assert (inout);

    zmq_connecter_t *reconnecter = NULL;
    if (reconnect) {

        //  Create a connecter object to attempt reconnect.
        //  Ask it to wait for a while before reconnecting.
        reconnecter = new (std::nothrow) zmq_connecter_t (
            inout->get_io_thread (), inout->get_owner (),
            options, inout->get_session_name (), true);
        zmq_assert (reconnecter);
        reconnecter->set_address (address.c_str ());
    }

    inout->detach (reconnecter);
    unplug ();
    delete this;
}
