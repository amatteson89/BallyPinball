/**************************************************
BallyLib.h
***************************************************

It should be noted that just because this library (and 
the Bally interface board) supports a certain number 
of devices, does NOT necessarily mean that a 
particular pinball machine makes use of them all.  
For example, many machines have only 6 digits per score
display, but up to 7 are supported in software.  
It is the responsibility of the user to determine the 
correspondence between switch, lamp, display, and 
solenoid numbers to actual playfield elements.  This 
can be determined from the pinball machine schematics 
or via test routines that use this library to cycle
through switches, lamps, and solenoids.  See the file
PinballTesting.pde for an example.

***************************************************
Revision History
***************************************************
0.1  Aug 2011, Paul Schimpf
0.2  Jul 2012, Paul Schimpf
               added force option to fireSolenoid()
               cleaned up some comments
               changed include of WProgram.h to Arduino.h
               added getRedgeRow() & getDebRedgeRow()
0.5  Dec 2012, Paul Schimpf
               added Coin switch to waitFor... routine
               added member function to zero out switch memory (between games)
0.7  Apr 2013  Paul Schimpf
               added setLampRow()
0.71 May 2013  David McInnis <davidm@eagles.ewu.edu
	       Fire solenoid now has options for full, Quarter, and Double
	       Quarter seems to work best on the pop bumpers
***************************************************/

#ifndef BALLYLIB_H
#define BALLYLIB_H

#include <Arduino.h>
//#include <WProgram.h>

#define N_SWITCH_ROWS  5     // playfield switch matrix is 5 x 8
#define N_SWITCH_COLS  8
#define N_CAB_SW_ROWS  2     // cabinet switch matrix is 2 x 8
#define N_CAB_SW_COLS  8
#define N_LAMP_ROWS    15    // lamp matrix is 15 x 4
#define N_LAMP_COLS    4
#define N_DISPLAYS     5     // up to 5 score displays
#define N_DIGITS       7     // each with 7 BCD digits
#define N_SOLENOIDS   15     // 15 momentary solenoids
#define N_CONT_SOL     4     // 4 continuous solenoids

#define NEXT   1
#define ENTER  2
#define TEST   4
#define CREDIT 8
#define COIN   16


typedef enum sMult
{
 S_NORMAL,
 S_QUARTER,
 S_DOUBLE
} sMult;



class Bally
{
public:
   Bally(bool installISRs=true, bool onDemandOnly=false, int switchDelay=60) ;
   
   // zeros out the switch memory images
   // useful to call this between games if you don't reinstantiate the
   // class object - so that you don't accidently process a sticky switch
   // that happened during the previous game
   void     zeroSwitchMemory() ;

   // Fire momentary solenoid for approx 30 msec
   // If another solenoid is still firing, ignore unless forcewait is true
   // and in that case, force a busy delay for previous solenoid to finish
   //
   // Downtime is S_NORMAL for default, S_QUARTER for 1/4 time and S_DOUBLE for doubl
   
   bool     fireSolenoid(int num, bool forcewait, sMult downTime=S_NORMAL);
   
   // Set the state of continuous solenoid
   bool     setContSolenoid(int num, bool val) ;

   // Set lamp at row (LS nibble of val, MS nibble is ignored)
   bool     setLampRow(int row, unsigned char val) ;
   
   // Set lamp at row, col
   bool     setLamp(int row, int col, bool val) ;
   // Get a row of lamp settings
   unsigned char getLampRow(int row) ;

   // Set a digit of a display (0 based)
   bool     setDisplay(int disp, int digit, unsigned char bcdval) ;

   // Get a particular switch state
   bool     getSwitch(int row, int col) ;
   // Get a particular debounced switch state
   bool     getDebounced(int row, int col) ;
   // Get the rising edge detection for a particular switch
   // A rising edge is reset to false once it is read
   bool     getRedge(int row, int col) ;
   // Get the rising edge detection for a particular debounced switch
   bool     getDebRedge(int row, int col) ;
   // Get the current states for an entire row of switches
   unsigned char getSwitchRow(int row) ;
   // Get the current states for an entire row of debounced switches
   unsigned char getDebouncedRow(int row) ;
   // Get the redge states for an entire row of switches
   unsigned char getRedgeRow(int row) ;
   // Get the debounced redge states for an entire row of switches
   unsigned char getDebRedgeRow(int row) ;

   // set the switch read delay to something other than the default 50 usec
   void     setSwitchDelay(int delay) ;
   // set whether to read cabinet switches only on demand
   void     setCabSwitchMonitor(bool onDemandOnly) ;

   // get the state of a particular cabinet switch (these are NOT debounced)
   bool     getCabSwitch(int row, int col) ;   

   // wait for one of the CPU board buttons to be pressed (debounced high)
   int      waitForNextEnterTest() ;
   
   // wait for one of the coin door buttons to be pressed (debounced high)
   int   waitForTestCreditCoin(int crRow, int crCol, int coinRow, int coinCol) ;
   
   // get the current state of the CPU board buttons
   int      getNextEnterTest() ;

   // play a sound clip (1-255, 0 is no sound)
   bool     playSound(unsigned char num) ;

private:
   bool     m_cabOnDemandOnly ;
} ;

// Normally PUBLIC_ISR shouldn't be #defined.  Doing so allows the
// user program to call these routines directly.  This is advisable
// only for code testing purposes, for example to measure the execution
// time of the ISR routines.  If this IS #defined, then you probably
// want to instantiate the Bally class constructor with the installISR
// argument "false" so that these routines will NOT also be installed 
// as interrupt handlers.

#ifdef PUBLIC_ISR
void zcIsr() ;
void dispIsr() ;
#endif

#endif
