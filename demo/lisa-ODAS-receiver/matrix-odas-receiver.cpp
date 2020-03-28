#include <json.h>
#include <math.h>
#include <matrix_hal/everloop.h>
#include <matrix_hal/everloop_image.h>
#include <matrix_hal/matrixio_bus.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>
#include <array>
#include <iostream>

namespace hal = matrix_hal;

// ENERGY_COUNT : Number of sound energy slots to maintain.
#define ENERGY_COUNT 36
// MAX_VALUE : controls smoothness
#define MAX_VALUE 200
// INCREMENT : controls sensitivity
#define INCREMENT 20
// DECREMENT : controls delay in the dimming
#define DECREMENT 1
// MAX_BRIGHTNESS: Filters out low energy
#define MIN_THRESHOLD 10
// MAX_BRIGHTNESS: 0 - 255
#define MAX_BRIGHTNESS 50

#define DEBUG_DOA 1

// TODO: add received data in a structure per message type
double x, y, z, E;
int energy_array_azimuth[ENERGY_COUNT]; // fi
int energy_array_elevation[ENERGY_COUNT]; //theta


const double leds_angle_mcreator[35] = {
    170, 159, 149, 139, 129, 118, 108, 98,  87,  77,  67,  57,
    46,  36,  26,  15,  5,   355, 345, 334, 324, 314, 303, 293,
    283, 273, 262, 252, 242, 231, 221, 211, 201, 190, 180};

const double led_angles_mvoice[18] = {170, 150, 130, 110, 90,  70,
                                      50,  30,  10,  350, 330, 310,
                                      290, 270, 250, 230, 210, 190};

void increase_pots() {
  // https://en.wikipedia.org/wiki/Spherical_coordinate_system#Coordinate_system_conversions
  // Convert x,y to angle. TODO: See why x axis from ODAS is inverted... ????
  double angle_fi = fmodf((atan2(y, x) * (180.0 / M_PI)) + 360, 360);
  double angle_theta = 90.0 - fmodf((atan2(sqrt(y*y+x*x), z) * (180.0 / M_PI)) + 180, 180);
  // Convert angle to index
  int i_angle_fi = angle_fi / 360 * ENERGY_COUNT;  // convert degrees to index
  int i_angle_proj_theta = angle_theta / 180 * ENERGY_COUNT;  // convert degrees to index
  
  
  // Set energy for this angle, azimuth fi
  energy_array_azimuth[i_angle_fi] += INCREMENT * E * cos(angle_theta * M_PI / 180.0 );
  energy_array_elevation[i_angle_fi] += INCREMENT * E * sin(angle_theta * M_PI / 180.0);
  
  // Set energy for this angle, angle_theta theta
  printf("angle_fi=%f energy_array_azimuth=%d--- i_angle_proj_theta=%f --- energy_array_elevation=%d\n", angle_fi, energy_array_azimuth[i_angle_fi], angle_theta, energy_array_elevation[i_angle_proj_theta] );
  
  // Set limit at MAX_VALUE
  energy_array_azimuth[i_angle_fi] =
      energy_array_azimuth[i_angle_fi] > MAX_VALUE ? MAX_VALUE : energy_array_azimuth[i_angle_fi];
  energy_array_elevation[i_angle_proj_theta] =
      energy_array_elevation[i_angle_proj_theta] > MAX_VALUE ? MAX_VALUE : energy_array_elevation[i_angle_proj_theta];
}

void decrease_pots() {
  for (int i = 0; i < ENERGY_COUNT; i++) {
    energy_array_azimuth[i] -= (energy_array_azimuth[i] > 0) ? DECREMENT : 0;
	energy_array_elevation[i] -= (energy_array_elevation[i] > 0) ? DECREMENT : 0;
  }
}

void json_parse_array(json_object *jobj, char *key) {
  // Forward Declaration
  void json_parse(json_object * jobj);
  enum json_type type;
  json_object *jarray = jobj;
  if (key) {
    if (json_object_object_get_ex(jobj, key, &jarray) == false) {
      printf("Error parsing json object\n");
      return;
    }
  }

  int arraylen = json_object_array_length(jarray);
  int i;
  json_object *jvalue;

  for (i = 0; i < arraylen; i++) {
    jvalue = json_object_array_get_idx(jarray, i);
    type = json_object_get_type(jvalue);

    if (type == json_type_array) {
      json_parse_array(jvalue, NULL);
    } else if (type != json_type_object) {
    } else {
      json_parse(jvalue);
    }
  }
}

void json_parse(json_object *jobj) {
  enum json_type type;
  unsigned int count = 0;
  decrease_pots();
  json_object_object_foreach(jobj, key, val) {
    type = json_object_get_type(val);
    switch (type) {
      case json_type_boolean:
        break;
      case json_type_double:
        if (!strcmp(key, "x")) {
          x = json_object_get_double(val);
        } else if (!strcmp(key, "y")) {
          y = json_object_get_double(val);
        } else if (!strcmp(key, "z")) {
          z = json_object_get_double(val);
        } else if (!strcmp(key, "E")) {
          E = json_object_get_double(val);
        }
        increase_pots();
        count++;
        break;
      case json_type_int:
        break;
      case json_type_string:
        break;
      case json_type_object:
        if (json_object_object_get_ex(jobj, key, &jobj) == false) {
          printf("Error parsing json object\n");
          return;
        }
        json_parse(jobj);
        break;
      case json_type_array:
        json_parse_array(jobj, key);
        break;
    }
  }
}

int main(int argc, char *argv[]) {
  // Everloop Initialization
  hal::MatrixIOBus bus;
  if (!bus.Init()) return false;
  hal::EverloopImage image1d(bus.MatrixLeds());
  hal::Everloop everloop;
  everloop.Setup(&bus);

  // Clear all LEDs
  for (hal::LedValue &led : image1d.leds) {
    led.red = 0;
    led.green = 0;
    led.blue = 0;
    led.white = 0;
  }
  everloop.Write(&image1d);

  char verbose = 0x00;

  int server_id;
  struct sockaddr_in server_address;
  int connection_id;
  char *message;
  int messageSize;

  int c;
  unsigned int portNumber = 9001;
  const unsigned int nBytes = 10240;

  server_id = socket(AF_INET, SOCK_STREAM, 0);

  server_address.sin_family = AF_INET;
  server_address.sin_addr.s_addr = htonl(INADDR_ANY);
  server_address.sin_port = htons(portNumber);

  printf("Binding socket........... ");
  fflush(stdout);
  bind(server_id, (struct sockaddr *)&server_address, sizeof(server_address));
  printf("[OK]\n");

  printf("Listening socket......... ");
  fflush(stdout);
  listen(server_id, 1);
  printf("[OK]\n");

  printf("Waiting for connection in port %d ... ", portNumber);
  fflush(stdout);
  connection_id = accept(server_id, (struct sockaddr *)NULL, NULL);
  printf("[OK]\n");

  message = (char *)malloc(sizeof(char) * nBytes);
  printf("Receiving data........... \n\n");

  while ((messageSize = recv(connection_id, message, nBytes, 0)) > 0) {
    message[messageSize] = 0x00;

    printf("message: %s\n", message);
    json_object *jobj = json_tokener_parse(message);
    json_parse(jobj);

    for (int i = 0; i < bus.MatrixLeds(); i++) {
      // led index to angle
      int led_angle = bus.MatrixName() == hal::kMatrixCreator
                          ? leds_angle_mcreator[i]
                          : led_angles_mvoice[i];
      // Convert from angle to pots index
      int index_pots = led_angle * ENERGY_COUNT / 360;
      // Mapping from pots values to color
      int color_azimuth = energy_array_azimuth[index_pots] * MAX_BRIGHTNESS / MAX_VALUE;
	  int color_elevation = energy_array_elevation[index_pots] * MAX_BRIGHTNESS / MAX_VALUE;
	  
      // Removing colors below the threshold
      color_azimuth = (color_azimuth < MIN_THRESHOLD) ? 0 : color_azimuth;
	  color_elevation = (color_elevation < MIN_THRESHOLD) ? 0 : color_elevation;
	  
	  printf("led_angle=%d, index_pots=%d, color_azimuth=%d, color_elevation=%d\n", led_angle,index_pots,color_azimuth, color_elevation );
      image1d.leds[i].red = 0;
      image1d.leds[i].green = color_elevation;
      image1d.leds[i].blue = color_azimuth;
      image1d.leds[i].white = 0;
    }
    everloop.Write(&image1d);
	printf("\n\n");
  }
}