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
#include "cnn_helper_funcs.h"


#define SCREEN_W 128 // image output width
#define SCREEN_H 128 // image output height
#define SCREEN_X 100 // image output top left corner
#define SCREEN_Y 50 // image output top left corner

int main()
{
    /* Enable cache */
	MXC_ICC_Enable(MXC_ICC0);

	/* Switch to 100 MHz clock */
	MXC_SYS_Clock_Select(MXC_SYS_CLOCK_IPO);
	SystemCoreClockUpdate();

    MXC_Delay(SEC(2)); // Let debugger interrupt if needed

    // initialize LCD and camera
    set_image_dimensions(SCREEN_W,SCREEN_H);
    LCD_Camera_Setup();
    
    printf("start up cnn\n");
    startup_cnn();
    cnn_output_t output;
    while (1)
    {
        //capture_process_camera(SCREEN_X,SCREEN_Y,NULL,false);
        // display_RGB565_img(0,0,NULL,false);

        // call a run cnn helper func here
        run_cnn(&output);
        show_cnn_output(&output);
    }
    
}
