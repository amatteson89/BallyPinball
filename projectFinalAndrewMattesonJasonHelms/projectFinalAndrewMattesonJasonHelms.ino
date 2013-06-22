#include <BallyLib.h>

/* 
**  -=PINBALL PROJECT=-
**  authors: Andrew Matteson and Jason Helms
**  date: today! duh!
**  last updated: June 7 2013
*/

/** TODO:
  * 3) Add ability to win balls - Harder
  * 5) put code into methods - Easy if done carefully
  * 7) woosh sound when you do the "loop" - EASY
  */

//Instantiate Bally object
Bally bally;

//denotes the index of the score array!!!
//point value (tried to be intent revealing)
#define topBumper 100
#define bottomBumper 1000
#define dropTarget 500
#define insideLane 500
#define outsideLane 1000
long fiftyKBonus =  50000;
#define centerOrTopBonus 1000
#define topPopper 3000
#define testerCheat 250

//every time a bonus is earned one of 
//these should be incremented and points
//added to appropriate player. 
int centerBonusCount = 0;
int topBonusCount = 0;

//DEFAULT for waitForTestCreditCoin
int crRow = 0;
int crCol = 5;
int coinRow = 1;
int coinCol = 0;

//Number of lamp elements 
int MAX = N_LAMP_ROWS * N_LAMP_COLS; 

//Number of players
int playerCount = 0;
int currentTurn = 0;
int bottom_bonus_counter = 0;

//drop target counter
int drop_target_counter = 0;

//Keep track of the current ball count of each player
int ballCount[] = {0,0,0,0};

//keep track of player scores
long playersScore[] = {0,0,0,0};

//Game start flag
boolean gameStarted = false;

//Used for debouncing checking
unsigned char debRowVal;

//FIXING PROBLEMs
boolean switchFlag = false;

//ReDo current play counter
long replayCounter = 0;
boolean canReplay = false;

//Top mid game field light 
int topLightBonus = 0;

//top top light stuff
int toptoplightbonus = 0;

void setup(){
    //Initialize Serial board
  Serial.begin(9600);
  
  Serial.println("======****Mata Hari Pinball****=======");
  Serial.println("The following describes the actions taken on the machine");
  Serial.println();
  Serial.println();
  Serial.println("-----SETUP-----");
  
  //Fire the top saucer in case ball is stuck in there
  //REMOVE for final version
  bally.fireSolenoid(0, true);
  Serial.println("Firing saucer solenoid");
  
   //Clear the scoreboard displays
   clearScoreBoards();
  Serial.println("Clearing scoreboards");

  //Turn off all machine lights
  turnOffLights();
  Serial.println("Turning off all machine lights");
  Serial.println(); Serial.println();
}//setup

void loop(){
    Serial.println("------MAIN LOOP------");
    
    //Initialize player count
    playerCount = 0;
    Serial.println("Initialized player count to 0");

    //Reset Player scores
    for(int i = 0; i < 4; i++)
    { playersScore[i] = 0;}
    Serial.println("Initialized all player scores to 0");
    
   //turn on game over
   bally.setLamp(12,2,1);
   Serial.println("Turned on game over light.");
  
  //Currently forces you to play with just one player
  while(playerCount < 4 ){
    if(checkSwitches() && playerCount > 1)
    {
      Serial.println("GAME IS STARTING");
        break;
    }
    
    //Waiting for players to start the game by adding credits
    static unsigned char coins = bally.getSwitch(0,5);
    if( coins != bally.getSwitch(0,5) ){
    
      addPlayer();
      Serial.println("Adding player");
      Serial.print("Current player count is: ");
      Serial.println(playerCount);
      playerAvailable();
         if(playerCount == 1)
           {
             //pushes ball from lower tray to the fire slot
             bally.fireSolenoid(6,true);
             Serial.println("Lower tray solenoid fired.");
           }//END IF
      }//end if
  }//end while
   
   /*************Do before each game**************/
   /**********************************************/
   Serial.println();
   Serial.println("=====BEFORE GAME SETUP STUFF===="); Serial.println();
   Serial.println();
   
   //Turn game over light off
   bally.setLamp(12,2,0);
   Serial.println("Game over light turned off");
   
    //Turn on flippers   (need to turn continuous solenoid off)
   bally.setContSolenoid(2, false);
   Serial.println("Flippers turned on");
   
   /**************Do before each turn*************/
   /**********************************************/
   Serial.println();
   Serial.println("=====BEFORE EACH TURN====");
   Serial.println(); Serial.println();

  while(!checkForGameOver() ){
      //Fire ball into launcher lane
      delay(200);  // THIS IS NEEDED FOR THE BALL TO KEEP coming back out 
      bally.fireSolenoid(6,true);
      switchFlag = false;
      
      drop_target_counter = 0;
      //Turn on the current players light
      //This will update every time it runs
      //Be sure this only happens once every time the players turn changes
      if(canReplay == true || replayCounter == 0){
        currentTurn++;
        if(currentTurn > playerCount)
          currentTurn = 1;
         Serial.print("Current players turn: ");
         Serial.println(currentTurn);
       
         for(int i = 1; i <= playerCount; i++){
           if(i == currentTurn ){
             bally.setLamp(14, i- 1, 1);
           }
           else{
             bally.setLamp(14, i-1, 0);
             }
         }
      
       Serial.print("Turning player ");
       Serial.print(currentTurn);
       Serial.println("'s lights on, everyone elses off");
       ballCount[currentTurn -1 ]++;
       bally.setDisplay(4,0,ballCount[currentTurn -1]);
       bottom_bonus_counter = 0;
      }
      
       //Reset the drop targets
       Serial.println("Resetting the drop targets");
       bally.fireSolenoid(3,true,true); //Left drop targets
       delay(50);
       bally.fireSolenoid(7,true,true); //right drop targets
       
       //Zero switch memory, so that any unread sticky 
       //rising edges can be ignored
       Serial.println("Zeroing switch memory");
       bally.zeroSwitchMemory();
        
       //The following while loop will be run while the ball is bouncing and trouncing around
       //It stops when the ball enters the outhole
       Serial.println("Now entering loop while ball is in play");
       delay(200);
       
       canReplay = false;
       replayCounter = 0;
       topLightBonus = 0;
       toptoplightbonus = 0;
       
       while( !bally.getDebounced(0, 7) || !switchFlag ){ //Check to see if ball is in outhole
          
          bally.setLamp(12, 0, 1);
          if( replayCounter < 40000 && canReplay == false){
             replayCounter++;  
             //Keep replay light turned on
             bally.setLamp( 10, 2, 1);
          } else if (canReplay == false){
            canReplay = true;
            //Turn off replay light
            bally.setLamp( 10, 2 ,0);
          }
          
           //top center kick out hole(saucer) 0
           if(bally.getDebounced(3,7)){
             switchFlag = true;
             //add points to current player
             if(toptoplightbonus <= 3)
               addPoints(currentTurn, (toptoplightbonus+1) * topPopper);
             else
               addPoints(currentTurn, 5 * topPopper);
             bally.playSound(24);
             bally.fireSolenoid(0,true);
             Serial.println("Solenoid (Top saucer) HIT");
           }
           
           //bottom left thumper bumper 14
           if(bally.getSwitch(4,5)){
             switchFlag = true;
             bally.fireSolenoid(14,true);
             //add points to current player
             addPoints(currentTurn, bottomBumper);
             bally.playSound(16);
             Serial.println("Solenoid bottm left thumper bumper HIT +100pts");
           }
           
           //top left thumper bumper 1
           if(bally.getSwitch(4,7)){
             switchFlag = true;
             bally.fireSolenoid(1, true);
             //add points to current player
             addPoints(currentTurn, topBumper);
             bally.playSound(16);
             Serial.println("Solenoid top left thumper bumper HIT +100pts");
           }
           
           //top right thumper bumper 9
           if(bally.getSwitch(4,6)){
             switchFlag = true;
             bally.fireSolenoid(9,true);
             //add points to current player
             addPoints(currentTurn, topBumper);
             bally.playSound(16);
             Serial.println("Solenoid top right thumper bumper HIT");
           }
           
           //bottom right thumper bumper 5
           if(bally.getSwitch(4,4)){
             switchFlag = true;
             bally.fireSolenoid(5, true);
             //add points to current player
             addPoints(currentTurn, bottomBumper);
             bally.playSound(16);
             Serial.println("Solenoid bottom right thumper bumper HIT");
           }
           
            debRowVal = bally.getDebouncedRow(2);
            unsigned char WOW = bally.getDebRedgeRow(2);
            debRowVal = debRowVal & WOW;
            unsigned char bitmask0 = 0x01 << 0;
            unsigned char bitmask1 = 0x01 << 1;
            unsigned char bitmask2 = 0x01 << 2;
            unsigned char bitmask3 = 0x01 << 3;
            unsigned char bitmask4 = 0x01 << 4;
            unsigned char bitmask5 = 0x01 << 5;
            unsigned char bitmask6 = 0x01 << 6;
            unsigned char bitmask7 = 0x01 << 7;

            if( (debRowVal &bitmask0) ){
              addPoints(currentTurn, dropTarget);
              Serial.println("DROP 0 HIT");
              bally.playSound(4);
              drop_target_counter += 1;
            }
            
            if( (debRowVal &bitmask1) ){
              addPoints(currentTurn, dropTarget);
              Serial.println("DROP 1 HIT"); 
              bally.playSound(4);
              drop_target_counter += 1;
            }
            
            if( (debRowVal &bitmask2) ){
              addPoints(currentTurn, dropTarget);
              Serial.println("DROP 2 HIT");
              bally.playSound(4);
              drop_target_counter += 1;
            }
            
            if( (debRowVal &bitmask3) ){
              addPoints(currentTurn, dropTarget);
              Serial.println("DROP 3 HIT");
              bally.playSound(4); 
              drop_target_counter += 1;
            }
            
            if( (debRowVal &bitmask4) ){
              addPoints(currentTurn, dropTarget);
              Serial.println("DROP 4 HIT");
              bally.playSound(4);
              drop_target_counter += 1;
            }
            
            if( (debRowVal &bitmask5) ){
              addPoints(currentTurn, dropTarget);
              Serial.println("DROP 5 HIT");
              bally.playSound(4); 
              drop_target_counter += 1; 
            }
            
            if( (debRowVal &bitmask6) ){
              addPoints(currentTurn, dropTarget);
              Serial.println("DROP 6 HIT");
              bally.playSound(4);
              drop_target_counter += 1;
            }
            
            if( (debRowVal &bitmask7) ){
              addPoints(currentTurn, dropTarget);
              Serial.println("DROP 7 HIT");
              bally.playSound(4);
              drop_target_counter += 1;
            }
           
           //left drop target reset 3
           if( drop_target_counter == 8){
             switchFlag = true;
             bally.fireSolenoid(3,true,true);
             bally.fireSolenoid(7,true,true);
             //fiftykKBonus => long, addPoints(int, LONG!!!!);
             addPoints(currentTurn, fiftyKBonus);
             bally.playSound(10);
             drop_target_counter = 0;
           }
           
           //right slingshot 11
           if(bally.getSwitch(4,2)){
             switchFlag = true;
             bally.fireSolenoid(11,true);
             Serial.println("Solenoid right slingshot HIT +100 pts");
             bally.playSound(8);
             addPoints(currentTurn, 10);
           }//end right sling shot
           
           //left slingshot 13
           if(bally.getSwitch(4,3)){
             switchFlag = true;
             bally.fireSolenoid(13,true);
             Serial.println("Solenoid left slingshot HIT +100 pts");
             bally.playSound(8);
             addPoints(currentTurn, 10);
           }//end left slingshot
           
           /********POINTS***********/
            unsigned char playfieldRow3 = bally.getDebouncedRow(3);
            unsigned char playfieldRow3D = bally.getDebRedgeRow(3);
            playfieldRow3 = playfieldRow3 & playfieldRow3D;
            
            if(playfieldRow3 != 0)
            {
              Serial.print("playfieldRow3 = ");
              Serial.println(playfieldRow3);
            }
            
            // col:0   RightFlipper feeder lane
            if(playfieldRow3 & 0x01){
              switchFlag = true;
              bottom_bonus_counter++;
              //                                                        turnOffLights();
              bally.playSound(16);
              addPoints(currentTurn, insideLane);
              Serial.println("RIGHT FLIPPER LANE RAN OVER +500pts");
            }
            
            // col: 2 Drop target rebound
            if( playfieldRow3 & 0x01 << 2){
              switchFlag = true;
              bally.playSound(14);
              addPoints(currentTurn, 10);
              Serial.println("DROP TARGET REBOUND HIT +10 pts"); 
            }
            
            // col:1    Left Flipper feeder lane
            if(playfieldRow3 & (0x01 << 1)){
              switchFlag = true;
              bottom_bonus_counter++;
              //                                                            turnOffLights();
              bally.playSound(16);
              addPoints(currentTurn, insideLane);
              Serial.println("LEFT FLIPPER LANE RAN OVER +500pts");
            }
            
            // col:3    Right B lane(top/middle)
             if(playfieldRow3 & (0x01 << 3)){
              switchFlag = true;
              bally.playSound(19);
              Serial.println("RIGHT B LANE RAN OVER");
              topLightBonus++;  
            }
            
            // col:4    Left A lane (top/middle)
            if(playfieldRow3 & (0x01 << 4)){
              switchFlag = true;
              bally.playSound(19);
              Serial.println("LEFT B LANE RAN OVER");
              topLightBonus++;
            }
            
            // col:5    Top B lane (top top)
            if(playfieldRow3 & (0x01 << 5)){
              switchFlag = true;
              bally.playSound(17);
              Serial.println("Top B LANE RAN OVER");
              toptoplightbonus++;
            }
            
            // col:6    Top A lane (top top)
            if(playfieldRow3 &( 0x01 << 6)){
              switchFlag = true;
              bally.playSound(17);
              Serial.println("top A LANE RAN OVER");
              toptoplightbonus++;
            }
            
            unsigned char playfieldRow4 = bally.getDebouncedRow(4);
            unsigned char playfieldRow4D = bally.getDebRedgeRow(4);
            playfieldRow4 = playfieldRow4 & playfieldRow4D;
            
            if(playfieldRow4 & (0x01)){
              switchFlag = true;
              bally.playSound(36);
              addPoints(currentTurn, outsideLane);
              Serial.println("Right out lane tagged! +1000pts");
            }
            
            if(playfieldRow4 & (0x01 << 1)){
              switchFlag = true;
              bally.playSound(36);
              addPoints(currentTurn, outsideLane);
              Serial.println("left out lane tagged! +1000pts");
            }
           
           /*************LIGHTS***********/
             if (bottom_bonus_counter <= 8)
               bally.setLamp( (bottom_bonus_counter > 4 ? 1: 0) , (bottom_bonus_counter % 4), 15);
             else if(bottom_bonus_counter > 8){
                if(bottom_bonus_counter == 9){
                   bally.setLamp( 2,0 , 15); 
                } else if(bottom_bonus_counter == 10){
                   bally.setLamp( 2,1 , 15); 
                } else if(bottom_bonus_counter > 10){
                   bally.setLamp(2, 2,15); 
                }
             }
            
             //TOP MID LIGHTS
            //1000  row 5 col 0
            //2000  row 5 col 1
            //3000  row 5 col 2
            //4000  row 5 col 3
            //5000  row 6 col 0
            //extra ball row 6 col 1
            //special row 6 col 2      
              TurnOffMidTopLights();  
             if(topLightBonus > 0 && topLightBonus < 5){
                bally.setLamp(5 , (topLightBonus -1 ) % 4, 15);
             } else if( topLightBonus == 5){
                bally.setLamp(6, 0, 15);
             } else if( topLightBonus == 6){
                bally.setLamp(6, 1, 15); 
             } else if( topLightBonus > 6){
                bally.setLamp(6,2,15);
             }
             
             //top top lights
         //2x bonus row:11 col: 3
         if(toptoplightbonus == 1)
         {bally.setLamp(9,3, 15);}
         
         //3x bonus row:11 col: 2
         if(toptoplightbonus == 2)
         {bally.setLamp(9,2,15);}
         
         //5x bonus row:11 col: 1
         if(toptoplightbonus > 2)
         {bally.setLamp(9,1,15);}
           
        delayMicroseconds(75);
       }//end while loop

       addMidTopPoints(currentTurn);
       
       if(canReplay == true){
         Serial.println("Ball is in the out hole! Turn over!");
         addPoints(currentTurn, (bottom_bonus_counter < 11 ? (bottom_bonus_counter + 1) * 1000: 20000) ); // Bottom light add points ater every ball
         for(int i = 0; i < 15; i ++) bally.setLamp( i > 4 ? 1 : 0, (i % 4), 0); 
         bally.playSound(4);
       } else{
        //canReplay == true
         Serial.println("YOU SUCK AT THIS GAME, try again"); 
         bally.playSound(36);
       }
  }
  
  Serial.println("Game Over");
  reset();
  determineWinner();
}

/*
**
**
****************<<<<<EXTRA FUNCTIONS>>>>>>**********************
**
**
*/

boolean checkForGameOver(){
 boolean result = false;
 
 Serial.println("Checking for game over");
 
  if(ballCount[playerCount - 1] == 3){
     result = true;
    Serial.println("Game is over for all players"); 
  }
 return result; 
}

void reset()
{
  clearScoreBoards();
  ballCount[0] = 0;
  ballCount[1] = 0;
  ballCount[2] = 0;
  ballCount[3] = 0;
}

void clearScoreBoards(){
  for(int displayBoard = 0; displayBoard < 4; displayBoard++){
     for(int digit = 0; digit < 8; digit++)
        bally.setDisplay( displayBoard, digit, 15);      
  }
}

void setPlayerTurn(){
  for(int i = 0; i < 4; i++){
    if( i == currentTurn)
      bally.setLamp(14, currentTurn -1, 1);
    else
      bally.setLamp(14, i, 0);
  }
}

void addMidTopPoints(int player){
    if(topLightBonus <= 5){
     //add points    topLightBonus * 1000 
     addPoints(player, topLightBonus * 1000);
     Serial.print("Player ");
     Serial.print(player);
     Serial.print(" recieved ");
     Serial.print(topLightBonus * 1000);
     Serial.println(" pts from MIDTOP ");
     } else if(topLightBonus == 6){
       //ADD EXTRA BALL 
    } else if(topLightBonus == 7){
       //ADD SPECIAL 
    }
}

//THIS DOES TOP TOP lights too
void TurnOffMidTopLights(){
   //clear top/mid lights
  for(int i = 0; i < 4; i++){
     bally.setLamp(5, i, 0); 
  }
  bally.setLamp(6,0,0);
  bally.setLamp(6,1,0);
  bally.setLamp(6,1,0); 
  bally.setLamp(9,3,0);
  bally.setLamp(9,2,0);              
  bally.setLamp(9,1,0);
}

void addPlayer(){
	if(playerCount < 4? playerCount++: Serial.println("Can't add anymore players"));

	bally.setDisplay(playerCount - 1, 0, 0);
	Serial.print("player ");
	Serial.print(playerCount);
	Serial.println(" added");
	bally.playSound(2);
}

void resetPlayers(){
	clearScoreBoards();
	playerCount = 0;
	bally.playSound(24);
}

void turnOffLights(){
  //clear all lamps
  for(int i = 0; i < N_LAMP_ROWS; i++){
    for(int j = 0; j < N_LAMP_COLS; j++){
      bally.setLamp(i, j, 0);
    }  
  }
}

void playerAvailable(){
 for(int i = 1; i <= 4; i++){
         if(i == playerCount ){
           bally.setLamp(13, i- 1, 1);
         }
         else{
           bally.setLamp(13, i-1, 0);
           }
       } 
}

void addPoints(int player, long pointsToAdd)
{
   playersScore[player-1] += pointsToAdd;
      
   //when the points change, update scoreboard
   printScore(player);
}//end addPoints

void printScore(int player)
{
   int temp =  0, i;
   boolean dummy = true;
   
   for(i = 6; i > -1; i--)
   {
       //may need to nest some if statements, testing time!!! <-lets see what happens with that
       temp = (playersScore[player - 1]/(long)pow(10, i)) % 10;
       if(temp != 0)
         dummy = false;
        
       if(i > 0 && temp == 0 && dummy){
         bally.setDisplay(player - 1, i, 15);
       }  
       else{
          bally.setDisplay(player - 1, i, temp);
       }   
   }//end for loop
   Serial.print("Player ");
   Serial.print(player);
   Serial.print(" score: ");
   Serial.println(playersScore[player -1]);
}//end printScore

void determineWinner(){
  int winner = 0;
  long temp = 0;
  for(int i = 0; i < playerCount; i++)
  {
    if(playersScore[i]>temp)
    {  
      winner = i;
      temp = playersScore[i];
    }//end if
  }//end for loop find winning score
  
  for(int i = 0; i < 4;i++)
    playersScore[i] = temp;
  
  for(int i = 0; i < 4; i++)
    printScore(i + 1);
    
  bally.fireSolenoid(10 , true);
  //give a little time for the first knock to occur, then one more time, game over!! <= just two!
  delay(25);
  bally.fireSolenoid(10 , true);
}

boolean checkSwitches()
{
  static unsigned char last2 = bally.getDebouncedRow(2);
  static unsigned char last3 = bally.getDebouncedRow(3);
  static unsigned char last4 = bally.getDebouncedRow(4);
  
  if(last2 != bally.getDebouncedRow(2))
      return true;
  if(last3 != bally.getDebouncedRow(3))
      return true;
  if(last4 != bally.getDebouncedRow(4))
      return true;
      
  return false;
}//checkSwitches
