
#include "led_bus.h"

/* --------------------------------------------- */
/* ---------- INTERNAL DATA STRUCTURE ---------- */
/* --------------------------------------------- */
static led_energies_struct led_energies;

// POTS (LED) SECTION
void update_pot(SSL_src_struct * SSL_data_src, SST_src_struct * SST_data_src)  {
  // https://en.wikipedia.org/wiki/Spherical_coordinate_system#Coordinate_system_conversions
  // Convert x,y to angle. TODO: See why x axis from ODAS is inverted... ????
  double x, y, z, E; 
  x = SSL_data_src->x;
  y = SSL_data_src->y;
  z = SSL_data_src->z;
  E = SSL_data_src->E;
  
  double t_x, t_y, t_z, t_act; 
  t_x = SST_data_src->x;
  t_y = SST_data_src->y;
  t_z = SST_data_src->z;
  t_act = SST_data_src->activity;
  
  // only for debug purpose
  char* tag = SST_data_src->tag;
 
  double angle_fi = fmodf((atan2(y, x) * (180.0 / M_PI)) + 360, 360);
  double angle_theta = 90.0 - fmodf((atan2(sqrt(y*y+x*x), z) * (180.0 / M_PI)) + 180, 180);
  double angle_fi_t = fmodf((atan2(t_y, t_x) * (180.0 / M_PI)) + 360, 360);
  
  // Convert angle to index
  int i_angle_fi = angle_fi / 360 * ENERGY_COUNT;  // convert degrees to index
  int i_angle_proj_theta = angle_theta / 180 * ENERGY_COUNT;  // convert degrees to index
  int i_angle_fi_t = angle_fi_t / 360 * ENERGY_COUNT;  // convert degrees to index
  
  // Set energies for  azimuth fi and theta
  led_energies.energy_array_azimuth[i_angle_fi] += INCREMENT * E * cos(angle_theta * M_PI / 180.0 ); // sin split the increment the projection of E on XY plane (the plane of the circular array)
  led_energies.energy_array_elevation[i_angle_proj_theta] += INCREMENT * E * sin(angle_theta * M_PI / 180.0); // cos split the increment the projection of fi  on XZ plane (looking at the top of the array)
  led_energies.detect[i_angle_fi_t] += INCREMENT * t_act;

  // limit at MAX_VALUE
  led_energies.energy_array_azimuth[i_angle_fi] =
      led_energies.energy_array_azimuth[i_angle_fi] > MAX_VALUE ? MAX_VALUE : led_energies.energy_array_azimuth[i_angle_fi];
  led_energies.energy_array_elevation[i_angle_proj_theta] =
      led_energies.energy_array_elevation[i_angle_proj_theta] > MAX_VALUE ? MAX_VALUE : led_energies.energy_array_elevation[i_angle_proj_theta];
  led_energies.detect[i_angle_fi_t] =
      led_energies.detect[i_angle_fi_t] > MAX_VALUE ? MAX_VALUE : led_energies.detect[i_angle_fi_t];

  // Debug section
  debug_print(DEBUG_DOA, "Object SSL_DATA.src(0x%X)\t(x=%f\ty=%f\tz=%f\tE=%f)\n", SSL_data_src,x,y,z,E);
  debug_print(DEBUG_DOA, "SSL angle_fi=%f energy_array_azimuth=%d --- i_angle_proj_theta=%f energy_array_elevation=%d\n", angle_fi, led_energies.energy_array_azimuth[i_angle_fi], angle_theta, led_energies.energy_array_elevation[i_angle_proj_theta] );
  debug_print(DEBUG_DOA, "Object SST_DATA.src(0x%X)\t%s\t(x=%f\ty=%f\tz=%f\tactivity=%f)\n", SST_data_src, tag, t_x,t_y,t_z,t_act);
  debug_print(DEBUG_DOA, "SST angle_fi=%f detect=%d\n", angle_fi_t, led_energies.detect[i_angle_fi_t]);
    
  if (PRINT_DETECTION and strlen(tag)>0) {
	  printf("SST ODAS_Channel\t%s\tactivity=%f\t(x=%f\ty=%f\tz=%f", tag,t_act, t_x,t_y,t_z);
	  printf("\tangle_fi=%f\tdetect=%d\n", angle_fi_t, led_energies.detect[i_angle_fi_t]);
  }
  if (PRINT_DETECTION and E>PRINT_MIN_DETECTION_SSL_E) {
	  printf("SSL ODAS_Channel\tE=%f\t(x=%f\ty=%f\tz=%f\tangle_fi=%f)", E, x,y,z,angle_fi_t);
	  printf("\tangle_fi=%f\tenergy_azimuth=%d\tangle_proj_theta=%f\tenergy_elevation=%d\n", angle_fi, led_energies.energy_array_azimuth[i_angle_fi], angle_theta, led_energies.energy_array_elevation[i_angle_proj_theta] );
  }
  
}

void decrease_pots() {
  for (int i = 0; i < ENERGY_COUNT; i++) {
    led_energies.energy_array_azimuth[i] -= (led_energies.energy_array_azimuth[i] > 0) ? DECREMENT : 0;
	led_energies.energy_array_elevation[i] -= (led_energies.energy_array_elevation[i] > 0) ? DECREMENT : 0;
	led_energies.detect[i] -= (led_energies.detect[i] > 0) ? DECREMENT : 0;
  }
}

void set_all_pots(hal_leds_struct *hw_led, SSL_struct * SSL_data, SST_struct * SST_data) {
	decrease_pots();
	for (int c = 0 ; c < MAX_ODAS_SOURCES; c++) {		
			debug_print(DEBUG_DOA, "Calculating pot energy for channel_%d SSL_DATA(0x%X).src(0x%X) - SST_DATA(0x%X).src(0x%X)\n", c, SSL_data, &SST_data->src[c], SST_data, &SST_data->src[c]);
			update_pot(&SSL_data->src[c], &SST_data->src[c]);
	}
	
	debug_print(DEBUG_DOA, "Update led bus (#%d) for Matrix %s HW_Data(0x%X)\n", hw_led->bus.MatrixLeds(), hw_led->bus.MatrixName() == hal::kMatrixCreator ? "Creator" : "Matrix",hw_led);
    for (int i = 0; i < hw_led->bus.MatrixLeds(); i++) {
      // led index to angle
      int led_angle = hw_led->bus.MatrixName() == hal::kMatrixCreator
                          ? leds_angle_mcreator[i]
                          : led_angles_mvoice[i];
      // Convert from angle to pots index
      int index_pots = led_angle * ENERGY_COUNT / 360;
      // Mapping from pots values to color
      int color_azimuth = led_energies.energy_array_azimuth[index_pots] * MAX_BRIGHTNESS / MAX_VALUE;
	  int color_elevation = led_energies.energy_array_elevation[index_pots] * MAX_BRIGHTNESS / MAX_VALUE;
	  int color_tracking = led_energies.detect[index_pots] * MAX_BRIGHTNESS / MAX_VALUE;
	  
      // Removing colors below the threshold
      color_azimuth = (color_azimuth < MIN_THRESHOLD) ? 0 : color_azimuth;
	  color_elevation = (color_elevation < MIN_THRESHOLD) ? 0 : color_elevation;
	  color_tracking = (color_tracking < MIN_THRESHOLD) ? 0 : color_tracking;
	  debug_print(DEBUG_DOA,"led_angle=%d, index_pots=%d, color_azimuth=%d, color_elevation=%d color_tracking=%d\n", led_angle,index_pots,color_azimuth, color_elevation, color_tracking ); 
	
      hw_led->image1d.leds[i].red = color_tracking;
      hw_led->image1d.leds[i].green = color_elevation;
      hw_led->image1d.leds[i].blue = color_azimuth;
      hw_led->image1d.leds[i].white = 0;
    }
    hw_led->everloop.Write(&hw_led->image1d);
}
