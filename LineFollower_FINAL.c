/*****************************************************************
* Module name: LineFollower
*
* Copyright 1997 Company as an unpublished work.
* All Rights Reserved.
*
* The information contained herein is confidential
* property of Company. The user, copying, transfer or
* disclosure of such information is prohibited except
* by express written agreement with Company.
*
* First written on 26/01/17 by Connor Parker.
*
* Module Description:
* -------------------
* The following module will allow a MARCO robot to:
* 
*    Follow a straight line
*
*    Follow a line with angles
*
*    Follow a curved line
*
*    Move at a good speed
*
*    Move Smoothly
*
*    Stop at an obstruction
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
#define STOP             0xC
#define FOWARD           0xF 
#define LEFT_ONE_MOTOR   0xA
#define RIGHT_ONE_MOTOR  0x5
#define LEFT_BOTH_MOTOR  0xB
#define RIGHT_BOTH_MOTOR 0x7

/* Sensors */
#define LEFT_FLOOR_SENSOR  0x4000
#define RIGHT_FLOOR_SENSOR 0x2000
#define LEFT_FRONT_BUMPER  0x8000
#define RIGHT_FRONT_BUMPER 0x800

/*****************************************************************
*  Function Prototype Section
*****************************************************************/

int main (void) __attribute__ ((weak, alias ("alt_main")));

alt_u32 edgeSensor(alt_u32 *repeats);

void spiral(void);

void checkObstruction(void);

/****************************************************************/

alt_main()
{
    /* 32 bit unsigned variable to allow us to interact with 
     * the Marco hardware */
    alt_u32 output, noLineRepeats;
    
    /* This sets the direction for bits on the expansion header.
    A â€˜1â€™ means itâ€™s writable â€˜0â€™ readable. */
    IOWR_ALTERA_AVALON_PIO_DIRECTION(EXPANSION_JP1_BASE,0xF000000F);
  
    /* initialise outputs to blank */
    output = 0x0;
    /* pass initialised inputs to header */
    IOWR_ALTERA_AVALON_PIO_DATA(EXPANSION_JP1_BASE,output);
    
    noLineRepeats = 0;
    
    /* main loop */
    while(1)
    {
        
        /* call edgeSensor function to assign a value to output*/
        output = edgeSensor(&noLineRepeats);
        
        /* if no line has been detected for a time equivalent to a single rotation
         * call spiral function to spin untill line is found again */
        if (noLineRepeats == 5000)
        {

            spiral();

            noLineRepeats = 0;
            
        }
                           
        /* Apply output variable to header to control motors on, left, 
         * right, foward or backwards*/         
        IOWR_ALTERA_AVALON_PIO_DATA(EXPANSION_JP1_BASE,output);
        
        usleep(500);
        
        /* if output is not foward wait for a longer period to allow for 
         * smoother corner turning*/ 
        if(!(output==0xF))
        {
            output = STOP;
        
            IOWR_ALTERA_AVALON_PIO_DATA(EXPANSION_JP1_BASE,output);
        
            usleep(100);    
   
        }

        /* default time motor is off for smoothness control */
        else
        {
            output = STOP;
        
            IOWR_ALTERA_AVALON_PIO_DATA(EXPANSION_JP1_BASE,output);
        
            usleep(30);   
        }

        checkObstruction();
  
    }
    
}

/****************************************************************
* Function name     : edgeSensor
*    returns        : 32 bit unsigned integer to be ouput to 
*                     robot header
*    arg1           : 32 bit unsigned integer keeping count of 
*                     number of times the function has been 
*                     called and no line was detected. passed by 
*                     reference so its value is maintained
*                     between calls
* Created by        : Connor Parker
* Date created      : 02/02/17
* Description       : This is the main function to determine
*                     what direction the bot should move so as
*                     to continue following the line. It returns
*                     a value that can be passes directly into 
*                     the bot header                     
* Notes             : n/a                     
*                     
****************************************************************/
alt_u32 edgeSensor(alt_u32 *noLineRepeats)
{

    /* 32 bit unsigned variable to read value of header into*/
    alt_u32 header, direction;

    /* read value of header into header variable*/        
    header = IORD_ALTERA_AVALON_PIO_DATA(EXPANSION_JP1_BASE);
     
    /* initialise return value to stop in case of error */      
    direction = STOP;

    /* test Line Followers in the format of left right *
     *
     * Bit 14 L=1 Bit 13 R=0  */
    if (((header & LEFT_FLOOR_SENSOR) == 16384) && ((header & RIGHT_FLOOR_SENSOR) != 8192))
    {
        direction = RIGHT_BOTH_MOTOR;

        *noLineRepeats = 0;
    }

    /* Bit 14 L=1 Bit 13 R=1 */
    if (((header & LEFT_FLOOR_SENSOR) == 16384) && ((header & RIGHT_FLOOR_SENSOR) == 8192))
    {
        direction = LEFT_BOTH_MOTOR;

        (*noLineRepeats)++;
   
    }

    /* Bit 14 L=0 Bit 13 R=0 */
    if (((header & LEFT_FLOOR_SENSOR) != 16384) && ((header & RIGHT_FLOOR_SENSOR) != 8192))
    {
        direction = RIGHT_BOTH_MOTOR;

        *noLineRepeats = 0; 

    }

    /* Bit 14 L=0 Bit 13 R=1 */
    if (((header & LEFT_FLOOR_SENSOR) != 16384) && ((header & RIGHT_FLOOR_SENSOR) == 8192))
    {
        direction = FOWARD; 

        *noLineRepeats = 0;
    }

    /* shift bit pattern to right by 13 bits */
    header >>= 13;
    
    /* Output to LED bumber on board*/        
    IOWR_ALTERA_AVALON_PIO_DATA(LED_BASE,header); 

    return direction;
    
}

/****************************************************************
* Function name     : spiral
*    returns        : void*                     
*    arg1           : void 
* Created by        : COnnor Parker
* Date created      : 06/02/17
* Description       : Will cause the bot to move in a spiral shape 
*                     so as to reaquire the line                     
* Notes             : In this program the only time it is called 
*                     is after the bot has completed a full 
*                     360 degree rotation and not found the line
****************************************************************/
void spiral()
{ 

    alt_u8 noLine, repeats;

    alt_u32 output, varWait, header;
    
    /*initialise variables */
    noLine  = 1;
    repeats = 0;
    varWait = 0;

    /* loop while noLine = 0 */
    while(noLine)
    {
        /* increment repeats so that varWait is incremented 
         * every 20 loops */
        repeats ++;
        
        if (repeats == 20)
        {
            varWait ++;
            /* reinitialise repeats for next loop*/
            repeats = 0;        
        }

        /* both motors going foward for varWait which increases over time*/        
        output = 0xF;     

        IOWR_ALTERA_AVALON_PIO_DATA(EXPANSION_JP1_BASE,output);
        
        /* usleep with a variable which icreases every 20 loops */
        usleep(varWait);

        /* only left motor on  to produce circling motion*/  
        output = 0xD;     

        IOWR_ALTERA_AVALON_PIO_DATA(EXPANSION_JP1_BASE,output);
        
        usleep(500);
        
        /* both motors off for speed control*/
        output = STOP;
        
        IOWR_ALTERA_AVALON_PIO_DATA(EXPANSION_JP1_BASE,output);
        
        usleep(30); 

        /* read header to determine state of sensors */
        header = IORD_ALTERA_AVALON_PIO_DATA(EXPANSION_JP1_BASE);

        /* if either sensor has detected line exit loop */
        if (((header & LEFT_FLOOR_SENSOR) != 16384) || ((header & RIGHT_FLOOR_SENSOR) != 8192))
        {             
            noLine = 0;
        }

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


