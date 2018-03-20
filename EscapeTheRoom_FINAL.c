/*******************************************************************************
 *
 *                         Connor Parker  &  Alex Lord
 *                            Marco Robot Coursework
 *
 *******************************************************************************/
/*******************************************************************************
 * Program Name         : main.c
 * Project              : Marco Robot Coursework - Room Escape 
 * Created By           : Alex Lord
 * Date Created         : 19/01/17
 * Description          : Marco Robot Coursework - Escape the Room section.
 *                        Aim is to escape a man made room. Direction that robot
 *                        will turn depends on what bumper(s) are activated.
 *                        If both are activated - it chooses a random direction
 *                        to turn. If right is activated, it turns left. If left
 *                        is activated, it turns right. It uses an even element 
 *                        of randomness and strategy to escape a room. PWM is 
 *                        also implemented to determine the speed of the robot
 *                        going forward.
 *******************************************************************************/

/* Standard Altera include files to enable the mapping of names
 To hardware addresses etc. */
#include "system.h"
#include "altera_avalon_pio_regs.h"
#include "alt_types.h"
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

// Bumpers
#define FRONT_BUMPERS 0x8800
#define FRONT_LEFT_BUMPER 0x8000
#define FRONT_RIGHT_BUMPER 0x800
// Motors
#define FORWARD 0xF
#define BACKWARD 0x3
#define ROTATE_RIGHT 0x7
#define ROTATE_LEFT 0xB

/* alt_main alias */
int main (void) __attribute__ ((weak, alias ("alt_main")));

/* function prototypes */
void forward(int x);
void rotate_dir(int direction);

/* start of main function */
int alt_main()
{
    /*
     * Varible Declarations
     * 32 bit unsigned variables to allow us to interact with the Marco hardware
     */
    alt_u32 inputs, front_bumpers;
    /* standard integer declarations */
    int random, count, dir;
    /* Turns motors and step motors on - v important - Robot won't work if this isn't included.
     * This sets the direction for bits on the expansion header. A ‘1’ means it’s writable ‘0’ readable. This is vital!
     */ 
    IOWR_ALTERA_AVALON_PIO_DIRECTION(EXPANSION_JP1_BASE, 0xF000000F);
    
    // seed for rand()
    srand(time(NULL));
    
    while(1){
        
        // Read all components of robot
        inputs = IORD_ALTERA_AVALON_PIO_DATA(EXPANSION_JP1_BASE);
        // Target specific parts of the robots
        front_bumpers = (~inputs) & FRONT_BUMPERS;
        
        if(!front_bumpers)  // Keep forward while both front bumpers not activated
            forward(6000);
        else{
            IOWR_ALTERA_AVALON_PIO_DATA(EXPANSION_JP1_BASE, BACKWARD);
            usleep(10000); // Reverse time - stops it to be able to turn
        
            switch(front_bumpers){
                /* If both front bumpers are on */
                case FRONT_BUMPERS      :   random = rand() % 2;    // randomly choose between 0/1 (to decide direction)
                    
                                            // while both front bumpers are still on, keep turning
                                            while(front_bumpers){
                                                rotate_dir(random);
                                                // Reset values for next iteration
                                                inputs = IORD_ALTERA_AVALON_PIO_DATA(EXPANSION_JP1_BASE);
                                                front_bumpers = (~inputs) & FRONT_BUMPERS;
                                            }
                                            break;
                
                /* If front left bumper is on */
                case FRONT_LEFT_BUMPER  :   // while front left bumper still on, keep rotating right
                                            while(front_bumpers){
                                                // to prevent it getting stuck in a corner / repetitive motion
                                                if(count == 5){
                                                    dir=0;      // turn left
                                                    count=0;    // both 'count=0' reset count for the next time it gets stuck
                                                }
                                                else{
                                                    dir=1;      // turn right - it will do this most of the time when the left front bumper is activated
                                                    count=0;
                                                }
                                                rotate_dir(dir);
                                                // Reset values for next iteration
                                                inputs = IORD_ALTERA_AVALON_PIO_DATA(EXPANSION_JP1_BASE);
                                                front_bumpers = (~inputs) & FRONT_BUMPERS;
                                                count++;
                                            }
                                            break;
                
                /* If front right bumper is on */
                case FRONT_RIGHT_BUMPER :   // while front right bumper still on, keep rotating left
                                            while(front_bumpers){
                                                rotate_dir(0);  // turn left
                                                // Reset values for next iteration
                                                inputs = IORD_ALTERA_AVALON_PIO_DATA(EXPANSION_JP1_BASE);
                                                front_bumpers = (~inputs) & FRONT_BUMPERS;
                                                count++;
                                            }
                                            break;
            }    
        }                
    }
}


/*******************************************************************************
 * Function Name        : forward
 *    Returns           : void / nothing
 *    Parameter         : An integer value, 'x' to determine how fast the robot
 *                        will move forward. The higher the value, the faster it
 *                        will go. 0 - 10000.
 * Created By           : Alex Lord
 * Date Created         : 02/02/17
 * Description          : PWM function to control the speed that the robot moves
 *                        forward. The higher the value that is passed through,
 *                        the faster the robot moves. 0 - 10000.
 *******************************************************************************/
 
void forward(int x){
    int y = 10000-x,i=0;
    while(x>i){
        IOWR_ALTERA_AVALON_PIO_DATA(EXPANSION_JP1_BASE, FORWARD);//go forward
        i++;
    }
    i=0;
    while(y>i){
        IOWR_ALTERA_AVALON_PIO_DATA(EXPANSION_JP1_BASE, 0xC);
        i++;
    }
    
}


/*******************************************************************************
 * Function Name        : rotate_dir
 *    Returns           : void / nothing
 *    Parameter         : Integer value from variable 'random' - either 1/2
 * Created By           : Alex Lord
 * Date Created         : 09/02/17
 * Description          : Chooses a direction depending on the value passed 
 *                        through. 1 is right, 0 is left. Used whenever either
 *                        or both of the front bumpers are activated. If both
 *                        are activated - it chooses randomly. If the right
 *                        bumper is activated - it is 0 (left). If the left
 *                        bumper is activated - it is 1 (right).
 *******************************************************************************/
    
void rotate_dir(int direction){
    if(direction == 1)
        direction = ROTATE_RIGHT;
    else
        direction = ROTATE_LEFT;
    IOWR_ALTERA_AVALON_PIO_DATA(EXPANSION_JP1_BASE, direction); // need to randomise this
    usleep(50000); // Amount of rotation - smaller for more 'finesse'
}    
