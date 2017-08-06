// Allocation of pins for refilling of RO and brine into...                                                    TOP SUMP:
#define STANDBYLEDTOP 15     // LED to fade on/off while not filling on pin D11 (PULLDOWN) & (current limiting resistor)
#define BRINETOPWARNLED 32   // LED to flash when brine mode engaged on pin D2  (PULLDOWN) & (current limiting resistor)
#define TOPBRINEMODEBUTTON 2 // Fluid top up toggle                  on pin D4  (PULLDOWN)
#define MOSFETROTOP 10       // RO mosfet                            on pin D6  (PULLDOWN) & (current limiting resistor)
#define MOSFETBRINETOP 11    // Brine mosfet                         on pin D7  (PULLDOWN) & (current limiting resistor)
#define LIQUIDTOPMIN 24      // Low level sensor                     on pin D15
#define LIQUIDTOPMAX 23      // High level sensor                    on pin D14

// Allocation of pins for refilling of RO and brine into...                                                 BOTTOM SUMP:
#define STANDBYLEDBOT 14     // LED to fade on/off while not filling on pin D10 (PULLDOWN) & (current limiting resistor)
#define BRINEBOTWARNLED 1    // LED to flash when brine mode engaged on pin D3  (PULLDOWN) & (current limiting resistor)
#define BOTBRINEMODEBUTTON 9 // Fluid top up mode toggle             on pin D5  (PULLDOWN)
#define MOSFETROBOT 12       // RO mosfet                            on pin D8  (PULLDOWN) & (current limiting resistor)
#define MOSFETBRINEBOT 13    // Brine mosfet                         on pin D9  (PULLDOWN) & (current limiting resistor)
#define LIQUIDBOTMIN 16      // Low level sensor                     on pin D12
#define LIQUIDBOTMAX 17      // High level sensor                    on pin D13

/*
   Initialisation of integers which define the values of sensor readings
   as well as defining a boolean variable which defines the state of the
   mosfet controlling the solenoid valve refilling each system's sump:
*/
struct systems
{
  int buttonState = LOW;
  long warnPreviousMillis = 0;
  int warnIntervalOn = 200;
  int warnIntervalOff = 800;
  bool warnLedState = false;      //warning LED is innactive... setting to true indicates warning LED is active
  long standbyPreviousMillis = 0;
  int standbyIncrementInterval = 3;
  int standbyHoldInterval = 135;
  int standbyLedState = 0;
  int standbyLastHeld = LOW;
  int liquidSenMin = 0;
  int liquidSenMax = 0;
  bool stateRO = false;           //refill innactive... setting to true indicates refill is active
  bool stateBrine = false;        //refill innactive... setting to true indicates refill is active
};
systems top, bot; //syntax: top.variablename or bot.variablename

void setup()
{
  Serial.begin(9600);             // initialise serial communications at 9600 baud

  /*
     Setup pinmodes for bot sump sensors, mosfet gates/shutoff
     switch and initialise top sump solenoids to closed
  */
  pinMode(LIQUIDTOPMIN, INPUT);
  pinMode(LIQUIDTOPMAX, INPUT);
  pinMode(MOSFETROTOP, OUTPUT);
  pinMode(MOSFETBRINETOP, OUTPUT);
  pinMode(TOPBRINEMODEBUTTON, INPUT);
  digitalWrite(MOSFETROTOP, LOW);
  digitalWrite(MOSFETBRINETOP, LOW);

  /*
     Setup pinmodes for bot sump sensors, mosfet gates/shutoff
     switch and initialise bottom sump solenoids to closed
  */
  pinMode(LIQUIDBOTMIN, INPUT);
  pinMode(LIQUIDBOTMAX, INPUT);
  pinMode(MOSFETROBOT, OUTPUT);
  pinMode(MOSFETBRINEBOT, OUTPUT);
  pinMode(BOTBRINEMODEBUTTON, INPUT);
  digitalWrite(MOSFETROBOT, LOW);
  digitalWrite(MOSFETBRINEBOT, LOW);

  /*
     Setup pinmodes and initialise pins for standby LED
     and brine mode warning LED for top sump top-up.
  */
  pinMode(STANDBYLEDTOP, OUTPUT);
  analogWrite(STANDBYLEDTOP, 0);
  pinMode(BRINETOPWARNLED, OUTPUT);
  digitalWrite(BRINETOPWARNLED, LOW);

  /*
     Setup pinmodes and initialise pins for standby LED
     and brine mode warning LED for bottom sump top-up.
  */
  pinMode(STANDBYLEDBOT, OUTPUT);
  analogWrite(STANDBYLEDBOT, 0);
  pinMode(BRINEBOTWARNLED, OUTPUT);
  digitalWrite(BRINEBOTWARNLED, LOW);
}

void loop()
{
  //Assign number of milliseconds since board last reset to variable currentMillis
  unsigned long currentMillis = millis();

  /*
     At the start of each loop, in addition to updating currentMillis,
     update variables associated with maximum and minimum level sensors, aswell as the
     states of RO/Brine mode switches for both top and bottom systems.
  */
  top.liquidSenMin = digitalRead(LIQUIDTOPMIN);
  top.liquidSenMax = digitalRead(LIQUIDTOPMAX);
  top.buttonState = digitalRead(TOPBRINEMODEBUTTON);
  bot.liquidSenMin = digitalRead(LIQUIDBOTMIN);
  bot.liquidSenMax = digitalRead(LIQUIDBOTMAX);
  bot.buttonState = digitalRead(BOTBRINEMODEBUTTON);

  /*
     If top shutoff switch is NOT engaged, use RO water to fill the sump,
     detecting when to switch on and switch off by reading the maximum and minimum
     level sensors, and checking and/or updating current mosfet/valve state...
  */
  if (top.buttonState == HIGH)
  {
    digitalWrite(BRINETOPWARNLED, LOW);                       //switches off top brine warning LED and...
    top.warnLedState = false;                                 //sets state of top brine warning LED to minimum level

    /*
      If the state of the top RO valve is active (true) and RO has filled the sump to the point
      at which the maximum liquid level sensor is located - triggering a reading of HIGH,
      make sure both top sump refill mosfets/valves are turned off, the RO refill state is reset
      to innactive (false)...
    */
    if (top.stateRO && top.liquidSenMax == HIGH)
    {
      digitalWrite(MOSFETBRINETOP, LOW);                      //makes sure top brine valve is closed
      digitalWrite(MOSFETROTOP, LOW);                         //switches top RO mosfet/valve off
      top.stateRO = false;                                    //...and sets top RO refill state
      Serial.println("Top sump filled, RO refill completed"); //confirms via serial
    }

    /*
      If the state of the top RO valve is innactive (false) and the water level in the top sump drops
      to a level at which point therefore the maximum and the minimum liquid level sensors BOTH
      return a reading of LOW...
    */
    else if (!top.stateRO && (top.liquidSenMin == LOW) && (top.liquidSenMax == LOW))
    {
      digitalWrite(STANDBYLEDTOP, LOW);
      top.standbyLastHeld = LOW;
      digitalWrite(MOSFETBRINETOP, LOW);                      //makes sure top brine valve is closed
      digitalWrite(MOSFETROTOP, HIGH);                        //switches top RO mosfet/valve on
      top.stateRO = true;                                     //...and sets top RO refill state
      Serial.println("Top sump refilling with RO water");     //confirms via serial
    }
  }

  /*
     Otherwise, if top shutoff switch IS engaged, flash top brine mode warning LED and
     use brine water to fill the sump, detecting when to switch on and switch
     off by reading the maximum and minimum level sensors, and checking and/or updating
     the current mosfet and therefore solenoid valve state...
  */
  else if (top.buttonState == LOW)
  {

    /*
       Flash top brine mode warning LED on:
       If the state of the top sump brine mode LED is off, check to see if the current
       time since boot minus the last time since boot (that was set as top.warnPreviousMillis)
       is greater than the variable determining the off duration for top brine warning LED...
    */
    if (!top.warnLedState && (currentMillis - top.warnPreviousMillis > top.warnIntervalOff))
    {
      top.warnPreviousMillis = currentMillis; //set value of top.warnPreviousMillis to the current time since boot
      digitalWrite(BRINETOPWARNLED, HIGH);    //turn on top brine mode warning LED
      top.warnLedState = true;                //set the top brine mode warning LED state to active
    }

    /*
       Flash top brine mode warning LED off:
       If the state of the top sump brine mode LED is on, check to see if the current
       time since boot minus the last time since boot (that was set as top.warnPreviousMillis)
       is greater than the variable determining the on duration for top brine warning LED...
    */
    else if (top.warnLedState == true && (currentMillis - top.warnPreviousMillis > top.warnIntervalOn))
    {
      top.warnPreviousMillis = currentMillis; //set value of top.warnPreviousMillis to the current time since boot
      digitalWrite(BRINETOPWARNLED, LOW);     //turn off top brine mode warning LED
      top.warnLedState = false;               //set the top brine mode warning LED state to innactive
    }

    /*
      If the state of the top brine valve is active (true) and brine has filled the sump to the point
      at which the maximum liquid level sensor is located - triggering a reading of HIGH,
      make sure both top sump refill mosfets/valves are turned off, the brine refill state is reset
      to innactive (false)...
    */
    if (top.stateBrine && top.liquidSenMax == HIGH)
    {
      digitalWrite(MOSFETROTOP, LOW);                             //makes sure top RO valve is closed
      digitalWrite(MOSFETBRINETOP, LOW);                          //switches top brine mosfet/valve off
      top.stateBrine = false;                                     //...and sets top brine refill state
      Serial.println("Top sump filled, brine refill completed."); //confirms via serial
    }

    /*
      If the state of the top brine valve is innactive (false) and the water level in the top sump drops
      to a level at which point therefore the maximum and the minimum liquid level sensors BOTH
      return a reading of LOW...
    */
    else if (!top.stateBrine && (top.liquidSenMin == LOW) && (top.liquidSenMax == LOW))
    {
      digitalWrite(STANDBYLEDTOP, LOW);
      top.standbyLastHeld = LOW;
      digitalWrite(MOSFETROTOP, LOW);                             //makes sure top RO valve is closed
      digitalWrite(MOSFETBRINETOP, HIGH);                         //switches top brine mosfet/valve on
      top.stateBrine = true;                                      //...and sets top RO refill state
      Serial.println("Top sump refilling with brine water.");     //confirms via serial
    }
  }

  /*
     If bottom shutoff switch is NOT engaged, use RO water to fill the sump,
     detecting when to switch on and switch off by reading the maximum and minimum
     level sensors, and checking and/or updating current mosfet/valve state.
  */
  if (bot.buttonState == HIGH)
  {
    digitalWrite(BRINEBOTWARNLED, LOW);      //switches off bottom brine warning LED and...
    bot.warnLedState = LOW;                  //sets state of bottom brine warning LED to minimum level

    /*
      If the state of the bottom RO valve is active (true) and RO has filled the sump to the point
      at which the maximum liquid level sensor is located - triggering a reading of HIGH,
      make sure both bottom sump refill mosfets/valves are turned off, the RO refill state is reset
      to innactive (false)...
    */
    if (bot.stateRO && bot.liquidSenMax == HIGH)
    {
      digitalWrite(MOSFETBRINETOP, LOW);                         //makes sure bottom brine valve is closed
      digitalWrite(MOSFETROBOT, LOW);                            //switches bottom RO mosfet/valve off
      bot.stateRO = false;                                       //...and sets bottom RO refill state
      Serial.println("Bottom sump filled, RO refill finished."); //confirms via serial
    }

    /*
      If the state of the bottom RO valve is innactive (false) and the water level in the bottom sump drops
      to a level at which point therefore the maximum and the minimum liquid level sensors BOTH
      return a reading of LOW...
    */
    else if (!bot.stateRO && (bot.liquidSenMin == LOW) && (bot.liquidSenMax == LOW))
    {
      digitalWrite(STANDBYLEDBOT, LOW);
      bot.standbyLastHeld = LOW;
      digitalWrite(MOSFETBRINETOP, LOW);                         //makes sure bottom brine valve is closed
      digitalWrite(MOSFETROBOT, HIGH);                           //switches bottom RO mosfet/valve on
      bot.stateRO = true;                                        //...and sets bottom RO refill state
      Serial.println("Bottom sump refilling with RO water.");    //confirms via serial
    }
  }

  /*
     Otherwise, if bottom shutoff switch IS engaged, flash bottom brine mode warning LED and
     use brine water to fill the sump, detecting when to switch on and switch
     off by reading the maximum and minimum level sensors, and checking and/or updating
     the current mosfet and therefore solenoid valve state...
  */
  else if (bot.buttonState == LOW)
  {

    /*
       Flash bottom brine mode warning LED on:
       If the state of the bottom sump brine mode LED is off, check to see if the current
       time since boot minus the last time since boot (that was set as bot.warnPreviousMillis)
       is greater than the variable determining the off duration for bottom brine warning LED...
    */
    if (bot.warnLedState == false && (currentMillis - bot.warnPreviousMillis > bot.warnIntervalOff))
    {
      bot.warnPreviousMillis = currentMillis; //set value of bot.warnPreviousMillis to the current time since boot
      digitalWrite(BRINEBOTWARNLED, HIGH);    //turn on bottom brine mode warning LED
      bot.warnLedState = true;                //set the bottom brine mode warning LED state to active
    }

    /*
      If the state of the bottom brine valve is active (true) and brine has filled the sump to the point
      at which the maximum liquid level sensor is located - triggering a reading of HIGH,
      make sure both bottom sump refill mosfets/valves are turned off, the brine refill state is reset
      to innactive (false)...
    */
    else if (bot.warnLedState == true && (currentMillis - bot.warnPreviousMillis > bot.warnIntervalOn))
    {
      bot.warnPreviousMillis = currentMillis; //set value of bot.warnPreviousMillis to the current time since boot
      digitalWrite(BRINEBOTWARNLED, LOW);     //turn off bottom brine mode warning LED
      bot.warnLedState = false;               //set the bottom brine mode warning LED state to innactive
    }

    /*
      If the state of the bottom brine valve is active (true) and brine has filled the sump to the point
      at which the maximum liquid level sensor is located - triggering a reading of HIGH,
      make sure both bottom sump refill mosfets/valves are turned off, the brine refill state is reset
      to innactive (false)...
    */
    if (bot.stateBrine && bot.liquidSenMax == HIGH)
    {
      digitalWrite(MOSFETROBOT, LOW);                               //makes sure top RO valve is closed
      digitalWrite(MOSFETBRINEBOT, LOW);                            //switches top brine mosfet/valve off
      bot.stateBrine = false;                                       //...and sets bottom brine refill state off
      Serial.println("Bottom sump filled, brine refill finished."); //confirms via serial
    }

    /*
      If the state of the bottom brine valve is innactive (false) and the water level in the bottom sump drops
      to a level at which point therefore the maximum and the minimum liquid level sensors BOTH
      return a reading of LOW...
    */
    else if (!bot.stateBrine && (bot.liquidSenMin == LOW) && (bot.liquidSenMax == LOW))
    {
      digitalWrite(STANDBYLEDBOT, LOW);
      bot.standbyLastHeld = LOW;
      digitalWrite(MOSFETROBOT, LOW);                               //makes sure bottom RO valve is closed
      digitalWrite(MOSFETBRINEBOT, HIGH);                           //switches bottom brine mosfet/valve on
      bot.stateBrine = true;                                        //...and sets bottom brine refill state on
      Serial.println("Bottom sump refilling with brine water.");    //confirms via serial
    }
  }

  /*
    If neither of the sumps have either of their RO or brine mosfet/valve states set to true,
    fade on and off both top and bottom standby LEDs.
  */
  if (top.stateRO == false && top.stateBrine == false
      && bot.stateRO == false && bot.stateBrine == false)
  {
    
   /*
     For i, starting off at 0 (0V), continuing up to 255 (5V) and increasing by 1 at the start of each iteration,
     set the voltage of the standby LED pins for both the top and bottom sumps increasing by 5V/256 (0.0196V).
    */
    for (int i = 0; i <= 255; i++)
    {
      if (top.standbyLastHeld == HIGH && (currentMillis - top.standbyPreviousMillis > top.standbyIncrementInterval))
      {
        top.standbyPreviousMillis = currentMillis;
        analogWrite(STANDBYLEDTOP, i);
        analogWrite(STANDBYLEDBOT, i);
      }
      top.standbyLedState = i;
    }
    
    if (top.standbyLedState == 255 && (currentMillis - top.standbyPreviousMillis > top.standbyHoldInterval))
      {
        top.standbyPreviousMillis = currentMillis;
        top.standbyLastHeld = HIGH;
      }

   /*
     For i, starting off at 255 (5V), continuing down to 0 (0V) and decreasing by 1 at the start of each iteration,
     set the voltage of the standby LED pins for both top and bottom sumps, decreasing by 5V/256 (0.0196V).
    */
    for (int i = 255; i >= 0; i--)
    {
      if (top.standbyLastHeld == HIGH && (currentMillis - top.standbyPreviousMillis > top.standbyIncrementInterval))
      {
        top.standbyPreviousMillis = currentMillis;
        analogWrite(STANDBYLEDTOP, i);
        analogWrite(STANDBYLEDBOT, i);
      }
      top.standbyLedState = i;
    }
    
    if (top.standbyLedState == 0 && (currentMillis - top.standbyPreviousMillis > top.standbyHoldInterval))
      {
        top.standbyPreviousMillis = currentMillis;
        top.standbyLastHeld = LOW;
    }
  }

  /*
    If neither of the top sump's mosfet/valve states but either of the bottom sump's
    mosfet/valve states is set to true, fade on and off the top but not the bottom standby LEDS.
  */
  else if (top.stateRO == false && top.stateBrine == false
           && (bot.stateRO == true || bot.stateBrine == true))
  {
    
   /*
     For i, starting off at 0 (0V), continuing up to 255 (5V) and increasing by 1 at the start of each iteration,
     set the voltage of the standby LED pins for the top sump increasing by 5V/256 (0.0196V).
    */
    for (int i = 0; i < 255; i++)
    {
      if (top.standbyLastHeld == LOW && (currentMillis - top.standbyPreviousMillis > top.standbyIncrementInterval))
      {
        top.standbyPreviousMillis = currentMillis;
        analogWrite(STANDBYLEDTOP, i);
      }
      top.standbyLedState = i;
    }
    
    if (top.standbyLedState == 255 && (currentMillis - top.standbyPreviousMillis > top.standbyHoldInterval))
      {
        top.standbyPreviousMillis = currentMillis;
        top.standbyLastHeld = HIGH;
      }
  }

   /*
     For i, starting off at 255 (5V), continuing down to 0 (0V) and decreasing by 1 at the start of each iteration,
     set the voltage of the standby LED pins for top sump, decreasing by 5V/256 (0.0196V).
    */
    for (int i = 255; i > 0; i--)
    {
      if (top.standbyLastHeld == HIGH && (currentMillis - top.standbyPreviousMillis > top.standbyIncrementInterval))
      {
        top.standbyPreviousMillis = currentMillis;
        analogWrite(STANDBYLEDTOP, i);
      }
      top.standbyLedState = i;
    }
    
    if (top.standbyLedState == 0 && (currentMillis - top.standbyPreviousMillis > top.standbyHoldInterval))
      {
        top.standbyPreviousMillis = currentMillis;
        top.standbyLastHeld = LOW;
      }

  /*
    If either of the top sump's mosfet/valve states but neither of the bottom sump's
    mosfet/valve states is set to true, fade on and off the bottom but not the top standby LEDS.
  */
  else if (top.stateRO == true || top.stateBrine == true
           && (bot.stateRO == false && bot.stateBrine == false))
  {
    
   /*
     For i, starting off at 0 (0V), continuing up to 255 (5V) and increasing by 1 at the start of each iteration,
     set the voltage of the standby LED pins for the bottom sump increasing by 5V/256 (0.0196V).
    */    
    for (int i = 0; i < 255; i++)
    {
      if (bot.standbyLastHeld == LOW && (currentMillis - bot.standbyPreviousMillis > bot.standbyIncrementInterval))
      {
        bot.standbyPreviousMillis = currentMillis;
        analogWrite(STANDBYLEDBOT, i);
      }
      bot.standbyLedState = i;
    }
    
    if (bot.standbyLedState == 255 && (currentMillis - bot.standbyPreviousMillis > bot.standbyHoldInterval))
      {
        bot.standbyPreviousMillis = currentMillis;
        bot.standbyLastHeld = HIGH;
      }
  }

   /*
     For i, starting off at 255 (5V), continuing down to 0 (0V) and decreasing by 1 at the start of each iteration,
     set the voltage of the standby LED pins for the bottom sump, decreasing by 5V/256 (0.0196V).
    */
    for (int i = 255; i > 0; i--)
    {
      if (bot.standbyLastHeld == HIGH && (currentMillis - bot.standbyPreviousMillis > bot.standbyIncrementInterval))
      {
        bot.standbyPreviousMillis = currentMillis;
        analogWrite(STANDBYLEDBOT, i);
      }
      bot.standbyLedState = i;
    }
    
    if (bot.standbyLedState == 0 && (currentMillis - bot.standbyPreviousMillis > bot.standbyHoldInterval))
      {
        bot.standbyPreviousMillis = currentMillis;
        bot.standbyLastHeld = LOW;
      }
}
