In order to build and run the program write these 2 following lines in the shell (replace the values in parentheses):
make
./scheduler (RR/WRR/DRR) (quantum size) (input file) (output file)


the code includes implementation of 3 scheduling protocols:
1. Round Robin - a while loop that goes over all the connections until a packet is found and then returns it to be written to the output file.
   If a packet has been found the global variable current_connection_priority is incremented by 1 so that the next time this function is called the the connection with the next priority will be the first to searched.
2. Weighted Round Robin - a while loop the goes over all the connections until a packet is found.
   If the packet has a weight, then the connection current_weight and remainder_to_send_by_weight variables are updated according to it.
   Whether or not the connection weight has been updated, the remainder is decremented by 1.
   If after that the remainder is 0, meaning this connection has no more packets to send according to its weight the global variable current_connection_priority is incremented by 1 so that the next time this function is called the the       connection with the next priority will be the first to searched.
3. Deficit Roung Robin - a while loop the goes over all the connections until a packet is found.
   If the packet have been found in a new connection the flag current_connection_not_yet_credited is set to true, so the new connection will receive credit according to the quantum.
   If the connection doesn not have enough credit to send the current packet, the packet is inserted again at the head of the connection packets queue, current_connection_priority is incremented by 1 and the functions runs again.
   If the connection has enough credit to send the current packet, the packet's length is reduced from the connection's credit and the packet returns to be written to the output file.



the code includes implementation of 4 data structures:
1. connection_identifier - includes the sender's and receive's IP and port.
2. packet - includes the packet's:
	a. Time
	b. Length
	c. Weight
	d. The connection_identifier of the connection the packet belongs to
3. connection - includes the hash table index of the connection which is set by the hash function and the connection's packet queue. all the connections are set in an array according to their priority. the array is called connections_by_priority.
4. connection_hash_table_item - includes the priority value which is used to find the connection data structure and the identifier. all the connections are set in an array according to their hash table index. the array is called connections_hash_table.
