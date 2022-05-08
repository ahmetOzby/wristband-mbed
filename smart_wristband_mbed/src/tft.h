#ifndef _TFT_H
#define _TFT_H
#include <SPI.h>
#include <TFT_eSPI.h>



#define TFT_GREY 0x5AEB
#define HH  12  // initial value of hours
#define MM  30  // initial value of minutes
#define SS  30  // initial value of seconds














void start_clock(void);
void clock_gui_init(void);
#endif