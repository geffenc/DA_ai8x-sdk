/*******************************************************************************
* Copyright (C) Maxim Integrated Products, Inc., All Rights Reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a
* copy of this software and associated documentation files (the "Software"),
* to deal in the Software without restriction, including without limitation
* the rights to use, copy, modify, merge, publish, distribute, sublicense,
* and/or sell copies of the Software, and to permit persons to whom the
* Software is furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included
* in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
* OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL MAXIM INTEGRATED BE LIABLE FOR ANY CLAIM, DAMAGES
* OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
* ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
* OTHER DEALINGS IN THE SOFTWARE.
*
* Except as contained in this notice, the name of Maxim Integrated
* Products, Inc. shall not be used except as stated in the Maxim Integrated
* Products, Inc. Branding Policy.
*
* The mere transfer of this software does not imply any licenses
* of trade secrets, proprietary technology, copyrights, patents,
* trademarks, maskwork rights, or any other form of intellectual
* property whatsoever. Maxim Integrated Products, Inc. retains all
* ownership rights.
*
******************************************************************************/

/*
  This file defines helper functions for interacting with the camera and
  LCD. Most of these come from the Maxim Integrated Drivers.
*/ 


// ========================================================================================= //
// ===================================== HEADERS =========================================== //
// ========================================================================================= //

#include "camera_tft_funcs.h"
#include "tft_ssd2119.h"
#include "camera.h"
#include "mxc.h"
#include "bitmap.h"
#include "color_print.h"


#define CAMERA_FREQ   (10 * 1000 * 1000)

// ========================================================================================= //
// ================================== GLOBAL VARIABLES ===================================== //
// ========================================================================================= //

// image dimensions
static int IMAGE_SIZE_X;
static int IMAGE_SIZE_Y;
int font_1 = urw_gothic_12_white_bg_grey;

// buffer for LCD (RGB565)
uint8_t data565[128 * 2];



// ========================================================================================= //
// ================================ FUNCTION DEFINITIONS =================================== //
// ========================================================================================= //

void set_image_dimensions(uint16_t x_dim, uint16_t y_dim)
{
    IMAGE_SIZE_X = x_dim;
    IMAGE_SIZE_Y = y_dim;
}


int get_image_x()
{
    return  IMAGE_SIZE_X;
}


int get_image_y()
{
    return  IMAGE_SIZE_Y;
}


void capture_camera_img(void) 
{
  camera_start_capture_image();
  while (1) 
  {
    if (camera_is_image_rcv()) 
    {
      return;
    }
  }
}


void display_RGB565_img(int x_coord, int y_coord,uint32_t* cnn_buffer, int load_cnn)
{
    uint8_t   *raw;
    uint32_t  imgLen;
    uint32_t  w, h;

    uint8_t ur,ug,ub;
    int8_t r,g,b;

    uint8_t px1;
    uint8_t px2;

    // Get the details of the image from the camera driver.
    capture_camera_img();
	  camera_get_image(&raw, &imgLen, &w, &h);
    printf("%d %d %d %d\n",imgLen,w,h,*raw);

    // if want to load to the CNN
    if(load_cnn)
    {
        // iterate over all pixels
        for(int i = 0; i < w; i++) // rows
        {
          for(int j = 0; j < h; j++) // cols
          {
              px1 = raw[2*(w*i+j)];
              px2 = raw[2*(w*i+j)+1];
              // extract the RGB values
              // RGB565 normally:   RRRRRGGG GGGBBBBB --> 16 bits
              ur = (px1 & 0b11111000);
              ug = ((((px1 & 0b00000111)<<5) | ((px2 & 0b11100000)>>3)));
              ub = (((px2 & 0b00011111))<<3);

              // convert from uint8_t to int8_t because CNN uses signed 8-bit data [-128, 127]
              r = ur-128;
              g = ug-128;
              b = ub-128;
              
              // load a word into the CNN buffer [00 BB GG RR] --> HWC format
              cnn_buffer[w*i+j] = 0x00FFFFFF & ((((uint8_t)b) << 16) | (((uint8_t)g) << 8) | ((uint8_t)r));
          }
        }
    }
    // display the image on the LCD
    MXC_TFT_ShowImageCameraRGB565(x_coord, y_coord, raw, h, w);
}

/***** LCD Functions *****/
void init_LCD()
{
   mxc_gpio_cfg_t tft_reset_pin = {MXC_GPIO0, MXC_GPIO_PIN_19, MXC_GPIO_FUNC_OUT, MXC_GPIO_PAD_NONE, MXC_GPIO_VSSEL_VDDIOH};
   MXC_TFT_Init(MXC_SPI0, 1, &tft_reset_pin, NULL);
   MXC_TFT_ClearScreen();
   MXC_TFT_SetBackGroundColor(4);

   printf(ANSI_COLOR_GREEN "--> LCD Initialized" ANSI_COLOR_RESET "\n");
   MXC_Delay(1000000);
}

void reset()
{
  mxc_gpio_cfg_t tft_reset_pin = {MXC_GPIO0, MXC_GPIO_PIN_19, MXC_GPIO_FUNC_OUT, MXC_GPIO_PAD_NONE, MXC_GPIO_VSSEL_VDDIOH};
  MXC_TFT_Init(MXC_SPI0, 1, &tft_reset_pin, NULL);
}


void TFT_Print(char *str, int x, int y, int font, int length)
{
    // fonts id
    text_t text;
    text.data = str;
    text.len = length;
    MXC_TFT_PrintFont(x, y, font, &text, NULL);
}


void LCD_Camera_Setup()
{
    // Initialize TFT display.
    init_LCD();

    // Initialize DMA for camera interface
    MXC_DMA_Init();
    int dma_channel = MXC_DMA_AcquireChannel();

    // Initialize camera.
    printf("Init Camera.\n");
    camera_init(CAMERA_FREQ);


    // Setup the camera image dimensions, pixel format and data acquiring details.
  	//int ret = camera_setup(get_image_x(), get_image_y(), PIXFORMAT_RGB565, FIFO_FOUR_BYTE, USE_DMA, dma_channel);
    int ret = camera_setup(get_image_x(), get_image_y(), PIXFORMAT_RGB888, FIFO_THREE_BYTE, STREAMING_DMA, dma_channel);
    if (ret != STATUS_OK) 
    {
      printf(ANSI_COLOR_RED "--> Error returned from setting up camera. Error %d" ANSI_COLOR_RESET "\n", ret);
	  }
    camera_write_reg(0x11, 0x1); // set camera clock prescaller to prevent streaming overflow

    printf(ANSI_COLOR_GREEN "--> Camera Initialized" ANSI_COLOR_RESET "\n");
}

void capture_process_camera(int x_coord, int y_coord, uint32_t* cnn_buffer, int load_cnn)
{
  // camera frame information
	uint8_t *raw;
	uint32_t imgLen;
	uint32_t w, h;

  // index for CNN buffer
	int cnt = 0;

  // used for converting formats
	uint8_t r, g, b;
	uint16_t rgb;
	int j = 0;

	uint8_t *data = NULL;

  // Get the details of the image from the camera driver.
	camera_start_capture_image();
	camera_get_image(&raw, &imgLen, &w, &h);

	// Get image line by line
	for (int row = 0; row < h; row++) 
  {
		// Wait until camera streaming buffer is full
		while ((data = get_camera_stream_buffer()) == NULL) 
    {
			if (camera_is_image_rcv())
      {
				break;
			}
		}

#ifdef BOARD_EVKIT_V1
			j = 128*2 - 2; // mirror on display
#else
		j = 0;
#endif
		for (int k = 0; k < 4 * w; k += 4) 
    {
			// data format: 0x00bbggrr
			r = data[k];
			g = data[k + 1];
			b = data[k + 2];
			//skip k+3

      if(load_cnn)
      {
        // change the range from [0,255] to [-128,127] and store in buffer for CNN
        cnn_buffer[cnt++] = ((b << 16) | (g << 8) | r) ^ 0x00808080;
      }

			// convert to RGB656 for display
			rgb = ((r & 0b11111000) << 8) | ((g & 0b11111100) << 3) | (b >> 3);
      
			data565[j] = (rgb >> 8) & 0xFF;
			data565[j + 1] = rgb & 0xFF;
#ifdef BOARD_EVKIT_V1
				j-=2; // mirror on display
#else
			j += 2;
#endif
		}
		MXC_TFT_ShowImageCameraRGB565(x_coord, y_coord + row, data565, w, 1);

		// Release stream buffer
		release_camera_stream_buffer();
	}

}

void get_font(int* font)
{
  *font = font_1;
}
