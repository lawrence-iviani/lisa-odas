#include "common.h"
#include "matrix_odas_receiver.h"


int main(int argc, char *argv[]) {
	int status;
	bool use_matrix_led = false;
	bool dump_raw_pcm = false;
	int c;

    if (argc<2) {
        status = parse_ini_file("example.ini");
    } else {
        status = parse_ini_file(argv[1]);
		while ((c = getopt (argc, argv, "ldh")) != -1) {
			switch (c) {
				case 'l':
					use_matrix_led = true;
					break;
				case 'd':
					dump_raw_pcm = true;
					break;
				//case 'c':
				//	cvalue = optarg;
				//	break;
				case 'h':
				default:
					fprintf (stdout,"Usage: lisa-ODAS-receiver [filename.ini] [-ld]\n\t-d dump to PCM files \n\t-l use matrix overloop leds (deprecated)\n");
					exit(-1);
			}
		}
    }
	if (status!=0) {
		exit(-2);
	}
	return main_loop(use_matrix_led, use_matrix_led);
}


