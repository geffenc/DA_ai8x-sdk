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
// #include "color_print.h"


#define SCREEN_W 128 // image output width
#define SCREEN_H 128 // image output height
#define SCREEN_X 56 // image output top left corner
#define SCREEN_Y 140 // image output top left corner
#define TFT_BUFF_SIZE   30    // TFT buffer size

char buff[TFT_BUFF_SIZE];
uint8_t data565[128 * 2];
static uint32_t input_0[128 * 128]; // buffer for camera image



void capture_process_camera(void) {

	uint8_t *raw;
	uint32_t imgLen;
	uint32_t w, h;

	int cnt = 0;

	uint8_t r, g, b;
	uint16_t rgb;
	int j = 0;

	uint8_t *data = NULL;
	stream_stat_t *stat;

	camera_start_capture_image();

	// Get the details of the image from the camera driver.
	camera_get_image(&raw, &imgLen, &w, &h);

	// Get image line by line
	for (int row = 0; row < h; row++) {
		// Wait until camera streaming buffer is full
		while ((data = get_camera_stream_buffer()) == NULL) {
			if (camera_is_image_rcv()) {
				break;
			}
		}

		//LED_Toggle(LED2);
#ifdef BOARD_EVKIT_V1
			j = 128*2 - 2; // mirror on display
#else
		j = 0;
#endif
		for (int k = 0; k < 2 * w; k += 2) {
			
            data565[j] = data[k];
            data565[j+1] = data[k+1];
#ifdef BOARD_EVKIT_V1
				j-=2; // mirror on display
#else
			j += 2;
#endif
		}
		MXC_TFT_ShowImageCameraRGB565(0, 0 + row, data565, w, 1);


		//LED_Toggle(LED2);
		// Release stream buffer
		release_camera_stream_buffer();
	}

	//camera_sleep(1);
	stat = get_camera_stream_statistic();

	if (stat->overflow_count > 0) {
		printf("OVERFLOW DISP = %d\n", stat->overflow_count);
		LED_On(LED2); // Turn on red LED if overflow detected
		while (1)
			;
	}

}
int image_bitmap_2 = logo_white_bg_darkgrey_bmp;
int main()
{
    int dma_channel;
    /* Enable cache */
	MXC_ICC_Enable(MXC_ICC0);

	/* Switch to 100 MHz clock */
	MXC_SYS_Clock_Select(MXC_SYS_CLOCK_IPO);
	SystemCoreClockUpdate();

    mxc_gpio_cfg_t tft_reset_pin = {MXC_GPIO0, MXC_GPIO_PIN_19, MXC_GPIO_FUNC_OUT, MXC_GPIO_PAD_NONE, MXC_GPIO_VSSEL_VDDIOH};
    MXC_TFT_Init(MXC_SPI0, 1, &tft_reset_pin, NULL);
    MXC_TFT_ClearScreen();
    MXC_TFT_ShowImage(0, 0, image_bitmap_2);
    MXC_Delay(1000000);

    // Initialize DMA for camera interface
	MXC_DMA_Init();
	dma_channel = MXC_DMA_AcquireChannel();
#define CAMERA_FREQ   (5 * 1000 * 1000)
	// Initialize camera.
	printf("Init Camera.\n");
	camera_init(CAMERA_FREQ);

	int ret = camera_setup(128, 128, PIXFORMAT_RGB565,
			FIFO_FOUR_BYTE, STREAMING_DMA, dma_channel);
	if (ret != STATUS_OK) {
		printf("Error returned from setting up camera. Error %d\n", ret);
		return -1;
	}
    camera_write_reg(0x11, 0x1); // set camera clock prescaller to prevent streaming overflow
    int font_1 = urw_gothic_12_white_bg_grey;
    MXC_TFT_SetBackGroundColor(4);
    memset(buff, 32, TFT_BUFF_SIZE);
    TFT_Print(buff, 55, 90, font_1, sprintf(buff, "Cats-vs-Dogs Demo"));
     

    while (1)
    {
        LED_Off(LED1);
		LED_Off(LED2);

        capture_process_camera();

        LED_On(LED1);
	    LED_On(LED2);
    }
    
}

// *****************************************************************************
// int main()
// {
//     // int font_1 = urw_gothic_12_white_bg_grey;
//     // printf("\n\n" ANSI_COLOR_YELLOW "****************** DA ******************" ANSI_COLOR_RESET "\n\n");


//     // printf("*** SETUP ***\n\n");

    
//     // MXC_ICC_Enable(MXC_ICC0); // Enable cache

//     // // Switch to 100 MHz clock
//     // MXC_SYS_Clock_Select(MXC_SYS_CLOCK_IPO);
//     // SystemCoreClockUpdate();

    
//     // // Set up the Camera and LCD
//     // LCD_Camera_Setup();

//     // memset(buff, 32, TFT_BUFF_SIZE);
//     // TFT_Print(buff, 55, 90, font_1, sprintf(buff, "Cats-vs-Dogs Demo"));
//     // printf("starting\n");
    
//     // while(1) 
//     // {
//     //     capture_camera_img();
//     //     display_RGB565_img(SCREEN_X,SCREEN_Y, NULL,false);
//     // }



// }