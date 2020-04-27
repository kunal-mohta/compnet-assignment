PROBLEM 2

Files included & their function:-

- client.c :- Code for client
- server.c :- Code for server
- relay.c :- Code for relay 
- readme.txt :- readme
- packet.h :- Packet structure & user defined macros
- common.h :- Utility macros and function prototypes
- utils.c :- Utility functions common to all hosts
- makefile :- For easy build/compilation
- client :- Executable for client
- server :- Executable for server
- relay :- Executable for relay

****************************************************************************************

HOW TO EXECUTE

First, compile and run server:-

$ make server
$ ./server


Then, in different terminal, compile and run relay 1 (for odd packets):-

$ make relay
$ ./relay odd


Then, in different terminal, compile and run relay 2 (for even packets):-

$ make relay
$ ./relay even 

*NOTE* command line arguments odd & even used for selecting which relay to run.
IP addresses and ports are defined as macros in common.h (given later).


Then, in different terminal, compile and run client:-

$ make client 
$ ./client


*NOTE* - Please follow the same order of execution as given above.
Even for relays, first run the odd one, because it is supposed to be relay 1
(according to problem statement).
Any other order of execution might lead to wrong node names in logs.

****************************************************************************************

*** CLIENT ***

DEALING WITH 2 RELAYS

-> Since UDP is connection-less, a synchronization message is sent by client to both the relays,
to inform them the address of client.
-> NON-BLOCKING read is used to read/write on the relays simultaneously, instead of fork().
This is because both relays need to know the offset till which the file has been read and sent, 
as well as the next sequence number to be used,
which is difficult to communicate between the child processes
(without the use of Interprocess communication techniques).
-> An empty string is sent to both the relays at the end, to inform them that client is stopping.

----------------------------------------------------------------------------------------

SELECTIVE REPEAT

-> 2 arrays (of size equal to window size) are used. One to keep a copy of packet sent from
the window. Other to keep track of the status of the packet in the other array, i.e. whether it
is not sent (i.e. invalid entry), sent but not acked, or acked.
-> Both the arrays work in a "circular" manner.
-> Extra care was required to maintain appropriate pointers on these arrays, especially
when considering limited number of sequence number.
-> Every time in the main loop, maximum possible number of packets are sent to fill up the window.
-> If ACK for a packet is received whose sequence number is NOT equal to the "window start",
then status for the packet in updated in the status array.
-> If ACK for a packet is received whose sequence number is equal to the "window start",
then window is moved forward as much as possible, i.e. till the next unacked packet found.

----------------------------------------------------------------------------------------

TIMEOUTS & RETRANSMISSION

-> clock() is used for timeouts. Whenever a packet is sent, time is recorded via clock() and
the packet is stored in an array of size equal to the window, which is later used for retransmission.
-> Since read is being done in non-blocking manner, the complete file transfer procedure is 
done in a constantly running loop. Everytime on entering the loop, the stored time values
are checked for timeout by comparing the current clock() value with previously stored value.


*** CLIENT END ***

****************************************************************************************

*** RELAY ***


DEALING WITH CLIENT & SERVER SIMULTANEOUSLY

-> Relay acts like "server" for Client and "client" for Server.
-> It uses poll() to simultaneously read packets from both client and server.
-> Same IP address but different ports are used for communication with client and server.
-> a "synchronization" message is sent to server to inform it about the relay's address
(because of connection-less UDP).
-> An empty string is sent to both client and server at the end, to inform them that the relay is stopping.

----------------------------------------------------------------------------------------

DELAYS

-> mdelay() function (in utility.c) created to generate a delay of specified milliseconds
(use decimal for higher precision).
-> We need random delay in a range of 0 to 2ms (editable using macros, described later).
rand_range() function (in utility.c) is created to generate random number in given range.

----------------------------------------------------------------------------------------

PACKET DROPS

-> should_drop() function (in utility.c) created to decide whether to drop the received packet or not
It generates a random number between 0 and 1, and drops the packet if the random number is
less than (PDR/100). [PDR -> Packet Drop Rate]
-> Even if the packet is dropped, it is still printed on the console of the server as RCVD.


*** RELAY END ***

****************************************************************************************

*** SERVER ***


DEALING WITH 2 RELAYS

-> "Synchronization" done with both relays to get their addresses.
-> Non-blocking read done in a continuosly running loop. When a packet is received, it can be from
any of the 2 relays. The initially recorded addresses are used here to find which relay it is from.
-> poll() not used here, because it would require separate sockets to be created for each relay.

----------------------------------------------------------------------------------------

SELECTIVE REPEAT & BUFFERING

-> 2 arrays (of size equal to window size) are used. One to keep a copy of payload of packet sent from
the window, i.e. acting as buffer. Other to keep track of the status of the payload's packet in the other array -
whether it is unack (invalid) or ack (valid).
-> Both the arrays work in a "circular" manner.
-> Special care was required to maintain appropriate pointers on these arrays, especially
when considering limited number of sequence number.
-> If the sequence number of received packet lies inside the "window start" and "window end" range (inclusive),
then the status of the packet is updated in the status array.
-> If the sequence number of received packet is equal to the "window start", then the payload for as many packets
as possible is written to the file, i.e. till the next unack/invalid packet status found, and the window is moved
forward correspondingly.


*** SERVER END ***

****************************************************************************************

IMPORTANT CONSTANTS USED

Defined in packet.h :-
- PACKET_SIZE -> Number of bytes of payload in each packet
- PDR -> Packet drop rate (in percentage units)
- PKT_TIMEOUT -> Time for which client waits for ACK before retransmission
				(in seconds unit, but can use decimal for milliseconds)
- WINDOW_SIZE -> Size of window used for the selective repeat protocol
- SEQ_NOS -> Total sequence numbers, i.e. seq. no. will be in range [0, SEQ_NOS-1] (inclusive)
- DELAY_MIN & DELAY_MAX -> Range from which a random number is selected and taken as the delay
						   to be added (by relay). Units are in milliseconds (use decimal for
							higher precision)

Defined in common.h :-
- INPUT_FILE -> Name of input file which is to be transferred
- OUTPUT_FILE -> Name of output file which will be created
- SERVER_PORT -> Port number on which server will run
- RELAY1_CLI_PORT -> Port number on which relay 1 will run as client
- RELAY1_SERV_PORT -> Port number on which relay 1 will run as server
- RELAY2_CLI_PORT -> Port number on which relay 2 will run as client
- RELAY2_SERV_PORT -> Port number on which relay 2 will run as server
- SERV_ADDR -> IP address for server
- RELAY1_ADDR -> IP address for relay 1 
- RELAY2_ADDR -> IP address for relay 2

****************************************************************************************

ASSUMPTIONS

-> Client knows the address of both relays, but the relays don't know the address of client.
Relays know the address of server, but server doesn't know the address of any relay.
(Synchronization messages used to inform hosts about unkown addresses)

-> Single server, single client, 2 relay  model is assumed, i.e. server will cater to a
single client and then stop.

-> Logs are to be printed on their respective terminals for client, relay1, relay2, server.

-> Since all 4 processes are on same machine, communication is fast. This could lead to a scenario
that both the sender and receiver have same timestamp for send and receive. Sorting of logs need
to be appropriately done in such cases, because it can lead to a confusion that receiver received
before sender sent, even though in reality sender only sent first but receiver received quickly.

-> No input file has been included in the folder, by default.
Please add a file with name same as the value of INPUT_FILE (in common.h) to the folder before running.

****************************************************************************************
