#ifndef LED_BUS 
#define LED_BUS


#include <math.h>
#include <matrix_hal/everloop.h>
#include <matrix_hal/everloop_image.h>
#include <matrix_hal/matrixio_bus.h>
namespace hal = matrix_hal;

#include "common.h"

/* --------------------------------------- */
/* ---------- HW LED  STRUCTURE ---------- */
/* --------------------------------------- */
struct hal_leds_struct {
   hal::MatrixIOBus bus;
   hal::EverloopImage image1d;
   hal::Everloop everloop;
}; // hw layer for LEDs control

const double leds_angle_mcreator[35] = {
    170, 159, 149, 139, 129, 118, 108, 98,  87,  77,  67,  57,
    46,  36,  26,  15,  5,   355, 345, 334, 324, 314, 303, 293,
    283, 273, 262, 252, 242, 231, 221, 211, 201, 190, 180};

const double led_angles_mvoice[18] = {170, 150, 130, 110, 90,  70,
                                      50,  30,  10,  350, 330, 310,
                                      290, 270, 250, 230, 210, 190};

void update_pot(SSL_src_struct *SSL_data_src, SST_src_struct *SST_data_src);
void decrease_pots();
void set_all_pots(hal_leds_struct *hw_led, SSL_struct *SSL_data, SST_struct *SST_data);


#endif