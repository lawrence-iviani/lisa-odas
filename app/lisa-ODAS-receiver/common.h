#ifndef COMMON 
#define COMMON 


#include <netinet/in.h>		
#include <array>
#include <iostream>
#include <string.h>
#include <stdio.h>  // fopen & c


/* -------------------------------------------------------------- */
/* ---------- GENERAL CONFIGURATION, LEDs, CONNECTION  ---------- */
/* -------------------------------------------------------------- */
// ENERGY_COUNT : Number of sound energy slots to maintain.
#define ENERGY_COUNT 36
// MAX_VALUE : controls smoothness
#define MAX_VALUE 150
// INCREMENT : controls sensitivity
#define INCREMENT 20
// DECREMENT : controls delay in the dimming
#define DECREMENT 2
// MIN_THRESHOLD: Filters out low energy
#define MIN_THRESHOLD 5
// MAX_BRIGHTNESS: 0 - 255
#define MAX_BRIGHTNESS 220	
// SLEEP IN SEC, 0.1s -> 100 ms, wating time before checking if a socket connection is available (accept return > 0)
#define SLEEP_ACCEPT_LOOP 0.5
// How many empty messges should be received before raising a timeout
#define MAX_EMPTY_MESSAGE 200
//In a recv call, how many buffers of PCM data should i acquire. 
// Provide a balance between CPU usage (less buffers, higher CPU) and Latency (less buffers, lower latency) (TODO: find balance)
#define RECV_PCM_BUFFERS 4
// This must be the same parameters as in defined in configuration ssl.nPots, the fix number of messages, stream that are transmitted.
#define MAX_ODAS_SOURCES 4
// Use for dumping received RAW files to PCM
#define DUMP_PCM 0
// Number of ODAS_data_source (SST, SSL, SSS_S, SSS_P)
#define NUM_OF_ODAS_DATA_SOURCES 4 
// The max baclog message number in socket recv. WIth 1 I assume only one message at time is processed (TODO: not sure of this assumption)
#define MAX_RECV_BACKLOG 1

// Raw wave data stream
// as defined in SSS module in configuration sss.separated|postfiltered
#define SSS_SAMPLERATE 16000
#define SSS_HOPSIZE 128
#define SSS_BITS 16
#define SSS_GAIN 10.0  // Used only in postfiltered. TODO: Verify the effect

// Activate for debug different components
#define DEBUG_CONNECTION 0
#define DEBUG_DOA 0
#define DEBUG_JSON 0
#define DEBUG_INCOME_MSG 0
#define DEBUG_DECODE 0
#define DEBUG_DUMP_FILES 0
#define DEBUG_PYTHON_WRAPPER 0

// Debug options specific components
#define PRINT_DETECTION 0 // In relation to message debug only items that have a non empty tag for SST messages
#define PRINT_MIN_DETECTION_SSL_E 0.2

/* ------------------------------------------------------- */
/* ---------- CONNECTION CONSTANT AND STRUCTURE ---------- */
/* ------------------------------------------------------- */
// Lookup table to determine the index of the data source
// SSL: Sound Source localization
// SST: Sound Source tracking
// SSS_S: Sound Source stream(??) Separated (TODO: check SSS what does it mean)
// SSS_P: Sound Source stream(??) Postfiltered TODO!
enum ODAS_data_source{SSL = 0, SST = 1, SSS_S = 2, SSS_P = 3}; 
static char const  *ODAS_data_source_str[NUM_OF_ODAS_DATA_SOURCES] = {"SSL", "SST", "SSS_S", "SSS_P"}; 
// TO DEACTIVATE a specific income ODAS data, set to 0 the port in the relative position in port numbers
// exampless: SSL only {9001, 0} , SST only {9000}. 
// NOTE port numbers are defined in the relative ODAS .cfg file loaded at boot by ODAS server
const unsigned int port_numbers[NUM_OF_ODAS_DATA_SOURCES] = {9001, 9000, 10000, 10010};//10000}; 
const unsigned int n_bytes_raw_msg = (unsigned int) RECV_PCM_BUFFERS*SSS_HOPSIZE*MAX_ODAS_SOURCES*SSS_BITS/8;
const unsigned int n_bytes_json_msg = 10240; // untouched from the matrix example
const unsigned int n_bytes_msg[NUM_OF_ODAS_DATA_SOURCES] = {n_bytes_json_msg, n_bytes_json_msg, n_bytes_raw_msg, n_bytes_raw_msg }; // The max length of a message (assumning sizeof(char)=1), For SSS this is interpreted as the min block, if more data are available they are received as multiple of this number

static int servers_id[NUM_OF_ODAS_DATA_SOURCES] = {0, 0, 0, 0}; 
static struct sockaddr_in servers_address[NUM_OF_ODAS_DATA_SOURCES];
static int connections_id[NUM_OF_ODAS_DATA_SOURCES] = {0, 0, 0, 0}; 
static char *messages[NUM_OF_ODAS_DATA_SOURCES] = {NULL, NULL, NULL, NULL};
static unsigned int messages_size[NUM_OF_ODAS_DATA_SOURCES] = {0, 0, 0, 0}; 
static FILE *dump_outfile_fd[NUM_OF_ODAS_DATA_SOURCES] = {NULL, NULL, NULL, NULL};
static const char	*dump_outfile_name[NUM_OF_ODAS_DATA_SOURCES] = {"","", "separated.pcm", "postfiltered.pcm"}; //TODO: serialize also the json messages?

/* ----------------------------------------- */
/* ---------- ODAS DATA STRUCTURE ---------- */
/* ----------------------------------------- */
/* ---- example SSL ----
{
    "timeStamp": 41888,
    "src": [
        { "x": 0.000, "y": 0.824, "z": 0.566, "E": 0.321 },
        { "x": -0.161, "y": 0.959, "z": 0.232, "E": 0.121 },
        { "x": -0.942, "y": -0.263, "z": 0.211, "E": 0.130 },
        { "x": 0.266, "y": 0.507, "z": 0.820, "E": 0.081 }
    ]
}*/
struct SSL_src_struct {
	double x;
	double y;
	double z;
	double E;
}; // SSL src 
struct SSL_struct {
	unsigned int timestamp;
	SSL_src_struct src[MAX_ODAS_SOURCES]; // TODO, Max value or variable?
}; // SSL struct message

/* ---- example SST ----
{
    "timeStamp": 41887,
    "src": [
        { "id": 100, "tag": "dynamic", "x": -0.014, "y": 0.901, "z": 0.434, "activity": 0.954 },
        { "id": 112, "tag": "dynamic", "x": -0.966, "y": -0.161, "z": 0.204, "activity": 0.000 },
        { "id": 0, "tag": "", "x": 0.000, "y": 0.000, "z": 0.000, "activity": 0.000 },
        { "id": 0, "tag": "", "x": 0.000, "y": 0.000, "z": 0.000, "activity": 0.000 }
    ]
} */
#define SST_TAG_LEN 20
struct SST_src_struct {
	unsigned int id;
	char tag[SST_TAG_LEN]; // TODO, VERIFY THIS IS NOT BUGGY, NO IDEA WHAT IS THE MAX LEN!!!
	double x;
	double y;
	double z;
	double activity;
}; // SST src 

struct SST_struct {
	unsigned int timestamp;
	SST_src_struct src[MAX_ODAS_SOURCES]; // TODO, Max value or variable?
}; // SST struct message

struct led_energies_struct {
	int energy_array_azimuth[ENERGY_COUNT]; // fi
	int energy_array_elevation[ENERGY_COUNT]; //theta
	int detect[ENERGY_COUNT]; //detection level (if present)
};

/* ---------------------------------------------------------------- */
/* ---------- UTILITIES FOR DEBUG & OTHERS COMMONALITIES ---------- */
/* ---------------------------------------------------------------- */
// https://stackoverflow.com/questions/8487986/file-macro-shows-full-path
#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
// For Windows use '\\' instead of '/'.

// https://stackoverflow.com/questions/1644868/define-macro-for-debug-printing-in-c
#define debug_print(DEBUG, fmt, ...) \
        do { if (DEBUG) fprintf(stdout, "%s:%d:%s(): " fmt, __FILENAME__, \
                                __LINE__, __func__, __VA_ARGS__); fflush(stdout);} while (0)

#ifdef __cplusplus
#define EXTERN_C extern "C" {
#define EXTERN_C_END }
#else
#define EXTERN_C
#define EXTERN_C_END
#endif


#endif 