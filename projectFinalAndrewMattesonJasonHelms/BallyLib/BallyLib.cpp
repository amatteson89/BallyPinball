/***************************************************
Revision History
***************************************************
0.1  Aug 2011, Paul Schimpf
0.2  Jul 2012, Paul Schimpf
               added force option to fireSolenoid()
               cleaned up some comments
               changed include of WProgram.h to Arduino.h
0.5  Jan 2013, Paul Schimpf
               added Coin switch to waitFor... routine
               added member function to zero out switch memory (between games)
               added doubletime for momentary solenoids
               added 50 ms delay and reset to 0 to PlaySound()   
0.6  Apr 2013, Paul Schimpf
               trial fix of playfield switches triggering Credit and Coin in
			   waitForTestCreditCoin() 
0.71 May 2013  David McInnis <davidm@eagles.ewu.edu
	       Fire solenoid now has options for full, Quarter, and Double
	       Quarter seems to work best on the pop bumpers
***************************************************/

#include "BallyLib.h"

/***************************************
some private defines
****************************************/
#define ZCPIN      2
#define ZCINUM     0
#define DISPPIN    21
#define DISPINUM   2

#define NEXTPIN    5
#define ENTERPIN   4
#define TESTPIN    3

#define DIN        PINC
#define DOUT       PORTA

#define SOLBANKSEL A14
#define SOLDATA    PORTF

// sound pins talk to 3.3V atmel, so MUST be either open circuit (input, no pullup - destination pulls
// up to 3.3V) or output low.  Thus we init PORTL to 0 and never change it.  DDRL = 1 sets a low output,
// and DDRL = 0 leaves it open circuit. 
#define SOUND      DDRL     

#define ENADISPSTRB A9
#define DISPBLANK   A15
#define DISPSTRB4   38
#define DISPSTRB   {26, 27, 28, 29, 38}
#define DISPDIGENA {39, 40, 41, 50, 51, 52, 53} 

#define LAMPSTRB    A12

#define CABSWSTRB0  A10
#define CABSWSTRB1  A11

#define DO_LAMPS_WITH_DISPS

/***************************************
private vars must be (locally) global if accessed by ISR 
routines, which cannot be class member functions. Arduino 
docs say to declare volatile, which doesn't sound quite
right, but oh well.
****************************************/
volatile unsigned char cabsw[N_CAB_SW_ROWS] ;
volatile unsigned char switches[N_SWITCH_ROWS] ;
volatile unsigned char debounced[N_SWITCH_ROWS] ;
volatile unsigned char redge[N_SWITCH_ROWS] ;
volatile unsigned char debredge[N_SWITCH_ROWS] ;
volatile unsigned char lamps[N_LAMP_ROWS] ;
volatile unsigned char displays[N_DISPLAYS][N_DIGITS] ;

volatile unsigned char sol_cnt ;
volatile unsigned char digit_num ;
volatile int  swDelay ;

// ISR uses this to determine whether to read CAB
// class object has a corresponding member var that controls when this is set
volatile bool doCAB ;    

/***************************************
* private ISR functions
****************************************/
void zcIsr()
{
   unsigned char temp, mask, newdeb ;

   // allow dispISR to interrupt
   // sei() ;
   // interrupts() ;

   // check for solenoid turnoff
   // turn off low nibble (momentary) and leave upper
   if (sol_cnt == 0)
   {
      digitalWrite(SOLBANKSEL, HIGH) ;
      SOLDATA = SOLDATA | B00001111 ;
   }
   else sol_cnt-- ;

   #ifndef DO_LAMPS_WITH_DISPS
   // update the lamps
   for (int row=0 ; row<N_LAMP_ROWS ; row++)
   {
      // lamps(row) constrained to nibble elsewhere
      // DOUT = (row<<4) | (~lamps[row] & 0x0F) ;
      DOUT = (row<<4) | (~lamps[row] & 0x0F) ;
      digitalWrite(LAMPSTRB, HIGH) ;
	  digitalWrite(LAMPSTRB, LOW) ;
      DOUT = B11111111 ;
      digitalWrite(LAMPSTRB, HIGH) ;
      digitalWrite(LAMPSTRB, LOW) ;  
   }
   #endif

   // read and debounce switches
   // note that a 50 microsecond delay is required between
   // row switches in order to allow any column with a 1 to
   // discharge the input capacitance.  Could probably skip
   // the delay on the first row read, but haven't tried.
   for (int row=0 ; row<N_SWITCH_ROWS ; row++)
   {
      // if old state (switches) and DIN are same then
      // debounced gets DIN
      // else debounce not updated and switches gets DIN
      DOUT = B00000001 << row ;
	  delayMicroseconds(swDelay) ;
	  temp = DIN ;
      mask = switches[row] ^ temp ;   // these are bouncing
	  
	  // update debounce for non-changing bits
      newdeb = ~mask & temp | mask & debounced[row] ;
	  
	  // record any rising edges
	  debredge[row] = newdeb & ~debounced[row] | debredge[row] ;
	  debounced[row] = newdeb ;
	  
	  // update nondebounced states
	  redge[row] = temp & ~switches[row] | redge[row] ;
      switches[row] = temp ;
   }
   DOUT = B00000000 ;
   
   // read cab switches if directed to do so
   if (doCAB)
   {
      digitalWrite(CABSWSTRB0, HIGH) ;
      delayMicroseconds(swDelay) ;
      cabsw[0] = DIN ;
      digitalWrite(CABSWSTRB0, LOW) ;
      digitalWrite(CABSWSTRB1, HIGH) ;
      delayMicroseconds(swDelay) ;
      cabsw[1] = DIN ;
      digitalWrite(CABSWSTRB1, LOW) ;
   }
}

void dispIsr()
{
   int dispstrb[] = DISPSTRB ;
   int dispdigena[] = DISPDIGENA ;

   #ifdef DO_LAMPS_WITH_DISPS
   // update the lamps
   for (int row=0 ; row<N_LAMP_ROWS ; row++)
   {
      DOUT = (row<<4) | (~lamps[row] & 0x0F) ;
      digitalWrite(LAMPSTRB, HIGH) ;
	  digitalWrite(LAMPSTRB, LOW) ;
      DOUT = B11111111 ;
      digitalWrite(LAMPSTRB, HIGH) ;
      digitalWrite(LAMPSTRB, LOW) ;  
   }
   // doing this only after the last read doesn't work
   //DOUT = B111111111 ;
   //digitalWrite(LAMPSTRB, HIGH) ;
   //digitalWrite(LAMPSTRB, LOW) ;  
   #endif

   // set blanking line high
   digitalWrite(DISPBLANK, HIGH) ;

   // deactivate previous digit
   digitalWrite(dispdigena[digit_num], LOW) ;

   // update digit circularly
   digit_num = (digit_num+1) % N_DIGITS ;

   // enable strobes
   digitalWrite(ENADISPSTRB, HIGH) ;

   // for each display, strobe current digit in
   for (int disp=0 ; disp<N_DISPLAYS ; disp++)
   {
      // displays entry limited to lower nibble elsewhere
      DOUT = displays[disp][digit_num] ;
      digitalWrite(dispstrb[disp], HIGH) ;
      digitalWrite(dispstrb[disp], LOW) ;
   }

   // disable strobes
   digitalWrite(ENADISPSTRB, LOW) ;

   // enable new digit
   digitalWrite(dispdigena[digit_num], HIGH) ;

   // set blanking line low
   digitalWrite(DISPBLANK, LOW) ;
}

/***************************************
* public functions
****************************************/
Bally::Bally(bool installISRs, bool onDemandOnly, int switchDelay)
{
   int digena[] = DISPDIGENA ;

   // set when to sample CAB switches
   setCabSwitchMonitor(onDemandOnly) ;
   setSwitchDelay(switchDelay) ;
   
   // set input dir and pullups on DIN, NEXT, RETURN, TEST
   // ZC and display interrupts need not be pulled up

   DDRC = B00000000 ;        // DIN is PORTC = all inputs
   // PORTC = B11111111 ;    // Do NOT attach pullups, or switches can't be read

   pinMode(NEXTPIN, INPUT) ;       
   digitalWrite(NEXTPIN, HIGH) ;   

   pinMode(ENTERPIN, INPUT) ;     
   digitalWrite(ENTERPIN, HIGH) ; 

   pinMode(TESTPIN, INPUT) ;       
   digitalWrite(TESTPIN, HIGH) ;   

   pinMode(ZCPIN, INPUT) ;
   pinMode(DISPPIN, INPUT) ;

   // configure cont solenoid controls 
   // high nibble is continous, which are active low
   // low nibble is momentary, binary coded, FF is rest posn
   digitalWrite(SOLBANKSEL, HIGH) ;
   pinMode(SOLBANKSEL, OUTPUT) ;
   DDRF = B11111111 ;                 // SOLDATA is PORTF = all output
   SOLDATA = B11111111 ;              // active low - all off

   // set other output directions

   // DOUT
   DDRA = B11111111 ;                // DOUT is PORTA = all output
   DOUT = B00000000 ;
   
   // SOUND CONTROL
   // talking to a 3.3V Atmel, so MUST be either open (input, no pullup)
   // or output low.  A 1 to SOUND (DDRL) puts an output low, but the Sound
   // Processor inverts so just use positive logic to PortL
   PORTL = B00000000 ;               // no pullups, low if output mode
   SOUND = B00000000 ;               // no sound file

   // CAB SWITCH STROBES
   digitalWrite(CABSWSTRB0, LOW) ;
   pinMode(CABSWSTRB0, OUTPUT) ;
   digitalWrite(CABSWSTRB1, LOW) ;
   pinMode(CABSWSTRB1, OUTPUT) ;
   
   // LAMP STROBE
   pinMode(LAMPSTRB, OUTPUT) ;
   digitalWrite(LAMPSTRB, LOW) ;

   // DISP CONTROLS
   digitalWrite(DISPSTRB4, LOW) ;
   pinMode(DISPSTRB4, OUTPUT) ;
   digitalWrite(ENADISPSTRB, LOW) ;
   pinMode(ENADISPSTRB, OUTPUT) ;
   digitalWrite(DISPBLANK, LOW) ;
   pinMode(DISPBLANK, OUTPUT) ;
   for (int i=0 ; i<N_DIGITS ; i++) 
   {
      digitalWrite(digena[i], LOW) ;
      pinMode(digena[i], OUTPUT) ;
   }

   // initialize memory images - shouldn't be necessary but apparently
   // is, so the loader is non-conformant to C?
   zeroSwitchMemory() ;
   
   for (int i=0 ; i<N_LAMP_ROWS ; i++)
      lamps[i] = 0x00 ;
	  
   for (int i=0 ; i<N_DISPLAYS ; i++)
      for (int j=0 ; j<N_DIGITS ; j++)
         displays[i][j] = 0x00 ;
		 
   // initialize other globals - also shouldn't be necessary
   sol_cnt = 0 ;

   // initialize display digit to last
   digit_num = N_DIGITS-1 ;

   // attach interrupts - also enables them
   if (installISRs)
   {
      attachInterrupt(ZCINUM, zcIsr, RISING) ; 
      attachInterrupt(DISPINUM, dispIsr, RISING) ;
   }
}

void Bally::zeroSwitchMemory()
{
   for (int i=0 ; i<N_SWITCH_ROWS ; i++)
   {
      switches[i] = 0x00 ;
      debounced[i] = 0x00 ;
      redge[i] = 0x00 ;
      debredge[i] = 0x00 ;
   }
   for (int i=0 ; i<N_CAB_SW_ROWS ; i++)
      cabsw[i] = 0x00 ;
}

bool Bally::fireSolenoid(int num, bool forcewait, sMult downTime)
{
   unsigned char data ;
   
   if (num >= N_SOLENOIDS) return false ;
   if (sol_cnt > 0) {
      if (forcewait)
         while (sol_cnt>0) delay(30) ;
      else
         return false ;
   }

   // dangerous to change a var that an ISR also changes
   // (sol_cnt), but here we only change it if it is 0,
   // and when it is 0 the ISR doesn't modify it.
   data = SOLDATA & B11110000 | num ;
   switch(downTime)
   {
     case S_NORMAL: //default hold 
      sol_cnt = 4;
      break;
     case S_QUARTER: //short hold
      sol_cnt = 1;
      break;
     case S_DOUBLE: //long hold
      sol_cnt = 7;
   }   
   SOLDATA = data ;
   digitalWrite(SOLBANKSEL, LOW) ;      
   return true ;
}

bool Bally::setContSolenoid(int num, bool val)
{
   unsigned char bitmask, data ;
   
   if (num >= N_CONT_SOL) return false ;
   
   bitmask = 0x01 << (num+4) ;
   data = val ? (SOLDATA | bitmask) : (SOLDATA & (~bitmask)) ;
   SOLDATA = data ;
   return true ;
}

unsigned char Bally::getLampRow(int row)
{
   if (row>=N_LAMP_ROWS) return 0 ;
   return lamps[row] ;
}

bool Bally::setLampRow(int row, unsigned char val)
{
   if (row>=N_LAMP_ROWS) return false ;
   lamps[row] = val & 0x0f ;
   return true ;
}
   
bool Bally::setLamp(int row, int col, bool val)
{
   unsigned char bitmask ;

   if (row>=N_LAMP_ROWS) return false ;
   if (col>=N_LAMP_COLS) return false ;

   bitmask = 0x01 << col ;
   lamps[row] = val ? (lamps[row] | bitmask) : (lamps[row] & (~bitmask)) ;

   return true ;   
}

bool Bally::setDisplay(int disp, int digit, unsigned char bcdval)
{
   if (disp>=N_DISPLAYS) return false ;
   if (digit>=N_DIGITS) return false ;

   displays[disp][digit] = bcdval & B00001111 ;

   return true ;
}

int Bally::waitForTestCreditCoin(int crRow, int crCol, int coinRow, int coinCol)
{
   int coin, credit, test ;

   int oldcoin = getCabSwitch(coinRow, coinCol) ;
   int oldcredit = getCabSwitch(crRow, crCol) ;
   int oldtest = digitalRead(TESTPIN) ;
   
   delay(20) ;

   int lastcoin = getCabSwitch(coinRow, coinCol) ;
   int lastcredit = getCabSwitch(crRow, crCol) ;
   int lasttest = digitalRead(TESTPIN) ;

   while(true)
   {
      delay(20) ;
      coin = getCabSwitch(coinRow, coinCol) ;
      credit = getCabSwitch(crRow, crCol) ;
      test = digitalRead(TESTPIN) ;
      if (coin && lastcoin && !oldcoin)
         return COIN ;
      if (credit && lastcredit && !oldcredit)
         return CREDIT ;
      if (test==LOW && lasttest==LOW && oldtest==HIGH)
         return TEST ;      
      oldcoin = lastcoin ;
      oldcredit = lastcredit ;
      oldtest = lasttest ;
      lastcoin = coin ;
      lastcredit = credit ;
      lasttest = test ;
   }
}

int Bally::waitForNextEnterTest()
{
   int next, enter, test ;

   int oldnext = digitalRead(NEXTPIN) ; ;
   int oldenter = digitalRead(ENTERPIN) ;
   int oldtest = digitalRead(TESTPIN) ;
   
   delay(20) ;

   int lastnext = digitalRead(NEXTPIN) ; ;
   int lastenter = digitalRead(ENTERPIN) ;
   int lasttest = digitalRead(TESTPIN) ;

   while(true)
   {
      delay(20) ;
      next = digitalRead(NEXTPIN) ;
      enter = digitalRead(ENTERPIN) ;
      test = digitalRead(TESTPIN) ;
      if (next==LOW && lastnext==LOW && oldnext==HIGH)
         return NEXT ;
      if (enter==LOW && lastenter==LOW && oldenter==HIGH)
         return ENTER ;
      if (test==LOW && lasttest==LOW && oldtest==HIGH)
         return TEST ;      
      oldnext = lastnext ;
      oldenter = lastenter ;
      oldtest = lasttest ;
      lastnext = next ;
      lastenter = enter ;
      lasttest = test ;
   }
}

int Bally::getNextEnterTest()
{
   int next = digitalRead(NEXTPIN) ; ;
   int enter = digitalRead(ENTERPIN) ;
   int test = digitalRead(TESTPIN) ;
   
   return ( (!test<<2) | (!enter<<1) | !next ) ;
}

void Bally::setCabSwitchMonitor(bool onDemandOnly)
{
   // stop ISR from monitoring CAB switches if on demand only
   if (onDemandOnly) doCAB = false ;
   else              doCAB = true ;
   m_cabOnDemandOnly = onDemandOnly ;
}

void Bally::setSwitchDelay(int delay)
{
   swDelay = delay ;
}

bool Bally::getCabSwitch(int row, int col)
{
   unsigned char bits, bitmask ;
   
   if (row>=N_CAB_SW_ROWS) return false ;
   if (col>=N_CAB_SW_COLS) return false ;

   // if on demand, wait for ISR to do it, otherwise
   // just return current state
   // Note:  haven't been able to read switches reliably from
   // outside the ISR, even if ISR is disabled during the read
   // and use delayMicroseconds instead of delay (delay doesn't
   // work with interrupts disabled).  So, just letting the ISR
   // read the switches.
   if (m_cabOnDemandOnly)
   {
      doCAB = true ;
	  delay(9) ;        // the ISR goes every 8.3 msec
	  doCAB = false ;
      /***
      digitalWrite(CABSWSTRB0, HIGH) ;
      delayMicroseconds(swDelay) ;
      cabsw[0] = DIN ;
      digitalWrite(CABSWSTRB0, LOW) ;
      digitalWrite(CABSWSTRB1, HIGH) ;
      delayMicroseconds(swDelay) ;
      cabsw[1] = DIN ;
      digitalWrite(CABSWSTRB1, LOW) ;	  
	  ***/
   }
   bits = cabsw[row] ;   
   bitmask = 0x01 << col ;
   return (bits & bitmask) ;
}
   
bool Bally::getSwitch(int row, int col)
{
   unsigned char bitmask ;

   if (row>=N_SWITCH_ROWS) return false ;
   if (col>=N_SWITCH_COLS) return false ;

   bitmask = 0x01 << col ;
   return (switches[row] & bitmask) ;
}

unsigned char Bally::getSwitchRow(int row)
{
   if (row>=N_SWITCH_ROWS) return 0 ;
   return switches[row] ;
}

bool Bally::getDebounced(int row, int col)
{
   unsigned char bitmask ;

   if (row>=N_SWITCH_ROWS) return false ;
   if (col>=N_SWITCH_COLS) return false ;

   bitmask = 0x01 << col ;
   return (debounced[row] & bitmask) ;
}

unsigned char Bally::getDebouncedRow(int row)
{
   if (row>=N_SWITCH_ROWS) return 0 ;

   return debounced[row] ;
}

bool Bally::getRedge(int row, int col)
{
   unsigned char bitmask ;
   bool val ;

   if (row>=N_SWITCH_ROWS) return false ;
   if (col>=N_SWITCH_COLS) return false ;

   bitmask = 0x01 << col ;
   val = redge[row] & bitmask ;
   
   // turn any redge event off on query
   redge[row] = redge[row] & ~bitmask ;
   return val ;
}

unsigned char Bally::getRedgeRow(int row)
{
   unsigned char val ;
   
   if (row>=N_SWITCH_ROWS) return 0 ;
   val = redge[row] ;
   // turn any redge event off on query
   redge[row] = 0x00 ;

   return val ;
}

bool Bally::getDebRedge(int row, int col)
{
   unsigned char bitmask ;
   bool val ;

   if (row>=N_SWITCH_ROWS) return false ;
   if (col>=N_SWITCH_COLS) return false ;

   bitmask = 0x01 << col ;
   val = debredge[row] & bitmask ;
   
   // turn any redge event off on query
   debredge[row] = debredge[row] & ~bitmask ;
   return val ;
}

unsigned char Bally::getDebRedgeRow(int row)
{
   unsigned char val ;
   
   if (row>=N_SWITCH_ROWS) return 0 ;
   val = debredge[row] ;
   // turn any redge event off on query
   debredge[row] = 0x00 ;
   
   return val ;
}

bool Bally::playSound(unsigned char num)
{
   SOUND = num ;
   delay(50) ;
   SOUND = 0 ;
}
