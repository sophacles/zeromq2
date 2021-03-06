#
#    Copyright (c) 2007-2009 FastMQ Inc.
#
#    This file is part of 0MQ.
#
#    0MQ is free software; you can redistribute it and/or modify it under
#    the terms of the Lesser GNU General Public License as published by
#    the Free Software Foundation; either version 3 of the License, or
#    (at your option) any later version.
#
#    0MQ is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    Lesser GNU General Public License for more details.
#
#    You should have received a copy of the Lesser GNU General Public License
#    along with this program.  If not, see <http://www.gnu.org/licenses/>.

require 'librbzmq'

if ARGV.length != 3
	puts "usage: local_lat <bind-to> <message-size> <roundtrip-count>"
	Process.exit
end
    
bind_to = ARGV[0]
message_size = ARGV[1].to_i
roundtrip_count = ARGV[2].to_i
			
ctx = Context.new(1, 1, 0)
s = Socket.new(ctx, REP);
s.bind(bind_to);

for i in 0...roundtrip_count do
    msg = s.recv(0)
    s.send(msg, 0)
end

sleep 1


