/***** Includes *****/
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "mxc_device.h"
#include "mxc_delay.h"
#include "mxc.h"

// personal
#include "camera_tft_funcs.h"
#include "color_print.h"


#define SCREEN_W 128 // image output width
#define SCREEN_H 128 // image output height
#define SCREEN_X 56 // image output top left corner
#define SCREEN_Y 140 // image output top left corner

// *****************************************************************************
int main()
{
    printf("\n\n" ANSI_COLOR_YELLOW "****************** DA ******************" ANSI_COLOR_RESET "\n\n");


    printf("*** SETUP ***\n\n");

    // Switch to 100 MHz clock
    MXC_SYS_Clock_Select(MXC_SYS_CLOCK_IPO);
    SystemCoreClockUpdate();

    
    // Set up the Camera and LCD
    LCD_Camera_Setup();

    while(1) 
    {
        capture_camera_img();
        display_RGB565_img(SCREEN_X,SCREEN_Y, NULL,false);
    }
}