// ========================================================================================= //
// ======================================= HEADERS ========================================= //
// ========================================================================================= //

#include "asl_helper_funcs.h"
#include "cnn_helper_funcs.h"
#include "string.h"
#include "camera_tft_funcs.h"
#include "mxc.h"

// ========================================================================================= //
// ================================== GLOBAL VARIABLES ===================================== //
// ========================================================================================= //

#ifdef ASL
asl_t asl = {NOTHING, 0};
char letters[27] = {'\0'};
uint8_t num_letters = 0;
static area_t box = {0,0,350,60};
char* tokens[] = {"M","S","T","L","H","NOTHING","J","N"," ","K","B","I","C","A","O","G","Y","U","X","D","R","E","W","V","P","F","Q","DEL","Z"};

void update_state(void)
{
    static cnn_output_t output;
    run_cnn(&output);
    
    // cnn outputs the same token as last inference (don't include NOTHING token)
    if((output.output_class != NOTHING) && (output.output_class == asl.last_letter))
    {
        // increment the consecutive inference count
        asl.count += 1;

        // if this is the third inference in a row, reset the count and display the letter to the screen
        if(asl.count == 3)
        {
            asl.count = 0;
            // printf("Display letter\n");
            display_letters();
        }
    }
    else
    {
        // reset the count and update the last letter
        asl.count = 0;
        asl.last_letter = output.output_class;
    }
}


void display_letters(void)
{
    static int font = 0;
    static uint8_t num_del = 0;
    
    // first check if we have room
    if(num_letters == 26)
    {
        // printf("char buffer full\n");
        return;
    }

    // next check if we want to delete
    if(asl.last_letter == DEL)
    {
        num_del += 1;
        // check if char buffer already empty
        if(num_letters == 0)
        {
            // printf("char buffer already empty\n");
            return;
        }

        // clear all letters
        if(num_del == 3)
        {
            num_del = 0;
            num_letters = 0;
            letters[0] = '\0';
            letters[1] = '\0';
            MXC_TFT_FillRect(&box,4);
            return;
        }

        // if we have a non-empty char buffer, delete the current character
        letters[num_letters - 1] = '\0';
        num_letters -= 1;
        get_font(&font);
        MXC_TFT_FillRect(&box,4);
        if(num_letters >= 24)
        {
            TFT_Print(letters, 0, 0, font, 24);
            TFT_Print(letters+24, 0, 30, font, num_letters-24);
        }
        else
        {
            TFT_Print(letters, 0, 0, font, num_letters);
        }
        // printf("Letters: %s\n", letters);
        MXC_Delay(1000000);
        return;
    }
    num_del = 0;

    // otherwise, display the letter
    letters[num_letters] = tokens[asl.last_letter][0];
    num_letters += 1;
    get_font(&font);
    MXC_TFT_FillRect(&box,4);
    if(num_letters >= 24)
    {
        TFT_Print(letters, 0, 0, font, 24);
        TFT_Print(letters+24, 0, 30, font, num_letters-24);
    }
    else
    {
        TFT_Print(letters, 0, 0, font, num_letters);
    }
    MXC_Delay(1000000);
    // printf("Letters: %s\n", letters);
}
#endif