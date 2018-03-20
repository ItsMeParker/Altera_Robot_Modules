/*******************************************************************************
 * 
 *                         Connor Parker  &  Alex Lord
 *                            Marco Robot Coursework
 * 
 *******************************************************************************/
/*****************************************************************
* Module name: LightFollower
*
* Copyright 1997 Company as an unpublished work.
* All Rights Reserved.
*
* The information contained herein is confidential
* property of Company. The user, copying, transfer or
* disclosure of such information is prohibited except
* by express written agreement with Company.
* 
* Authors   :   Connor Parker  &  Alex Lord
*
* First written on 26/01/17 by Connor Parker.
*
* Module Description:
* -------------------
* The following module will allow a MARCO robot to:
*
*   Rotate in correct direction             YES
*
*   Moves towards light at good speed       KINDA
*
*   Proportional speed to light intensity   NO
*
*   Scans during movement                   KINDA
*
*   Stops at an obstacle                    YES
*
*****************************************************************
*  Includes section
*****************************************************************/

/* Standard Altera include files to enable the mapping of names
To hardware addresses etc. */
#include "system.h"
#include "altera_avalon_pio_regs.h"
#include "alt_types.h"

#include <unistd.h>
#include <stdio.h>

/*****************************************************************
*  Defines section
*****************************************************************/

/* Directions */
#define STOP             0xFFFFFFFC
#define FORWARD           0xFFFFFFFF
#define LEFT_ONE_MOTOR   0xFFFFFFFA
#define RIGHT_ONE_MOTOR  0xFFFFFFF5
#define LEFT_BOTH_MOTOR  0xFFFFFFFB
#define RIGHT_BOTH_MOTOR 0xFFFFFFF7

/* Sensors */
#define LEFT_FLOOR_SENSOR  0x4000
#define RIGHT_FLOOR_SENSOR 0x2000
#define LEFT_FRONT_BUMPER  0x8000
#define RIGHT_FRONT_BUMPER 0x800

/* Switches */
#define LEFT_EYE_SWITCH   0x20000
#define RIGHT_EYE_SWITCH  0x10000

/* Flags for reading ADC */
#define START_FLAG 0x8000         
#define DONE_FLAG  0x8000    

/* BOOLEAN */
#define FALSE 0
#define TRUE  1

/*****************************************************************
*  Function Prototype Section
*****************************************************************/

void checkObstruction(void);

alt_u16 read_adc(alt_u8 channel);

void makeTurn(alt_u32 direction, int duration);

void calcTurn(int light_start, int light_end, int current_dir_start, int current_dir_end, alt_u32 totalSteps);

/****************************************************************/

int main()
{
    /* 32 bit unsigned variable to allow us to interact with
     * the Marco hardware */
    alt_u32 output, header, totalSteps, currentStep;

    alt_u32 steps[8] = { 0x8FFFFFFF,
                         0x9FFFFFFF,
                         0x1FFFFFFF,
                         0x5FFFFFFF,
                         0x4FFFFFFF,
                         0x6FFFFFFF,
                         0x2FFFFFFF,
                         0xAFFFFFFF };

    alt_u8 direction;
    int stepNum, light, light_start, light_end, first_below_200, current_dir_start, current_dir_end, light_middle, light_previous, light_total, light_half;
    float percent;

    /* This sets the direction for bits on the expansion header.
    A ‘1’ means it’s writable ‘0’ readable. */
    IOWR_ALTERA_AVALON_PIO_DIRECTION(EXPANSION_JP1_BASE,0xF000000F);

    /* initialise variables */
    stepNum = 0;
    totalSteps = 0;
    currentStep = 0;
    percent = 0.0;
    first_below_200 = 0;
    light_start = -50;
    light_end = -50;
    light_middle = 0;
    current_dir_start = 1;
    current_dir_end = 1;
    light_half = 0;
    light_total = -50;
    light_previous = -50;
    direction = 1;
    
    /* initialise outputs to STOP */
    output = STOP;
    /* pass initialised inputs to header */
    IOWR_ALTERA_AVALON_PIO_DATA(EXPANSION_JP1_BASE,output);
    
    /* initialisation - turn light sensor left until it hits the left switch */
    while(direction == 1)
    {
        for(stepNum = 0; stepNum < 8; stepNum++)
        {
            /* bitwise OR to wipe stepper section of output while maintaining motor values */
            output |= 0xFFFFFFF0;
            /* bitwise AND applys steps[] value to ouput while still maintaining motor values */
            output &= (steps[stepNum]);
            /* Apply output variable to header to control stepper motor */
            IOWR_ALTERA_AVALON_PIO_DATA(EXPANSION_JP1_BASE,output);

            usleep(2000);

            /* read value of header into header variable*/
            header = IORD_ALTERA_AVALON_PIO_DATA(EXPANSION_JP1_BASE);
            /* when left switch hit, change direction */
            if (!(header & LEFT_EYE_SWITCH))
            {
                direction = 0;
            }

        }
    }
    
    /* still initialising - turn light sensor right counting number of steps for a full rotation */
    while(direction == 0)
    {
        for(stepNum = 7; stepNum >= 0; stepNum--)
        {
            /* increment for each step */
            totalSteps++;
            
            /* bitwise OR to wipe stepper section of output while maintaining motor values */
            output |= 0xFFFFFFF0;
            /* bitwise AND applys steps[] value to ouput while still maintaining motor values */
            output &= (steps[stepNum]);
            /* Apply output variable to header to control stepper motor */
            IOWR_ALTERA_AVALON_PIO_DATA(EXPANSION_JP1_BASE,output);

            usleep(2000);

            /* read value of header into header variable*/
            header = IORD_ALTERA_AVALON_PIO_DATA(EXPANSION_JP1_BASE);
            /* when right switch hit, change direction */
            if (!(header & RIGHT_EYE_SWITCH))
            {
                direction = 1;
            }

        }
    }
    
    /* main loop */
    while(1){
        /* If direction is 1 (going left) */
        if(direction == 1){
            for(stepNum = 0; stepNum < 8; stepNum++)
            {   
                /* increment for each step */
                currentStep++;  
                
                output = STOP; 
                          
                /* bitwise OR to wipe stepper section to allow steps[] to be applied to output while maintaining motor values */
                output |= 0xFFFFFFF0;
                /* bitwise AND applys steps[] value to ouput while still maintaining motor values */
                output &= (steps[stepNum]);
                /*Apply output variable to header to control stepper motor */
                IOWR_ALTERA_AVALON_PIO_DATA(EXPANSION_JP1_BASE,output);

                // reads the adc on the bot - get light value
                light = read_adc(1);
                // allows adc to finish reading
                usleep(2500);
                
                output = FORWARD & steps[stepNum];

                IOWR_ALTERA_AVALON_PIO_DATA(EXPANSION_JP1_BASE,output);

                usleep(1500);
                
                /* 
                 * Only enter if light value exceeds threshold and end value not yet found
                 */
                if((light > 300) && (first_below_200 == FALSE)){
                    light_start = currentStep;
                    current_dir_start = direction;  
                    first_below_200 = TRUE;                         
                }
                /* 
                 * Only enter if light value drops below threshold and start value has been found 
                 */
                if((light < 300) && (first_below_200 == TRUE)){
                    light_end = currentStep;
                    current_dir_end = direction;  
                    first_below_200 = FALSE;
                    /* calculate amount to turn depending on position of light cone */   
                    calcTurn(light_start, light_end, current_dir_start, current_dir_end, totalSteps);
                    /* reset values for next loop */
                    light_start = -50;
                    light_end = -50; 
                }
              
                /* read value of header into header variable*/
                header = IORD_ALTERA_AVALON_PIO_DATA(EXPANSION_JP1_BASE);
                if (!(header & LEFT_EYE_SWITCH))
                {
                    /* start turning right */
                    direction = 0;
                    /* re-initialise value to account for discrepancies caused by hardware */
                    currentStep = totalSteps;
                }
            }
        }
        
        /* If direction is 0 (going right) */
        if(direction == 0){      

            for(stepNum = 7; stepNum >= 0; stepNum--)
            {
                /* decrement for each step */
                currentStep--;
                
                output = STOP;
                
                /* bitwise OR to wipe stepper section of output while maintaining motor values */
                output |= 0xFFFFFFF0;
                /* bitwise AND applys steps[] value to ouput while still maintaining motor values */
                output &= (steps[stepNum]);

                /*Apply output variable to header to control stepper motor */
                IOWR_ALTERA_AVALON_PIO_DATA(EXPANSION_JP1_BASE,output);
                
                // reads the adc on the bot 
                light = read_adc(1);
                //allows adc to finish reading will probably cause problems if lowered definately causes problems if zero
                usleep(2500);
                
                output = FORWARD & steps[stepNum];

                IOWR_ALTERA_AVALON_PIO_DATA(EXPANSION_JP1_BASE,output);
                
                usleep(1500);  
                          
                /* 
                 * Only enter if light value exceeds threshold and end value not yet found
                 */
                if((light > 300) && (first_below_200 == FALSE)){
                    light_start = currentStep;
                    current_dir_start = direction;  
                    first_below_200 = TRUE;                         
                }                
                /* 
                 * Only enter if light value drops below threshold and start value has been found 
                 */
                if((light < 300) && (first_below_200 == TRUE)){
                    light_end = currentStep;
                    current_dir_end = direction;  
                    first_below_200 = FALSE;
                    /* calculate amount to turn depending on position of light cone */   
                    calcTurn(light_start, light_end, current_dir_start, current_dir_end, totalSteps);
                    /* reset values for next loop */
                    light_start = -50;
                    light_end = -50; 
                }
                   
                /* read value of header into header variable*/
                header = IORD_ALTERA_AVALON_PIO_DATA(EXPANSION_JP1_BASE);
                if (!(header & RIGHT_EYE_SWITCH))
                {
                    /* start turning left */
                    direction = 1;
                    /* re-initialise value to account for discrepancies caused by hardware */
                    currentStep = 0;
                }
            }
        }
        
        // check for and obstruction after every loop of mainLoop
        checkObstruction();

    }
}


/****************************************************************
* Function name     : checkObstruction
*    returns        : void                     
*    arg1           : void 
* Created by        : Connor Parker
* Date created      : 10/02/17
* Description       : Checks for objects activaing the sensors on
*                     the front of the bot. If an obstruction is 
*                     found movement will be stopped untill the
*                     it is removed                     
* Notes             : n/a
****************************************************************/
void checkObstruction()
{ 
    alt_u32 header;

    alt_u8 blocked;
    
    IOWR_ALTERA_AVALON_PIO_DATA(EXPANSION_JP1_BASE,STOP);    

    /* read value of header into header variable*/        
    header = IORD_ALTERA_AVALON_PIO_DATA(EXPANSION_JP1_BASE);

    /* if either front sensor detects a blockage enter if*/
    if (((header & LEFT_FRONT_BUMPER ) != 32768) || ((RIGHT_FRONT_BUMPER & 0x800) != 2048))
    {
        blocked = 1;

        while(blocked == 1)
        {

            /* read value of header into header variable*/        
            header = IORD_ALTERA_AVALON_PIO_DATA(EXPANSION_JP1_BASE);        

            if (((header & LEFT_FRONT_BUMPER ) == 32768) && ((RIGHT_FRONT_BUMPER & 0x800) == 2048))   
            {
                blocked = 0;
            }

            usleep(500);

        }
    }
}


/****************************************************************
* Function name     : read_adc
*    returns        : alt_u16 of value returned by adc
*    arg1           : channel*                     
* Created by        : Ian Johnson
* Date created      : 25/3/17
* Description       : Makes request to bot to convert analogue
*                     light sensor into binary output*                     
* Notes             : n/a                     
****************************************************************/
alt_u16 read_adc(alt_u8 channel)
{
    
    alt_u16 data, digitalvalue = 0;
    int done = FALSE;
    const int maxwait = 1000;
    int waitcount = 0;
    
    // initialise ADC
    data = channel;
    IOWR(ADC_SPI_READ_BASE, 0, data);       // specify channel
    data |= START_FLAG;                     // data OR START FLAG
    IOWR(ADC_SPI_READ_BASE, 0, data);       // tell ADC to start
    //usleep(10000);                          // wait 10ms
    
    // wait until finished
    while(!done && (waitcount++ <= maxwait))
    {
        data = IORD(ADC_SPI_READ_BASE,0);   // has the ADC finished?
        done = (data & DONE_FLAG)  ? TRUE : FALSE;
    }
    
    /* 12 bit ADC, 4 bits for control, so clear the top 4 */
    if (done)
        digitalvalue = data & 0xFFF;        

    IOWR(ADC_SPI_READ_BASE, 0, 0);          // tell ADC to stop
    return digitalvalue;                    // return the result
}

/****************************************************************
* Function name     : makeTurn
*    returns        : void                     
*    arg1           : direction - hex value to be passed to header
*    arg2           : duration - amount to turn*                     
* Created by        : Connor Parker
* Date created      : 25/03/17
* Description       : Turn towards direction specified by inputs.                    
* Notes             : Used as function as used many time, despite
*                     being short                     
****************************************************************/
void makeTurn(alt_u32 direction, int duration)
{
    IOWR_ALTERA_AVALON_PIO_DATA(EXPANSION_JP1_BASE,direction);

    usleep(duration);

    IOWR_ALTERA_AVALON_PIO_DATA(EXPANSION_JP1_BASE,0x0FFFFFFF);
}


/****************************************************************
* Function name     : calcTurn
*    returns        : void                     
*    arg1           : light_start - when light threshold first exceeded                     
*    arg2           : light_end - when light threshold goes below                     
*    arg3           : current_dir_start - direction when light threshold first exceeded                   
*    arg4           : current_dir_end - direction when light goes below threshold                    
*    arg5           : totalSteps - steps from left to right of sensor recorded during initialisation                     
* Created by        : Connor Parker
* Date created      : 25/03/17
* Description       : Calculating mid-point of light cone. 
*                     Calls makeTurn, passing values calculated                    
* Notes             : n/a                     
****************************************************************/
void calcTurn(int light_start, int light_end, int current_dir_start, int current_dir_end, alt_u32 totalSteps)
{
    /* declare variables */
    int light_total, light_half, light_middle;
    float percent; 
    
    /* if edge of cone is beyond boundry of sensor, make 90 degree turn in appropriate direction */
    if(current_dir_start != current_dir_end){
        if(current_dir_start == 1){
            makeTurn(LEFT_BOTH_MOTOR, 260000);    
        }
        else{
            makeTurn(RIGHT_BOTH_MOTOR, 260000);
        }
        current_dir_start = current_dir_end; 
    }
    /* if start and end found, calculate middle of cone */    
    else if((!(light_start == -50) && (!(light_end == -50))) == TRUE){
        if(light_start > light_end){
            light_total = light_start - light_end;
            light_half = light_total / 2;  
            light_middle = light_end + light_half;       
        }
        else{
            light_total = light_end - light_start;
            light_half = light_total / 2;
            light_middle = light_start + light_half;
        }
        
        /* allow for differences between bots */      
        percent = (((float)light_middle / (float)totalSteps) * 100);
        
        /* determines how much to turn depending on where middle of cone is */
        // hard left 
        if ((percent <= 100) && (percent >= 90))
        {
            makeTurn(LEFT_BOTH_MOTOR,260000);
        } 
        else if ((percent <= 91) && (percent >= 80))
        {
            makeTurn(LEFT_BOTH_MOTOR,180000);
        } 
        else if ((percent <= 81) && (percent >= 56))
        {
            makeTurn(LEFT_BOTH_MOTOR,100000);
        } 
        // soft left
        else if ((percent <= 55) && (percent >= 50))
        {
            makeTurn(LEFT_BOTH_MOTOR,30000);
        }
        // soft right
        else if ((percent <= 40) && (percent >= 35))
        {
            makeTurn(RIGHT_BOTH_MOTOR,30000);
        } 
        else if ((percent <= 34) && (percent >= 21))
        {
            makeTurn(RIGHT_BOTH_MOTOR,100000);
        } 
        else if ((percent <= 20) && (percent >= 11))
        {
            makeTurn(RIGHT_BOTH_MOTOR,180000);
        } 
        // hard right
        else if ((percent <= 10) && (percent >= 0))
        {
            makeTurn(RIGHT_BOTH_MOTOR,260000);
        }                       
    }
}






