// ========================================================================================= //
// ======================================= HEADERS ========================================= //
// ========================================================================================= //

#include "cnn_helper_funcs.h"
#include "mxc_delay.h"
#include "mxc_device.h"
#include "mxc_sys.h"
#include "bbfc_regs.h"
#include "fcr_regs.h"
#include "cnn.h"
#include "camera_tft_funcs.h"
#include "mxc.h"
#include "camera.h"
#include "string.h"
#include "bitmap.h"

// ========================================================================================= //
// ======================================= MACROS ========================================== //
// ========================================================================================= //

#define SCREEN_W 128 // image output width
#define SCREEN_H 128 // image output height
#define SCREEN_X 100 // image output top left corner
#define SCREEN_Y 50 // image output top left corner
#define TFT_BUFF_SIZE   30 // TFT text buffer size

#ifdef CD
#define NUM_CLASSES 2 // number of output classes
#define NUM_OUTPUTS 2 // number of output neurons
#endif

#ifdef OFFICE
#define NUM_CLASSES 5 // number of output classes
#define NUM_OUTPUTS 5 // number of output neurons
#endif

#ifdef ASL
#define NUM_CLASSES 29 // number of output classes
#define NUM_OUTPUTS 29 // number of output neurons
#endif

#define CNN_INPUT_SIZE 16384 // data is 128 x 128 px = 16,384 px each word is 0RGB, byte for each

#define BB_COLOR YELLOW // the bounding box color
#define BB_W 2 // the bounding box width in pixels

// ========================================================================================= //
// ================================== GLOBAL VARIABLES ===================================== //
// ========================================================================================= //

uint32_t cnn_buffer[CNN_INPUT_SIZE]; // the input image data into the CNN (80*80 = 6400 bytes = 1600 words)
cnn_output_t output; // the output data of the CNN

static int32_t ml_data[NUM_OUTPUTS]; // output data
static q15_t ml_softmax[NUM_CLASSES]; // softmax output data
int font_1 = urw_gothic_12_white_bg_grey;

char buff[TFT_BUFF_SIZE];
volatile uint32_t cnn_time; // Stopwatch

#ifdef CD
char* class_strings[] = {"DOG","CAT"};
#endif

#ifdef OFFICE
char* class_strings[] = {"KEYBOARD","MOUSE","BACKPACK","MUG","MONITOR"};
#endif

#ifdef ASL
char* class_strings[] = {"M","S","T","L","H","NOTHING","J","N","SPACE","K","B","I","C","A","O","G","Y","U","X","D","R","E","W","V","P","F","Q","DEL","Z"};
#endif

static area_t box = {0,0,250,30};


// ========================================================================================= //
// ================================ FUNCTION DEFINITIONS =================================== //
// ========================================================================================= //

// this function loads the image data into the input layer's data memory instance
void load_input(void)
{
  int i;
  const uint32_t *in0 = cnn_buffer;

  for (i = 0; i < 16384; i++) {
    while (((*((volatile uint32_t *) 0x50000004) & 1)) != 0); // Wait for FIFO 0
    *((volatile uint32_t *) 0x50000008) = *in0++; // Write FIFO 0
  }
}


// ========================================================================================= //


// this function gets the classification data from the output layer
// data memory instance and passes it to the auto-generated softmax function
void softmax_layer(void)
{
  cnn_unload((uint32_t *) ml_data);
  softmax_q17p14_q15((const q31_t *) ml_data, NUM_CLASSES, ml_softmax);
}


// ========================================================================================= //


// simple getter function for the CNN input data buffer
uint32_t* get_cnn_buffer()
{
    return cnn_buffer;
}


// ========================================================================================= //


// this function does a forward pass through the CNN 
void run_cnn(cnn_output_t* output)
{
    int class_sums[] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    int digs, tens; // format probability
    int max = 0; // the current highest class probability
    int max_i = 0; // the class with the highest probability
    

    for(int i = 0; i < 1; i++)
    {
        // first get an image from the camera and load it into the CNN buffer
        capture_process_camera(SCREEN_X,SCREEN_Y, cnn_buffer,true);
        
        // Enable CNN clock
        MXC_SYS_ClockEnable(MXC_SYS_PERIPH_CLOCK_CNN);

        cnn_init(); // Bring state machine into consistent state
        cnn_configure(); // Configure state machine

        cnn_start();
        load_input(); 
        while (cnn_time == 0)
        __WFI(); // Wait for CNN

        // classify the output
        softmax_layer();

        printf("Classification results:\n");
        // might want to average across runs
        for (int i = 0; i < NUM_CLASSES; i++) 
        {
          // softmax output is in fixed point so need to convert to percentage
          digs = (1000 * ml_softmax[i] + 0x4000) >> 15;
          tens = digs % 10; // get the fractional part
          digs = digs / 10; // get the integer part
          printf("[%7d] -> Class %d: %d.%d%%\n", ml_data[i], i, digs, tens);

          class_sums[i] += digs;
          if(class_sums[i] > max)
          {
            max = class_sums[i];
            max_i = i;
          }
        }
        // Disable CNN clock to save power
        cnn_stop();
        MXC_SYS_ClockDisable(MXC_SYS_PERIPH_CLOCK_CNN);

        printf("Approximate inference time: %u us\n\n", cnn_time);
    }
    output->output_class = max_i;
    output->percent = max;
    printf("CLASS: %s\n",class_strings[max_i]);
    printf("\033[0;0f");
}


// ========================================================================================= //


void startup_cnn()
{
    // Enable peripheral, enable CNN interrupt, turn on CNN clock
    // CNN clock: 50 MHz div 1
    cnn_enable(MXC_S_GCR_PCLKDIV_CNNCLKSEL_PCLK, MXC_S_GCR_PCLKDIV_CNNCLKDIV_DIV1);

    cnn_init(); // Bring state machine into consistent state
    cnn_load_weights(); // Load kernels
    cnn_load_bias();
    cnn_configure(); // Configure state machine
}


// ========================================================================================= //


void show_cnn_output(cnn_output_t* output)
{
    static cnn_output_t last_output;

    // only update text when class changes
    // if(last_output.output_class != (*output).output_class)
    // {
        memset(buff,32,TFT_BUFF_SIZE);
        MXC_TFT_FillRect(&box,4);
        TFT_Print(buff, 0, 0, font_1, sprintf(buff, "Class: %s (%d %%)", class_strings[output->output_class], output->percent));
    // }
    // last_output.output_class = output->output_class; 
}