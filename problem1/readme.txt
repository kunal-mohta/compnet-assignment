PROBLEM 1

Files included & their function:-

- client.c :- Code for client
- server.c :- Code for server
- readme.txt :- readme
- packet.h :- Packet structure & user defined macros
- common.h :- Utility macros and function prototypes
- utils.c :- Utility functions common to all hosts
- makefile :- For easy build/compilation
- client :- Executable for client
- server :- Executable for server

*******************************************************************************************

HOW TO EXECUTE

First, compile and run server:-

$ make server
$ ./server

Second, in different terminal, compile and run client:-

$ make client 
$ ./client

*NOTE* that server should be running before client.

*******************************************************************************************

*** CLIENT ***

CHANNELS

-> 2 TCP connections used for 2 channels. NON-BLOCKING read used to read/write on the channels simultaneously, instead of fork().
This is because both channels need to know the offset till which the file has been read and sent, 
which is difficult to communicate between the child processes
(without the use of Interprocess communication techniques).

----------------------------------------------------------------------------------------

TIMEOUTS & RETRANSMISSION

-> clock() is used for timeouts. Whenever a packet is sent, time is recorded via clock() and
the packet is stored in variable, which is later used for retransmission.
-> Since sockets have been made non-blocking (using fcntl()),
the complete file transfer procedure is done in a constantly running loop.
-> Whenever a read() gives EAGAIN or EWOULDBLOCK error (which occurs when a non blocking read has
no data to read on the socket), a check for time out for the previously sent packet is done,
by comparing the current clock() value with previously stored value.
[*NOTE* that separate variables for storing packet copy & time are used for the 2 different channels]


*** CLIENT END ***

*******************************************************************************************

*** SERVER ***

-> Server uses poll() to deal with simultaneously reading for data from both the channels of client.


PACKET DROPS

-> should_drop() function (in utility.c) created to decide whether to drop the received packet or not
It generates a random number between 0 and 1, and drops the packet if the random number is
less than (PDR/100). [PDR -> Packet Drop Rate]
-> Even if the packet is dropped, it is still printed on the console of the server as RCVD.

----------------------------------------------------------------------------------------

BUFFERING

-> Fixed size buffer used on server to facilitate in-order writing of packets to output file.
-> Protocol used for buffering is as follows:-
If a packet arrives with sequence number GREATER THAN the current expected sequence number,
it is inserted into the buffer.
If a packet arrives with sequence number EQUAL TO the current expected sequence number,
write its payload to file and loop through the buffer to find all packets with sequence number
that are in order, write their payload to file and remove them from the buffer.
If the buffer is full, ignore the packet.
For any other case, ignore the packet.

*NOTE* that even if a packet is ignored, it is still printed as RCVD on the console of server.


*** SERVER END ***

*******************************************************************************************

IMPORTANT CONSTANTS USED

Defined in packet.h :-
- PACKET_SIZE -> Number of bytes of payload in each packet
- PDR -> Packet drop rate (in percentage units)
- PKT_TIMEOUT -> Time for which client waits for ACK before retransmission
				(in seconds unit, but can use decimal for milliseconds)
- BUF_SIZE -> Number of packets that can be buffered on server side
			  (for in order writing to file)

Defined in common.h :-
- INPUT_FILE -> Name of input file which is to be transferred
- OUTPUT_FILE -> Name of output file which will be created
- SERVER_PORT -> Port number on which server will run
- SERV_ADDR -> IP address for server 

*******************************************************************************************

ASSUMPTIONS

-> Single server, single client (but 2 channels) model is assumed, i.e. server will cater to a
single client and then stop.

-> Separate logs for events like packet drops, timeouts, retransmissions, etc. have NOT been printed.
Only the client and server trace (as described in the problem statement) has been printed on
the console. On retransmission, standard SENT event is printed. On packet drop,
RCVD event is printed, but ACK is not sent.

-> No input file has been included in the folder, by default.
Please add a file with name same as the value of INPUT_FILE (in common.h) to the folder before running.

*******************************************************************************************
