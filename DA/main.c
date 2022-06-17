/***** Includes *****/
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "mxc_device.h"
#include "mxc_delay.h"
#include "mxc.h"
#include "bitmap.h"
#include "camera.h"
#include "tft_ssd2119.h"
#include "dma.h"

// personal
#include "camera_tft_funcs.h"


#define SCREEN_W 128 // image output width
#define SCREEN_H 128 // image output height
#define TFT_BUFF_SIZE   30    // TFT buffer size

char buff[TFT_BUFF_SIZE];

int main()
{
    /* Enable cache */
	MXC_ICC_Enable(MXC_ICC0);

	/* Switch to 100 MHz clock */
	MXC_SYS_Clock_Select(MXC_SYS_CLOCK_IPO);
	SystemCoreClockUpdate();

    // initialize LCD and camera
    set_image_dimensions(SCREEN_W,SCREEN_H);
    LCD_Camera_Setup();
    
    // int font_1 = urw_gothic_12_white_bg_grey;
    // MXC_TFT_SetBackGroundColor(4);
    // memset(buff, 32, TFT_BUFF_SIZE);
    // TFT_Print(buff, 55, 90, font_1, sprintf(buff, "Cats-vs-Dogs Demo"));

    while (1)
    {
        capture_process_camera();
        // display_RGB565_img(0,0,NULL,false);
    }
    
}
