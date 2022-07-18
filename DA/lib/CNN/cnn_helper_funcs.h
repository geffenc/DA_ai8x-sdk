// This file defines helper functions for setting up and running the CNN
// Note that the code in CNN.c and CNN.h is autogenerated by the Maxim Integrated
// synthesis tool when syntheszing a trained PyTorch model

#ifndef CNN_HELPER_FUNCS_H
#define CNN_HELPER_FUNCS_H

#include <stdint.h>

extern char* class_strings[];

#define ASL

#ifdef CD
typedef enum
{
    DOG=0,
    CAT
} output_classes_t;
#endif

#ifdef OFFICE
typedef enum
{
    KEYBOARD=0,
    MOUSE,
    BACKPACK,
    MUG,
    MONITOR
} output_classes_t;
#endif

#ifdef ASL
typedef enum
{
    M=0,S,T,L,H,NOTHING,J,N,SPACE,K,B,I,C,A,O,G,Y,U,X,D,R,E,W,V,P,F,Q,DEL,Z
} output_classes_t;
#endif

// this struct stores the output information from the CNN
// we may want to add bounding box variables later
typedef struct 
{
    output_classes_t output_class;
    uint8_t percent;
}cnn_output_t;


/*
    Description: This function loads the image data into 
                 the starting layer's data memory instance.
                 This gets called every time there is a forward pass.
    Parameters: none
    Return: none
*/
void load_input(void);


/*
    Description: This function gets the classification output
                 from the output layer's data memory instance and passes it to
                 the auto-generated softmax function to get probability of each class
    Parameters: none
    Return: none
*/
void softmax_layer(void);


/*
    Description: A simple getter function to access the cnn input data buffer. This
                 buffer stores the image data as signed 8 bit values packed
                 into a 32 bit words. This buffer is read by load_input().
    Parameters: none
    Return: pointer to the cnn buffer
*/
uint32_t* get_cnn_buffer();


/*
    Description: This initializes the CNN by loading the weights and biases.
                 This function only needs to be called once.
    Parameters: none
    Return: none
*/
void startup_cnn();


/*
    Description: This function essentially does one forward pass through
                 the network (inference). Each time is gets called it will
                 start the CNN, load the input data, wait for completion,
                 stop the CNN, and return the output data through a pointer to a struct.
    Parameters: None.
    Return: the output of the CNN
*/
void run_cnn(cnn_output_t* output);

/*
    Description: This function shows the output of the CNN
    Parameters: output of the CNN.
    Return: Nine
*/
void show_cnn_output(cnn_output_t* output);

#endif