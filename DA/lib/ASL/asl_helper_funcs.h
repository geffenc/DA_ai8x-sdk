// this file defines some helper functions to make an application from the
// ASL CNN outputs

#ifndef ASL_HELPER_FUNCS
#define ASL_HELPER_FUNCS

#include "stdint.h"
#include "cnn_helper_funcs.h"


typedef struct 
{
    output_classes_t last_letter; // the current state
    uint8_t count; // consecutive inferences in current state 
} asl_t;


/*
    Description: This function does an inference and then updates
                 the state based on the CNN result.
    Parameters: none
    Return: none
*/
void update_state(void);

/*
    Description: This function will display the characters on
                 the screen based on the state
    Parameters: none
    Return: none
*/
void display_letters(void);

#endif