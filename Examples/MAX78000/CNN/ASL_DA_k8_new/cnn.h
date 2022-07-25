/**************************************************************************************************
* Copyright (C) 2019-2021 Maxim Integrated Products, Inc. All Rights Reserved.
*
* Maxim Integrated Products, Inc. Default Copyright Notice:
* https://www.maximintegrated.com/en/aboutus/legal/copyrights.html
**************************************************************************************************/

/*
 * This header file was automatically generated for the ASL_DA_k8_new network from a template.
 * Please do not edit; instead, edit the template and regenerate.
 */

#ifndef __CNN_H__
#define __CNN_H__

#include <stdint.h>
typedef int32_t q31_t;
typedef int16_t q15_t;

/* Return codes */
#define CNN_FAIL 0
#define CNN_OK 1

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

/* Number of outputs for this network */
#define CNN_NUM_OUTPUTS 29

/* Use this timer to time the inference */
#define CNN_INFERENCE_TIMER MXC_TMR0

/* Port pin actions used to signal that processing is active */

#define CNN_START LED_On(1)
#define CNN_COMPLETE LED_Off(1)
#define SYS_START LED_On(0)
#define SYS_COMPLETE LED_Off(0)

/* Run software SoftMax on unloaded data */
void softmax_q17p14_q15(const q31_t * vec_in, const uint16_t dim_vec, q15_t * p_out);
/* Shift the input, then calculate SoftMax */
void softmax_shift_q17p14_q15(q31_t * vec_in, const uint16_t dim_vec, uint8_t in_shift, q15_t * p_out);

/* Stopwatch - holds the runtime when accelerator finishes */
extern volatile uint32_t cnn_time;

/* Custom memcopy routines used for weights and data */
void memcpy32(uint32_t *dst, const uint32_t *src, int n);
void memcpy32_const(uint32_t *dst, int n);

/* Enable clocks and power to accelerator, enable interrupt */
int cnn_enable(uint32_t clock_source, uint32_t clock_divider);

/* Disable clocks and power to accelerator */
int cnn_disable(void);

/* Perform minimum accelerator initialization so it can be configured */
int cnn_init(void);

/* Configure accelerator for the given network */
int cnn_configure(void);

/* Load accelerator weights */
int cnn_load_weights(void);

/* Verify accelerator weights (debug only) */
int cnn_verify_weights(void);

/* Load accelerator bias values (if needed) */
int cnn_load_bias(void);

/* Start accelerator processing */
int cnn_start(void);

/* Force stop accelerator */
int cnn_stop(void);

/* Continue accelerator after stop */
int cnn_continue(void);

/* Unload results from accelerator */
int cnn_unload(uint32_t *out_buf);

/* Turn on the boost circuit */
int cnn_boost_enable(mxc_gpio_regs_t *port, uint32_t pin);

/* Turn off the boost circuit */
int cnn_boost_disable(mxc_gpio_regs_t *port, uint32_t pin);

#endif // __CNN_H__
