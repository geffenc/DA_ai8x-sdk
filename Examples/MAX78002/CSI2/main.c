/**
 * @file    main.c
 * @brief   MIPI CSI-2 Demo.
 * @details This example captures an image using a MIPI Camera and an CSI-2 Receiver.
 */

/******************************************************************************
* Copyright (C) 2022 Maxim Integrated Products, Inc., All Rights Reserved.
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

/***** Includes *****/
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "mxc_device.h"
#include "mxc_sys.h"
#include "mxc_delay.h"
#include "mxc_errors.h"
#include "csi2.h"
#include "csi2_regs.h"
#include "dma.h"
#include "icc.h"
#include "pb.h"
#include "led.h"
#include "board.h"
#include "nvic_table.h"
#include "mipi_camera.h"
#include "utils.h"
#include "gcr_regs.h"
#include "mcr_regs.h"

/***** Definitions *****/

// Select how to receive data
#define DMA
// Options:
//      SYNC
//      ASYNC
//      DMA

#ifdef SYNC 
#error Incompatible mode. Not yet supported.
#endif
#ifdef ASYNC 
#error Incompatible mode. Not yet supported.
#endif

#define IMAGE_WIDTH             128
#define IMAGE_HEIGHT            128

// Check CSI-2 Standard and your color format for these values.
#define BITS_PER_PIXEL_ODD      24      // RGB888
#define BITS_PER_PIXEL_EVEN     24      // RGB888

#define NUM_DATA_LANES          2
#define FLUSH_COUNT             3
#define VIRTUAL_CHANNEL         0x00
#define RX_THRESHOLD            0x20
#define WAIT_CYCLE              0x2000
#define FLOW_CTRL               (MXC_F_CSI2_VFIFO_CFG1_WAIT_FIRST_FS | MXC_F_CSI2_VFIFO_CFG1_ACCU_FRAME_CTRL | \
                                    MXC_F_CSI2_VFIFO_CFG1_ACCU_LINE_CNT | MXC_F_CSI2_VFIFO_CFG1_ACCU_PIXEL_CNT)

// Select corresponding pixel format and output sequence from OV5640 Datasheet.
#define PIXEL_FORMAT            MIPI_PIXFORMAT_RAW
#define OUT_SEQ                 0x0        // BGBG GRGR
#define MUX_CTRL                1        // ISP RGB

// Streaming pixel format may not match pixel format the camera originally processed.
//    For example, the camera can processes RAW then converts to RGB888.
#define STREAM_PIXEL_FORMAT     MIPI_PIXFORMAT_RGB888

// Size of Image Buffer
#define SIZE ((BITS_PER_PIXEL_ODD + BITS_PER_PIXEL_EVEN) * IMAGE_WIDTH * IMAGE_HEIGHT) >> 4

// Update for future cameras
#if defined(CAMERA_OV5640)
#define CAMERA_ID               0x5640
#else // Default
#define CAMERA_ID               0x5640
#endif

/***** Globals *****/

volatile int RX_FLAG;
volatile int DMA_FLAG;

// RAW Line Buffers
uint32_t RAW_ADDR0[IMAGE_WIDTH] __attribute__ ((aligned (4))) __attribute__ ((section (".csi2_buff_section")));
uint32_t RAW_ADDR1[IMAGE_WIDTH] __attribute__ ((aligned (4))) __attribute__ ((section (".csi2_buff_section")));

// Buffer for processed image
uint8_t IMAGE[SIZE];

/***** Functions *****/

void DMA_Handler(void)
{
    MXC_DMA_Handler();

    DMA_FLAG = 0;
}

void CSI2_Handler(void)
{
    MXC_CSI2_Handler();
}

void rxCallback(mxc_csi2_req_t *req, int error)
{
    RX_FLAG = error;
}

void process_img(void)
{
    uint8_t*   raw;
    uint32_t  imgLen;
    uint32_t  w, h;

#ifdef SYNC
    MXC_CSI2_CaptureFrame(NUM_DATA_LANES);
#endif
#ifdef ASYNC
    MXC_CSI2_CaptureFrameAsync(NUM_DATA_LANES);

    while(RX_FLAG);

    if(RX_FLAG != E_NO_ERROR) {
        printf("Error with CSI2 RX Callback: %d\n", RX_FLAG);
        LED_On(1);
    }

    RX_FLAG = 1;
#endif
#ifdef DMA
    MXC_CSI2_CaptureFrameDMA(NUM_DATA_LANES);

    while(DMA_FLAG);

    DMA_FLAG = 1;
#endif

    // Get the details of the image from the camera driver.
    MXC_CSI2_GetImageDetails(&raw, &imgLen, &w, &h);

    utils_send_img_to_pc(raw, imgLen, w, h, mipi_camera_get_pixel_format(STREAM_PIXEL_FORMAT));
}

volatile int buttonPressed = 0;
void buttonHandler()
{
    buttonPressed = 1;
}

int main(void)
{
    int error;
    int csi2_dma_channel;
    int id;
    mxc_csi2_req_t req;
    mxc_csi2_ctrl_cfg_t ctrl_cfg;
    mxc_csi2_vfifo_cfg_t vfifo_cfg;

    printf("\n\n**** MIPI CSI-2 Example ****\n");
    printf("This example streams the image data through the COM port\n");
    printf("and a script running on the host pc converts the data into\n");
    printf("a .png image. Note: You can not run the script and have\n");
    printf("a serial terminal open running on the same COM port at the\n");
    printf("the same time.\n");
    printf("\nGo into the pc_utility folder and run the script:\n");
    printf("python grab_image.py [COM#] 115200\n");
    printf("\nPress PB1 (SW4) to trigger a frame capture.\n");

    // Enable cache
    MXC_ICC_Enable(MXC_ICC0);

    // Switch to 100 MHz clock
    MXC_SYS_Clock_Select(MXC_SYS_CLOCK_IPO);
    SystemCoreClockUpdate();

    // Initialize camera
    mipi_camera_init();

    // Confirm correct camera is connected
    mipi_camera_get_product_id(&id);
    if(id != CAMERA_ID) {
        printf("Incorrect camera.\n");
        LED_On(1);
        while(1);
    }

    mipi_camera_setup(IMAGE_WIDTH, IMAGE_HEIGHT, PIXEL_FORMAT, OUT_SEQ, MUX_CTRL);

    // Configure RX Controller and PPI (D-PHY)
    ctrl_cfg.invert_ppi_clk = MXC_CSI2_PPI_NO_INVERT;
    ctrl_cfg.num_lanes = NUM_DATA_LANES;   
    ctrl_cfg.payload0 = MXC_CSI2_PL0_RAW10;
    ctrl_cfg.payload1 = MXC_CSI2_PL1_DISABLE_ALL;
    ctrl_cfg.flush_cnt = FLUSH_COUNT;

    ctrl_cfg.lane_src.d0_swap_sel = MXC_CSI2_PAD_CDRX_PN_L0;
    ctrl_cfg.lane_src.d1_swap_sel = MXC_CSI2_PAD_CDRX_PN_L1;
    ctrl_cfg.lane_src.d2_swap_sel = MXC_CSI2_PAD_CDRX_PN_L2;
    ctrl_cfg.lane_src.d3_swap_sel = MXC_CSI2_PAD_CDRX_PN_L3;
    ctrl_cfg.lane_src.c0_swap_sel = MXC_CSI2_PAD_CDRX_PN_L4;

    // Image Data
    req.img_addr = IMAGE;
    req.pixels_per_line = IMAGE_WIDTH;
    req.lines_per_frame = IMAGE_HEIGHT;
    req.bits_per_pixel_odd = BITS_PER_PIXEL_ODD;
    req.bits_per_pixel_even = BITS_PER_PIXEL_EVEN;
    req.frame_num = 1;
#ifdef ASYNC
    req.callback = rxCallback;
    RX_FLAG = 1;
#endif

    // Convert RAW to RGB
    req.process_raw_to_rgb = true;
    req.rgb_type = MXC_CSI2_TYPE_RGB888;
    req.raw_format = MXC_CSI2_FORMAT_BGBG_GRGR;
    req.autoflush = MXC_CSI2_AUTOFLUSH_ENABLE;
    req.raw_buf0_addr = (uint32_t) RAW_ADDR0;
    req.raw_buf1_addr = (uint32_t) RAW_ADDR1;

    // Configure VFIFO
    vfifo_cfg.virtual_channel = VIRTUAL_CHANNEL;
    vfifo_cfg.rx_thd = RX_THRESHOLD;
    vfifo_cfg.wait_cyc = WAIT_CYCLE;
    vfifo_cfg.flow_ctrl  = FLOW_CTRL;
    vfifo_cfg.err_det_en = MXC_CSI2_ERR_DETECT_DISABLE;
    vfifo_cfg.fifo_rd_mode = MXC_CSI2_READ_ONE_BY_ONE;
    vfifo_cfg.bandwidth_mode = MXC_CSI2_NORMAL_BW;
    vfifo_cfg.dma_whole_frame = MXC_CSI2_DMA_LINE_BY_LINE;
    vfifo_cfg.dma_mode = MXC_CSI2_DMA_FIFO_ABV_THD;
    vfifo_cfg.wait_en = MXC_CSI2_AHBWAIT_DISABLE;

    error = MXC_CSI2_Init(&req, &ctrl_cfg, &vfifo_cfg);
    if(error != E_NO_ERROR) {
        printf("Error Initializating.\n\n");
        while(1);
    }

#ifdef DMA
    csi2_dma_channel = MXC_CSI2_DMA_GetChannel();
    NVIC_SetVector(DMA0_IRQn + csi2_dma_channel, DMA_Handler);
#endif
    NVIC_SetVector(CSI2_IRQn, CSI2_Handler);

    PB_RegisterCallback(0, (pb_callback) buttonHandler);

    while (1) {
        LED_On(0);

        if (buttonPressed) {
            LED_Off(0);

            process_img();

            MXC_Delay(SEC(3));

            buttonPressed = 0;
        }
    }
}
