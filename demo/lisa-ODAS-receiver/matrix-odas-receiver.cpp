
#include "common.h"
#include "led_bus.h"
#include "connection.h"
#include "json_decoder.h"

/* --------------------------------------------- */
/* ---------- INTERNAL DATA STRUCTURE ---------- */
/* --------------------------------------------- */
static SSL_struct SSL_data;
static SST_struct SST_data;
static hal_leds_struct hw_led;

/* ------------------------------- */
/* ---------- FUNCTIONS ---------- */
/* ------------------------------- */
char* message2str(int odas_id, char msg[]) {
	msg[0] = '\0';
	if (odas_id==SSL) {
		sprintf(msg + strlen(msg), "SSL Message\ntimestamp: %d\n", SSL_data.timestamp);
		double x,y,z,E;
		for (int c = 0; c < MAX_ODAS_SOURCES; c++) {
			x = SSL_data.src[c].x;
			y = SSL_data.src[c].y;
			z = SSL_data.src[c].z;
			E = SSL_data.src[c].E;
			sprintf(msg + strlen(msg), "\tsrc[%d]\tx=%f\ty=%f\tz=%f\tE=%f\n",c,x,y,z,E);
		}
	} else if (odas_id==SST) {
		sprintf(msg + strlen(msg), "SST Message\ntimestamp: %d\n", SST_data.timestamp);
		double x,y,z,activity;
		char* tag;
		unsigned id;
		for (int c = 0; c < MAX_ODAS_SOURCES; c++) {
			// NO IDEA, IF I USE DIRECTLY THE DATA IN THE STRUCTURE I GET ALWAYS 0!
			id = SST_data.src[c].id;
			x = SST_data.src[c].x;
			y = SST_data.src[c].y;
			z = SST_data.src[c].z;
			activity = SST_data.src[c].activity;
			tag = SST_data.src[c].tag;
			sprintf(msg + strlen(msg), "\tsrc[%d]\tid=%d\ttag=%s\tx=%f\ty=%f\tz=%f\tactivity=%f\n",
			                                  c,   id,    tag,    x,    y,    z,    activity);
	    }
	}
	return msg;
}


//DECODE ODAS MESSAGE
void decode_message(unsigned int msg_type, char * odas_json_msg) {
	// at start up there can be some message in queue, this can bring to the second message to be bad formatted
	// This creates a segfault
	debug_print(DEBUG_DECODE, "Decoding Message:\n%s\n",odas_json_msg);
	void* SSx_data;
	SSx_data = msg_type == SSL ? (void*)&SSL_data : SSx_data;
	SSx_data = msg_type == SST ? (void*)&SST_data : SSx_data;
	
	if (odas_json_msg[0]!='{') {
		fprintf(stderr, "decode_message: Ignoring message %s, wrong opening character  ->%c<-,  }", ODAS_data_source_str[msg_type], odas_json_msg[0]);
		return;
	}
	json_object *jobj = json_tokener_parse(odas_json_msg);
    json_parse(SSx_data, jobj, msg_type);
	
	
	// Only needed if debugging
	if (DEBUG_DECODE) {
		char msg[1024];
		debug_print(DEBUG_DECODE, "Decoded Message:\n%s\n",message2str(msg_type, msg));
	}
	
	json_object_put(jobj); 
}	

void decode_audio_stream_raw(FILE* outfile, char* odas_stream_msg, int message_len) {
	unsigned int n_bytes_sample_inmsg = (unsigned int) SSS_BITS/8; 
	unsigned int n_bytes_frame_inmsg = n_bytes_sample_inmsg*MAX_ODAS_SOURCES; 
	//unsigned int n_samples = message_len/n_bytes_frame_inmsg;
	//In this context a frame is collection of all the bytes, related to one sample for all channels
	// In other word 4 channels, INT_16 bit -> 8 bytes, 4 channels FLOAT 32 bit -> 16 bytes
	unsigned int n_frames_inmsg = message_len/n_bytes_frame_inmsg; // BETTER NAME?? frames or samples??
	
	if (message_len % n_frames_inmsg != 0) {
		printf("Received a non well formatted raw message... TODO!!!!!!!!!!!!!!!!!!!!!!");
		// TODO: should I limit the message len to skip the last frames. But this sounds not nessary, granted by the recv
	}
	debug_print(DEBUG_DUMP_FILES, "Writing %d bytes in %d frames of len=%d (word is %d) ---> fd=%d\n%", message_len, n_frames_inmsg, n_bytes_frame_inmsg, n_bytes_sample_inmsg, outfile);	
	fwrite(odas_stream_msg, sizeof(char), message_len, outfile);
}

int main_loop() {
  int c; // a counter for cycles for 
  
// Everloop Initialization
  if (!hw_led.bus.Init()) return false;
  hw_led.image1d = hal::EverloopImage(hw_led.bus.MatrixLeds());
  hw_led.everloop.Setup(&hw_led.bus);

// Clear all LEDs
  for (hal::LedValue &led : hw_led.image1d.leds) {
    led.red = 0;
    led.green = 0;
    led.blue = 0;
    led.white = 0;
  }
  hw_led.everloop.Write(&hw_led.image1d);

// INIT MESSAGES
  printf("(0x%X)SSL_data and (0x%X)SST_data", &SSL_data, &SST_data);
  printf("Init messages ");
  for (c = 0 ; c < NUM_OF_ODAS_DATA_SOURCES; c++) {	
    messages[c] = (char *)malloc(sizeof(char) * n_bytes_msg[c]);
	memset(messages[c], '\0', sizeof(char) * n_bytes_msg[c]);
	messages_size[c] = strlen(messages[c]);
	printf(" ...  %s(len=%d,nBytes=%d)", ODAS_data_source_str[c], messages_size[c], n_bytes_msg[c]);
  }
  printf(" [OK]\n");
  fflush(stdout);

// DUMP FILES
  if (DUMP_PCM) {  
	  printf(" Init output file(s)");
	  for (c = 0 ; c < NUM_OF_ODAS_DATA_SOURCES; c++) {	
			if (c==SSS_S or c == SSS_P) {
				dump_outfile_fd[c] = fopen (dump_outfile_name[c], "wb");
				printf(" ... for %s Open file %s fd = %d", ODAS_data_source_str[c],dump_outfile_name[c], dump_outfile_fd[c]);
				if (dump_outfile_fd[c]==NULL){
					printf("Fail opening file %s", dump_outfile_name[c]);
					exit(-1);
				}
			}
	  }
	  printf(" [OK]\n");
  }

// INIT CONNECTIONS
  printf(" Init listening");
  for (c = 0 ; c < NUM_OF_ODAS_DATA_SOURCES; c++) {
	  if (port_numbers[c]) {
		  printf(" ... %s ", ODAS_data_source_str[c]);
		  servers_id[c] = init_connection(servers_address[c], port_numbers[c], backlog); 
		  printf(" (%d)", servers_id[c]);
	  }
  }
  printf(" [OK]\n");
  fflush(stdout);
  
// ACCEPT CONNECTIONS
  printf(" Waiting For Connections\n ");
  bool services_connected = false;
  printf("Connecting: ");
  while (!services_connected) {
	for (c = 0 ; c < NUM_OF_ODAS_DATA_SOURCES; c++) {
	  if (port_numbers[c]) {
		printf("[%s", ODAS_data_source_str[c]);
		connections_id[c] = accept_connection(servers_id[c]); 
		printf("%s", connections_id[c] >= 0 ? " CONNECTED]\n" : ".]");
		fflush(stdout);
	  } 
    }
	services_connected = true;
	for (c = 0 ; c < NUM_OF_ODAS_DATA_SOURCES; c++) {	
	  services_connected = services_connected and (port_numbers[c] ? connections_id[c] > 0 : true);
    }
	printf("[services_connected %s\n", services_connected ? "True]" : "False]");
	//services_connected = (portNumber_ssl ? connection_id_ssl > 0 : true) and (portNumber_sst ? connection_id_sst > 0 : true); 
    usleep( (unsigned int)(SLEEP_ACCEPT_LOOP*1000000) ); 
  } 
  printf("Connection [OK]\n");
  fflush(stdout);

// RECEIVING DATA
  printf("Receiving data........... \n");
  int bytes_available;
  unsigned long n_cycles = 1; // Just a counter
  while (!reception_terminate(messages_size)) { 
	  // Separator to print only when debugging but not with the debug formatting	  
	  if (DEBUG_INCOME_MSG) { printf("---------------------------------\nSTART RECEPTION: %d\n---------------------------------\n", n_cycles);}
	  for (c = 0 ; c < NUM_OF_ODAS_DATA_SOURCES; c++) {	
		if (!port_numbers[c]) {
			// skip if 0, port not selected -> service not in use
			continue;
		}
	    memset(messages[c], '\0', sizeof(char) * n_bytes_msg[c]); // Reset before using, fill the message of NULLs (reset and possible previous values from previous iteraction)
		ioctl(connections_id[c] ,FIONREAD,&bytes_available);
		messages_size[c] = port_numbers[c] ? recv(connections_id[c] , messages[c], n_bytes_msg[c], 0): 0; // Received the message, if available
		//messages_size[c] = port_numbers[c] ? recv(connections_id[c] , messages[c], n_bytes_msg[c], MSG_DONTWAIT ): 0; // Received the message, if available
		debug_print(DEBUG_INCOME_MSG, "[Count %d] RECEIVED stream message %s: len=%d - bytes_available(before recv)=%d, ratio %f\n",n_cycles,  ODAS_data_source_str[c], messages_size[c], bytes_available, (float)bytes_available/n_bytes_msg[c]);
		if (messages_size[c]) {
			// accordingly to enum ODAS_data_source, 0 SSL, 1 SST, ...
			// Decode an incoming message and store in the proper C structure
			if (c == SST or c == SSL) {
				messages[c][messages_size[c]] = 0x00; 
				debug_print(DEBUG_INCOME_MSG, "RECEIVED JSOM message %s: len=%d\n", ODAS_data_source_str[c], messages_size[c]);
				decode_message( c, messages[c]);	
			} else if (c == SSS_S or c == SSS_P) {
				debug_print(DEBUG_INCOME_MSG, "RECEIVED PCM message %s: len=%d\n", ODAS_data_source_str[c], messages_size[c]);
				if (DUMP_PCM) {  
					decode_audio_stream_raw(dump_outfile_fd[c], messages[c], messages_size[c]);
				}
			} else {
				printf("Here with invalid c=%d",c );
			}
		} else {
			debug_print(DEBUG_INCOME_MSG, "returned 0 len for %s: len=%d\n", ODAS_data_source_str[c], messages_size[c]);
		}
		if (DEBUG_INCOME_MSG) { printf("END RECEPTION message %s: len=%d\n+-+-+-+-+-+-+-+-+-+-\n", ODAS_data_source_str[c], messages_size[c]); }
		fflush(stdout);
	  }
	  // Finally, set all the pots with all complete data
	  set_all_pots(&hw_led ,&SSL_data, &SST_data);
	  if (DEBUG_INCOME_MSG) { printf("---------------------------------\nEND RECEPTION: %d\n---------------------------------\n\n", n_cycles);}
	  n_cycles++;
  }
  printf("Receiving Data terminated [OK]\n");
  if (DUMP_PCM) {  
	  for (c = 0 ; c < NUM_OF_ODAS_DATA_SOURCES; c++) {	
		if (c==SSS_S or c == SSS_P) {
			fclose (dump_outfile_fd[c]); 
		}
	  }
	  printf("Closed Files [OK]\n");
  }
  return 1; 
}

int main(int argc, char *argv[]) {
	return main_loop();
}

