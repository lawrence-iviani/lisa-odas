
#include "connection.h"

/* --------------------------------------------- */
/* ---------- INTERNAL DATA STRUCTURE ---------- */
/* --------------------------------------------- */
static int counter_no_data = MAX_EMPTY_MESSAGE; //counter for empty messages, used for timeout

// CONNECTION SECTION
int init_connection(sockaddr_in &server_address, int port_number, int backlog) {
	/*Init a non blocking connection and return the socket ID*/
	int server_id = 0;
	server_id = socket(AF_INET, SOCK_STREAM, 0);
	server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port = htons(port_number);
	bind(server_id, (struct sockaddr *)&server_address, sizeof(server_address));  
	
	// https://www.cs.tau.ac.il/~eddiea/samples/Non-Blocking/tcp-nonblocking-server.c.html
    /* listen and change the socket into non-blocking state	*/
	listen(server_id, backlog); 
	fcntl(server_id, F_SETFL, O_NONBLOCK);
	
	return server_id;
}

int accept_connection(int server_id) {
	int connection_id = accept(server_id, (struct sockaddr *)NULL, NULL);
	if (connection_id==-1) {
		if (errno==EAGAIN) {
		  debug_print(DEBUG_CONNECTION, "server %d, no data (retry again) - %s\n",server_id, strerror(errno));
		} else {
		  fprintf(stderr,"accepting connection %d, error errno=%d - %s\n",server_id, errno, strerror(errno));
		  exit(-2);
		}
	} else {
		debug_print(DEBUG_CONNECTION, " [Connected] id=%d\n", connection_id);
	}
	fflush(stdout);
	return connection_id;
}

bool reception_terminate(unsigned int rcvd_bytes[]) {
	/*If any connetion terminated abnormally return true, not receiving data after a while is a sign of data closing*/
	for (int c = 0 ; c < NUM_OF_ODAS_DATA_SOURCES; c++) {	
		debug_print(DEBUG_CONNECTION, " For %s len msg received is %d\n", ODAS_data_source_str[c], rcvd_bytes[c]);
		if (rcvd_bytes[c] == -1) {
			debug_print(DEBUG_CONNECTION, "reception terminate  for income data %s\n" , ODAS_data_source_str[c]);
			return true;
		} else if ((rcvd_bytes[c] == 0) ) {
			counter_no_data--;
			if (counter_no_data==0) {
				debug_print(DEBUG_CONNECTION, "timeout for income data %s\n" , ODAS_data_source_str[c]);
				return true;
			} 
			debug_print(DEBUG_CONNECTION, "Decrease timeout counter %d/%d\n" , counter_no_data, MAX_EMPTY_MESSAGE);
		} else {
			counter_no_data = MAX_EMPTY_MESSAGE; // reset the counter
			debug_print(DEBUG_CONNECTION, "Reset counter, \n" , counter_no_data);
		}
	}
	fflush(stdout);
	return false;
}