/* Muffsy Relay Input Selector
 *      
 *  Control relays using IR and rotary encoder
 *  Control external power to amp using IR and push button
 *  
 */

 /*
  * powerState:
  * 
  *   0: Boot
  *     powerOn()
  *       startup procedure
  *       read NVRAM (relayCount)
  *       set relays to off (previousRelay = relayCount)
  *       set power amp to off, SSR = LOW
  *       
  *   1: Powered ON
  *       turn on power button LED
  *       set power amp to on, SSR = HIGH
  *       trigger relayOn(): previousRelay = relayCount + 1
  *     rotaryEncoder()
  *       increases or decreases relayCount depending on rotational direction
  *       pushbutton: Power ON/OFF
  *       does only Power ON if powerState == 2
  *     irRemote()
  *       input up/down
  *       input direct (buttons 1-5)
  *       power on/off
  *       does only Power ON if powerState == 2
  *     relayOn()
  *       activates relays based on the relayCount
  *       handles relayCount too high or low
  *     powerControl()
  *       read power button, set powerState accordingly
  *       
  *    2: Powered OFF
  *     turn off all relays
  *     set power amp to off (SSR = LOW)
  *     powerControl()
  *       read power button, set powerState == 1 if pushed
  *     irRemote()
  *       read power button, set powerState == 1 if pushed
  */

 // Libraries
 #include <IRremote.h> // IR Remote Library
 #include <EEPROM.h> // EEPROM Library

 //  Size: 1 (relayCount)
 #define EEPROM_SIZE 1

 // Variables, pin definitions

 // Onboard LED/Power LED
 #define LED 2

 // IR Receiver pin and setup
 #define IR_Recv 36
 IRrecv irrecv(IR_Recv);
 decode_results results;
  
 // Power button
 #define poweronButton 5

 // Pins for the rotary encoder:
 #define rotaryA 35 // DT
 #define rotaryB 34 // CLK

 // Relays
 #define R1 16
 #define R2 12
 #define R3 27
 #define R4 33
 #define R5 32

 //Solid State Relay
 #define SSR 17

 // Mute LED
 #define muteLed 4

 // Rotary Encoder variables
 int counter = 0; 
 int previous = 0;
 int aState;
 int aPreviousState;  

 // Relay Array
 int relays[] = {16, 12, 27, 33, 32};

 // Relay variables
 int relayCount;
 int previousRelay;
 int relayNumber;

 // Power/Mute variables
 int powerState;
 int buttonState = 1;       // the current reading from the input pin
 int lastButtonState = 1;   // the previous reading from the input pin
 unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
 unsigned long debounceDelay = 50;    // the debounce time; increase if the output flickers
 int mute = 0; // Mute on/off (1/0)

 // Setup
 void setup() { 

   // Power button
   pinMode (poweronButton,INPUT_PULLUP);
   
   // Onboard LED
   pinMode (LED,OUTPUT);

   // Mute LED
   pinMode (muteLed,OUTPUT);
   
   // Rotary Encoder
   pinMode (rotaryA,INPUT);
   pinMode (rotaryB,INPUT);

   // Reads the initial state of the rotaryA
   aPreviousState = digitalRead(rotaryA); 

   // Relays
   pinMode (R1,OUTPUT);
   pinMode (R2,OUTPUT);
   pinMode (R3,OUTPUT);
   pinMode (R4,OUTPUT);
   pinMode (R5,OUTPUT);
   pinMode (SSR,OUTPUT);

   // Relay variables
   EEPROM.begin(EEPROM_SIZE);
   relayCount = EEPROM.read(0);
   previousRelay = relayCount + 1; // Start out not matching relayCount???

   // Start the IR Receiver
   //pinMode(IR_Recv, INPUT_PULLDOWN);
   irrecv.enableIRIn(true); // Starts the receiver
   
   /*
    * powerStates:
      * 0: Powering on
      * 1: Powered on
      * 2: Powered off
    */
   powerState = 0;
   mute = 0; // Mute on/off (1/0)

   // Serial monitor
   Serial.begin (115200);
 } 

 /*
 * Main program
 */
 void loop() {
  if (powerState == 0) {
    powerOn();
  } else if (powerState == 1){
    relayOn();
    rotaryEncoder(); // Include Push = MUTE
    powerControl(); // Read power button
    irRemote(); // Up, Down, Direct, Volume, MUTE, Power
  } else {
    rotaryEncoder(); // Rotary push button is temporarily power button???
    powerControl(); // Read power button
    irRemote(); // Power on/off only
  }
 }

 /*
  * Turn on current relay
  */
void relayOn() {
  
  // If relayCount has changed: Turn on the selected relay (next, previous, direct)
  // If previousRelay has changed: Turn on the last selected relay
  if (relayCount != previousRelay) {

    // Rollover 3 or 0
    if (relayCount > 3) {
      relayCount = 0;
    } else if (relayCount < 0) {
      relayCount = 3;
    }

    // Turn off all relays, then turn on relayCount
    relayOff();
    digitalWrite(relays[relayCount], HIGH);

    // Stop IR, write relayCount to memory, start IR
    irrecv.enableIRIn(false);
    EEPROM.write(0,relayCount);
    EEPROM.commit();
    irrecv.enableIRIn(true);
    Serial.print("[http://muffsy.com]: Written \"relayCount = ");
    Serial.print(relayCount);
    Serial.println("\" to save slot 0");
      
   // Reset counters, output relayNumber
   previousRelay = relayCount;
   relayNumber = relayCount + 1;
   Serial.print("[http://muffsy.com]: Activated relay #");
   Serial.println(relayNumber);
   Serial.println();

   // If circuit is muted, unmute
   if (mute == 1) {
    digitalWrite(muteLed,HIGH);
    Serial.println("[http://muffsy.com]: Waiting 1.5 seconds before turning off mute");
    delay(1500);
    toggleMute();

   }
  }
}

/*
 * Power on amplifier
 */
void powerOn() { // Only called if powerState is 0 (Powering on)
    Serial.println("\n       --- http://muffsy.com ---\n");
    Serial.println("The Muffsy Relay Input Selector has woken up!\n");
    Serial.print(" ** Reading saved relay state from NVRAM: ");
    Serial.println(relayCount);
    digitalWrite(relays[4],LOW);
    mute = 1;
    Serial.println("\n ** Mute Relay turned ON");
    Serial.println(" ** All input relays are turned OFF");
    relayOff();
    Serial.println(" ** Solid State Relay is turned OFF\n");
    digitalWrite (SSR,LOW);
    Serial.println(" ** Startup completed - waiting for Power ON\n");
    Serial.println("       -------------------------\n");
    
    // Set powerState to 2 (Powered off):
    powerState = 2;
}

/*
 * Read powerbutton, turn on or off
 */
void toggleMute() {
    if (mute == 0) {
      Serial.println("[http://muffsy.com]: Mute relay turned ON\n");
      digitalWrite(relays[4],LOW);
      if (powerState == 1){
        digitalWrite(muteLed,HIGH);
      }
      mute = 1;
    } else {
      Serial.println("[http://muffsy.com]: Mute relay turned OFF\n");
      digitalWrite(relays[4],HIGH);
      digitalWrite(muteLed,LOW);
      mute = 0;
    }
}


/*
 * Read powerbutton, turn on or off
 */
void powerControl() {
    int reading = digitalRead(poweronButton);

    if (reading != lastButtonState) {
      lastDebounceTime = millis();
    }

    if ((millis() - lastDebounceTime) > debounceDelay) {
    // whatever the reading is at, it's been there for longer than the debounce
    // delay, so take it as the actual current state:

    // if the button state has changed:
    if (reading != buttonState) {
      buttonState = reading;

      // only toggle the LED if the new button state is HIGH
      if (buttonState == 1) {
        Serial.println("[http://muffsy.com]: Power button pushed");
        
        if (powerState == 1) { // Turning power OFF: All relays OFF, power amp OFF
          powerState = 2;
          digitalWrite (SSR,LOW);
          digitalWrite (LED,LOW);
          relayOff();
          if (mute == 0) {
            toggleMute();
          }
          Serial.println("[http://muffsy.com]: Solid State Relay OFF");
          Serial.println("[http://muffsy.com]: Power OFF\n");
          
        } else if (powerState == 2) { // Turning power ON: Last selected relay ON, power amp ON
          powerState = 1;
          digitalWrite (SSR,HIGH);
          digitalWrite (LED,HIGH);
          previousRelay = relayCount + 1; // Trigger relayOn()
          Serial.println("[http://muffsy.com]: Power ON");
          Serial.println("[http://muffsy.com]: Solid State Relay ON\n");
        } 
      } 
    }
  }
  lastButtonState = reading;
}

 /*
  * IR Remote
  */
void irRemote() { // Start irRemote function

    // Decode the infrared input
    
  if (irrecv.decode(&results)) {
    long int decCode = results.value;
    Serial.print("[http://muffsy.com]: Received IR code: ");
    Serial.print(decCode);
    Serial.println();
    // Switch case to use the selected remote control button
    switch (results.value) { // Start switch/case
      
      case 16724175: // Relay 1
        {
          Serial.println("[http://muffsy.com]: Button \"1\"");
          if (powerState == 1) {
            relayCount = 0;
          } else {
            Serial.println("[http://muffsy.com]: Powered off, doing nothing...\n");
          }
          break;
        }

      case 16718055: // Relay 2
        {
          Serial.println("[http://muffsy.com]: Button \"2\"");
          if (powerState == 1) {
            relayCount = 1;
          } else {
            Serial.println("[http://muffsy.com]: Powered off, doing nothing...\n");
          }
          break;
        }

      case 16743045: // Relay 3
        {
          Serial.println("[http://muffsy.com]: Button \"3\"");
          if (powerState == 1) {
            relayCount = 2;
          } else {
            Serial.println("[http://muffsy.com]: Powered off, doing nothing...\n");
          }
          break;
        }

      case 16716015: // Relay 4
        {
          Serial.println("[http://muffsy.com]: Button \"4\"");
          if (powerState == 1) {
            relayCount = 3;
          } else {
            Serial.println("[http://muffsy.com]: Powered off, doing nothing...\n");
          }
          break;
        }

      case 16769565: // Mute
        {
          Serial.println("[http://muffsy.com]: Button \"Mute\"");
          if (powerState == 1) {
            toggleMute();
          } else {
            Serial.println("[http://muffsy.com]: Powered off, doing nothing...\n");
          }
          break;
        }

      case 16754775: // Channel UP
        {
          Serial.println("[http://muffsy.com]: Button \"UP\"");
          if (powerState == 1) {
            relayCount++;
          } else {
            Serial.println("[http://muffsy.com]: Powered off, doing nothing...\n");
          }
          break;
        }

      case 16769055: // Channel DOWN
        {
          Serial.println("[http://muffsy.com]: Button \"DOWN\"");
          if (powerState == 1) {
            relayCount--;
          } else {
            Serial.println("[http://muffsy.com]: Powered off, doing nothing...\n");
          };
          break;
        }

      case 16753245: // Power button
        {
          Serial.println("[http://muffsy.com]: Button \"POWER\"");
          if (powerState == 1) {
            powerState = 2;
            digitalWrite (SSR,LOW);
            digitalWrite (LED,LOW);
            relayOff();
            if (mute == 0) {
              toggleMute();
            }
            Serial.println("[http://muffsy.com]: Solid State Relay OFF");
            Serial.println("[http://muffsy.com]: Power OFF\n");
            
          } else {
            powerState = 1;
            digitalWrite (SSR,HIGH);
            digitalWrite (LED,HIGH);
            previousRelay = relayCount + 1; // Trigger relayOn()
            Serial.println("[http://muffsy.com]: Power ON");
            Serial.println("[http://muffsy.com]: Solid State Relay ON\n");
          }
          break;
        }

      default:
      {
        Serial.println("[http://muffsy.com]: Going back to waiting for IR remote keypress\n");
      }
    } // End switch/case
    irrecv.resume(); // Receives the next value from the button you press
  }
    
  } // End irRemote function

  
 /*
  * Mute (turn off all relays)
  */
   void relayOff() {
    for (int off = 0; off <= 3; off++) {
      digitalWrite(relays[off], LOW);
      }
   }


 /*
 * Rotary Encoder Control of Relays
 */
 void rotaryEncoder() { 
   aState = digitalRead(rotaryA); // Reads the "current" state of the rotaryA
   
   // If the previous and the current state of the rotaryA are different, that means a Pulse has occured
   if (aState != aPreviousState){  

     // If the rotaryB state is different to the rotaryA state, that means the encoder is rotating clockwise
     if (digitalRead(rotaryB) != aState) { 
       counter ++;
     } else {
       counter --;
     }
    }

   // What to do if rotating Right of Left 
   if (previous != counter) {
     if (counter > 1) { // Since the encoder gives two signals when turning
       Serial.print("[http://muffsy.com]: Rotational encoder turned ");
       Serial.println("clockwise");

       if (powerState == 1) {
        // Increase relayCount
        relayCount++;
       } else {
        Serial.println("[http://muffsy.com]: Powered off, doing nothing...\n"); // Powered off???
       }
       
     } else if (counter < -1) { // Since the encoder gives two signals when turning
       Serial.print("[http://muffsy.com]: Rotational encoder turned ");
       Serial.println("counter-clockwise");
       
       if (powerState == 1) {
        // Increase relayCount
        relayCount--;
       } else {
        Serial.println("[http://muffsy.com]: Powered off, doing nothing...\n"); // Powered off???
       }
     }
   }

   // Reset counters
   previous = counter; 
   if (counter < -1) {
    counter = 0;
    previous = 0;
   } else if (counter > 1){
    counter = 0;
    previous = 0;
   }
    
   // Updates the previous state of the rotaryA with the current state
   aPreviousState = aState; 
 }
