/*******************************************************************************
* Copyright (C) 2019-2022 Maxim Integrated Products, Inc., All rights Reserved.
*
* This software is protected by copyright laws of the United States and
* of foreign countries. This material may also be protected by patent laws
* and technology transfer regulations of the United States and of foreign
* countries. This software is furnished under a license agreement and/or a
* nondisclosure agreement and may only be used or reproduced in accordance
* with the terms of those agreements. Dissemination of this information to
* any party or parties not specified in the license agreement and/or
* nondisclosure agreement is expressly prohibited.
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
*******************************************************************************/

// ASL_DA_k8_new
// Created using ai8xize.py --verbose --log --test-dir sdk/Examples/MAX78000/CNN --prefix ASL_DA_k8_new --checkpoint-file ../ai8x-training/jupyter_logging/asl_dcd_train_adv_k8run3___2022.07.18-175442/aslclassifier_qat_best-q.pth.tar --config-file networks/classifier.yaml --device MAX78000 --softmax --compact-data --mexpress --timer 0 --fifo --display-checkpoint

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "mxc.h"
#include "cnn.h"
#include "sampledata.h"
#include "sampleoutput.h"

volatile uint32_t cnn_time; // Stopwatch

void fail(void)
{
  printf("\n*** FAIL ***\n\n");
  while (1);
}

// Data input: HWC 3x128x128 (49152 bytes total / 16384 bytes per channel):
static const uint32_t input_0[] = SAMPLE_INPUT_0;
void load_input(void)
{
  // This function loads the sample data input -- replace with actual data

  int i;
  const uint32_t *in0 = input_0;

  for (i = 0; i < 16384; i++) {
    // Remove the following line if there is no risk that the source would overrun the FIFO:
    while (((*((volatile uint32_t *) 0x50000004) & 1)) != 0); // Wait for FIFO 0
    *((volatile uint32_t *) 0x50000008) = *in0++; // Write FIFO 0
  }
}

// Expected output of layer 12 for ASL_DA_k8_new given the sample input (known-answer test)
// Delete this function for production code
static const uint32_t sample_output[] = SAMPLE_OUTPUT;
int check_output(void)
{
  int i;
  uint32_t mask, len;
  volatile uint32_t *addr;
  const uint32_t *ptr = sample_output;

  while ((addr = (volatile uint32_t *) *ptr++) != 0) {
    mask = *ptr++;
    len = *ptr++;
    for (i = 0; i < len; i++)
      if ((*addr++ & mask) != *ptr++) {
        printf("Data mismatch (%d/%d) at address 0x%08x: Expected 0x%08x, read 0x%08x.\n",
               i + 1, len, addr - 1, *(ptr - 1), *(addr - 1) & mask);
        return CNN_FAIL;
      }
  }

  return CNN_OK;
}

// Classification layer:
static int32_t ml_data[CNN_NUM_OUTPUTS];
static q15_t ml_softmax[CNN_NUM_OUTPUTS];

void softmax_layer(void)
{
  cnn_unload((uint32_t *) ml_data);
  softmax_q17p14_q15((const q31_t *) ml_data, CNN_NUM_OUTPUTS, ml_softmax);
}

int main(void)
{
  int i;
  int digs, tens;

  MXC_ICC_Enable(MXC_ICC0); // Enable cache

  // Switch to 100 MHz clock
  MXC_SYS_Clock_Select(MXC_SYS_CLOCK_IPO);
  SystemCoreClockUpdate();

  printf("Waiting...\n");

  // DO NOT DELETE THIS LINE:
  MXC_Delay(SEC(2)); // Let debugger interrupt if needed

  // Enable peripheral, enable CNN interrupt, turn on CNN clock
  // CNN clock: 50 MHz div 1
  cnn_enable(MXC_S_GCR_PCLKDIV_CNNCLKSEL_PCLK, MXC_S_GCR_PCLKDIV_CNNCLKDIV_DIV1);

  printf("\n*** CNN Inference Test ***\n");

  cnn_init(); // Bring state machine into consistent state
  cnn_load_weights(); // Load kernels
  cnn_load_bias();
  cnn_configure(); // Configure state machine
  cnn_start(); // Start CNN processing
  load_input(); // Load data input via FIFO

  SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk; // SLEEPDEEP=0
  while (cnn_time == 0)
    __WFI(); // Wait for CNN

  if (check_output() != CNN_OK) fail();
  softmax_layer();

  printf("\n*** PASS ***\n\n");

#ifdef CNN_INFERENCE_TIMER
  printf("Approximate data loading and inference time: %u us\n\n", cnn_time);
#endif

  cnn_disable(); // Shut down CNN clock, disable peripheral

  printf("Classification results:\n");
  for (i = 0; i < CNN_NUM_OUTPUTS; i++) {
    digs = (1000 * ml_softmax[i] + 0x4000) >> 15;
    tens = digs % 10;
    digs = digs / 10;
    printf("[%7d] -> Class %d: %d.%d%%\n", ml_data[i], i, digs, tens);
  }

  return 0;
}

/*
  SUMMARY OF OPS
  Hardware: 118,560,768 ops (117,067,392 macc; 1,493,376 comp; 0 add; 0 mul; 0 bitwise)
    Layer 0: 7,340,032 ops (7,077,888 macc; 262,144 comp; 0 add; 0 mul; 0 bitwise)
    Layer 1: 19,136,512 ops (18,874,368 macc; 262,144 comp; 0 add; 0 mul; 0 bitwise)
    Layer 2: 9,830,400 ops (9,437,184 macc; 393,216 comp; 0 add; 0 mul; 0 bitwise)
    Layer 3: 19,005,440 ops (18,874,368 macc; 131,072 comp; 0 add; 0 mul; 0 bitwise)
    Layer 4: 9,633,792 ops (9,437,184 macc; 196,608 comp; 0 add; 0 mul; 0 bitwise)
    Layer 5: 18,939,904 ops (18,874,368 macc; 65,536 comp; 0 add; 0 mul; 0 bitwise)
    Layer 6: 9,535,488 ops (9,437,184 macc; 98,304 comp; 0 add; 0 mul; 0 bitwise)
    Layer 7: 18,907,136 ops (18,874,368 macc; 32,768 comp; 0 add; 0 mul; 0 bitwise)
    Layer 8: 4,759,552 ops (4,718,592 macc; 40,960 comp; 0 add; 0 mul; 0 bitwise)
    Layer 9: 1,189,888 ops (1,179,648 macc; 10,240 comp; 0 add; 0 mul; 0 bitwise)
    Layer 10: 262,400 ops (262,144 macc; 256 comp; 0 add; 0 mul; 0 bitwise)
    Layer 11: 16,512 ops (16,384 macc; 128 comp; 0 add; 0 mul; 0 bitwise)
    Layer 12: 3,712 ops (3,712 macc; 0 comp; 0 add; 0 mul; 0 bitwise)

  RESOURCE USAGE
  Weight memory: 288,216 bytes out of 442,368 bytes total (65%)
  Bias memory:   557 bytes out of 2,048 bytes total (27%)
*/

