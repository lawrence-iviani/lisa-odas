#include "common.h"
#include "matrix_odas_receiver.h"


//extern int parse_ini_file(char * ini_name);

int main(int argc, char *argv[]) {
	int status ;

    if (argc<2) {
        status = parse_ini_file("example.ini");
    } else {
        status = parse_ini_file(argv[1]);
    }
	if (status!=0) {
		exit(-1);
	}
	return main_loop();
}


