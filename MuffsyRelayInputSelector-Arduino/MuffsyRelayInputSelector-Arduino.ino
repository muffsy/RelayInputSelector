/* Muffsy Relay Input Selector
 *      
 * Control relays using IR and rotary encoder
 * Control external power to amp using IR and push button
 * http://muffsy.com
 *
 * Last update with functionality changes: 2023-09-19
 *
 * Introduction:
 * The software works on the concept of "powerState", which determines the behavior of the Input Selector:
 * 
 * 0: Boot
 *   Read last input channel from NVRAM, selecting the same input as when the input selector was turned off or lost power 
 *   The input selector is muted, and mute LED is turned off 
 *   Print startup message
 *   powerState is changed to 2 (off)
 *
   2: Power OFF
 *   Turn off all relays
 *   Set power amp to off (SSR = LOW)
 *   The input selector is muted, and mute LED is turned off
 *   Limited functionality, only IR powerbutton and rotary encoder pushbutton is active
 *   All input events, even those not available, will be shown in the serial monitor
 *        
 * 1: Power ON
 *   Turn on power button LED
 *   Set power amp to on, SSR = HIGH
 *   Activates the input channel read from NVRAM
 *   Will start muted with mute LED turned on
 *   No actions are read until the input selector unmutes after 1500 ms (default)
 *   All functions available, IR Remote and rotary encoder
 *   All input events and actions will be shown in the serial monitor
 */

/*
 * Load libraries
 */
#include <EEPROM.h> // EEPROM Library
#include <IRremote.h> // IR Remote Library
#include <Versatile_RotaryEncoder.h> // Rotary Encoder Library

/*
 * Startup delay
 *
 * When powering on, the input selector is muted to avoid any unwanted pops
 * Set the delay value in milliseconds (default 1500)
 *
 * Change to 0 for no delay
 */
#define startupDelay 1500

/*
 * Enable SSR
 *
 * If you want to use a Solid State Relay to control mains power to an
 * amplifier or similar, set this value to 1
 *
 * Default 0 (disabled), as handling mains power is dangerous
 */
 #define enableSSR 0

/* Mute and Power on / off:
 *
 * The rotary encoder is also a push button, which registers
 * either a short or a long press. These are their functions:
 *
 *    Short press: Power on / off
 *    Long press: Mute
 *
 * To reverse this behaviour, change the value below to 1 (default = 0)
 */
 #define encPush 0

/* 
 * Rotary encoder direction.
 *
 * If the rotary encoder rotation is the wrong way around, change this value to 1
 * Default: 0
 */
 #define encDirection 0

/* 
 * Rotational encoder rate
 *
 * Delay between registering rotational encoder turns, in milliseconds.
 * If the repeat rate when turning the rotational encoder is too fast, change this timer.
 *
 * Note:
 * Introducing a delay in reading turns of the rotational encoder may skip one or more
 * of the encoders "clicks". In turn, it prevents you from skipping channels if you turn
 * the encoder to fast.
 *
 * Lower number: Faster repeat rate
 * Higher number: Slower repeat rate
 * Default: 0
 */
#define rotRate 0

/* 
 * IR rate
 *
 * Delay between registering IR commands, in milliseconds.
 * If the repeat rate when holding down a button on the remote is
 * to slow or too fast, change this timer.
 *
 * Lower number: Faster repeat rate
 * Higher number: Slower repeat rate
 * Default: 150
 */
#define irRate 150

/*
 *IR Commands
 *
 * The following variables map all the buttons on the remote
 * control that comes with the Muffsy Input Selector.
 *
 * For instructions on how to configure your own remote, see:
 * https://www.muffsy.com/muffsy-relay-input-selector-4#configureremote
 *
 * While connecting your Input Selector to the computer, it will display
 * any remote commands in the Arduiono IDE's Serial Monitor like this:
 *
 *    [http://muffsy.com]: Received IR code: <IR code of button pushed>
 *
 * There are placeholder commands in the function irRemote{} for the buttons
 * 0 and 5 to 9 that serves as templates for your own IR routines
 *
 */
#define zeroButton 13       // Placeholder command in the function irRemote{}, default: 13
#define oneButton 12        // Input 1, default: 12
#define twoButton 24        // Input 2, default: 24
#define threeButton 94      // Input 3, default: 94
#define fourButton 8        // Input 4, default: 8
#define fiveButton 28       // Placeholder command in the function irRemote{}, default: 28
#define sixButton 90        // Placeholder command in the function irRemote{}, default: 90
#define sevenButton 66      // Placeholder command in the function irRemote{}, default: 66
#define eightButton 82      // Placeholder command in the function irRemote{}, default: 82
#define nineButton 74       // Placeholder command in the function irRemote{}, default: 74
#define eqButton 70         // Default value: 70
#define modeButton 68       // Default value: 68
#define muteButton 71       // Mute, default: 71
#define ffButton 21         // "Fast Forward" button: Channel up, default: 21
#define rewButton 7         // "Rewind" button: Channel down, default: 7
#define powerButton 69      // Power on / off, default: 69
#define volupButton 25      // Default value: 25
#define voldownButton 22    // Default value: 22
#define rptButton 64        // Default value: 64
#define tfuButton 67        // Default value: 67
#define playButton 9        // Default value: 9

/**********************************************************
 * Do not edit below, unless you intend to change the code!
 **********************************************************/

// Rotary encoder pins
#define clk 35  // (A3)
#define dt 34   // (A4)
#define sw 5   // (A5)

// Functions prototyping to be handled on each Encoder Event
void handleRotate(int8_t rotation);
void handlePressRelease();
void handleLongPress();

// Create a global pointer for the encoder object
Versatile_RotaryEncoder *versatile_encoder;

//  EEPROM size: 1 (relayCount)
#define EEPROM_SIZE 1
// Variables, pin definitions

// Onboard LED/Power LED
#define LED 2

// IR Receiver pin and setup
//const byte IR_Recv = 36;
#define IR_Recv 36

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

// Relay Array
volatile int relays[] = {16, 12, 27, 33, 32};

// Relay variables
volatile int relayCount;
int previousRelay;
int relayNumber;

/*
 * Power/Mute variables
 *
 * powerState explained in detail at the beginning of this program:
 *  0 = Welcome message, getting ready
 *  1 = Powered on
 *  2 = Powered off
 *
 * mute:
 *  0: Tell toggleMute() to unmute
 *  1: Tell toggleMute() to mute
 *
 * poweronMute:
 *  1: mute for the period defined by startupDelay, then unmute, only when powering on (change powerState to 1)
 *  0: Disable the startupDelay if powerState is unchanged
 */
int powerState = 0;   // Power ready-state
int mute = 1;         // Mute off/on (0/1) -> The circuit starts out muted, tell toggleMute() to stay muted at first run
int poweronMute = 1;  // Mute at poweron, with startupDelay

void setup() {

  Serial.begin(115200); // Set serial monitor transfer rate to 115,200
	versatile_encoder = new Versatile_RotaryEncoder(clk, dt, sw);

  // Load to the rotary encoder functions
  versatile_encoder->setHandleRotate(handleRotate);
  versatile_encoder->setHandlePressRelease(handlePressRelease);
  versatile_encoder->setHandleLongPress(handleLongPress);
  // Modify rotary encoder defualt values (optional)
  versatile_encoder->setReadIntervalDuration(3);    // set 3 ms as long press duration (default is 1 ms)
  // versatile_encoder->setShortPressDuration(35);  // set 35 ms as short press duration (default is 50 ms)
  // versatile_encoder->setLongPressDuration(550);  // set 550 ms as long press duration (default is 1000 ms)

  // Define INPUT pins, make sure the inputs aren't touch enabled
  pinMode (sw,INPUT_PULLUP);
  pinMode (clk,INPUT_PULLDOWN);
  pinMode (dt,INPUT_PULLDOWN);
  pinMode (IR_Recv,INPUT_PULLUP);

  // Define OUTPUT pins
  // Onboard LED
  pinMode (LED,OUTPUT);

  // Mute LED
  pinMode (muteLed,OUTPUT);

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
  previousRelay = relayCount + 1; // Start out not matching relayCount

  // Enable IR Receiver
  IrReceiver.begin(IR_Recv);
}

/******
 * Main loop
 ******/

void loop() {
  if (powerState == 0) { // Booting, welcome message
    powerOn();

  } else if (powerState == 1){ // Powered on
      relayOn();    // Will automatically change input if a function changes the relayCount variable
      irRemote();   // Allowing all functionality in the irRemote() function
      rotEncoder(); // Read the rotaryencoder 

  } else { // Powered off
      irRemote();   // irRemote() function, react to Power on/off only
      rotEncoder(); // Read the rotaryencoder, react to Power on/off only
  }

} // End Main loop

/******
 * Functions
 ******/

/*
 * Power on amplifier (Welcome/Boot message when first powered on)
 */
void powerOn() { // Only called if powerState is 0 (Ready-status)
  Serial.println("\n       --- http://muffsy.com ---\n");
  Serial.println("The Muffsy Relay Input Selector has woken up!\n");
  Serial.print(" ** Reading saved relay state from NVRAM: ");
  Serial.println(relayCount);
  digitalWrite(relays[4],LOW);

  Serial.println("\n ** Mute Relay turned ON");
  Serial.println(" ** All input relays are turned OFF");
  relayOff();

  Serial.println(" ** Solid State Relay is turned OFF\n");
  digitalWrite (SSR,LOW);
  Serial.println(" ** Startup completed - waiting for Power ON\n");
  Serial.println("       -------------------------\n");

  // Set powerState to 2 (Powered off). This function will not run again.
  powerState = 2;
} // End powerOn()

 /*
  * Turn off all relays
  */
void relayOff() {
    for (int off = 0; off <= 3; off++) {
    digitalWrite(relays[off], LOW);
  } 
} // End relayOff


 /*
  * Turn on or off input relays (mute relay is controlled by toggleMute())
  *
  * relayCount is the actual chosen active relay
  * previousRelay is the previous chosen active relay
  * This function is triggered if the two are unequal
  * The function can be forced to trigger by changing previousRelay: previousRelay = relayCount + 1;
  */
void relayOn() {
  // If relayCount has changed: Turn on the selected relay (next, previous, direct)
  // If previousRelay has changed: Turn on the last selected relay
  if (relayCount != previousRelay) {
    auto localRelayCount = relayCount; // local copy, assuming atomic integer

    /* Rollover 3 or 0, 
     *
     * There are four input relays, 0 to 3 (Status messages prints them as inputs 1 to 4)
     * This part makes sure we can't choose relays lower than 0 or higher than 3
     */
    if (localRelayCount > 3) {
      localRelayCount = 0;
    } else if (localRelayCount < 0) {
      localRelayCount = 3;
    }

    // Turn off all relays, then turn on localRelayCount
    relayOff();
    digitalWrite(relays[localRelayCount], HIGH);
    relayCount = localRelayCount;

    // Write relayCount to memory
    EEPROM.write(0,relayCount);
    EEPROM.commit();
    Serial.print("[http://muffsy.com]: Written \"relayCount = ");
    Serial.print(relayCount);
    Serial.println("\" to save slot 0");
      
    // Reset counters, output relayNumber
    previousRelay = relayCount;
    relayNumber = relayCount + 1;
    Serial.print("[http://muffsy.com]: Activated relay #");
    Serial.println(relayNumber);
    Serial.println();

    /* 
     * Mute, then unmute if just powered on
     * relayOn() is called at power on, placing the startupDelay mute here is convenient so we don't have to run a separate function
     * If the configurable value startupMute is 0, there will be no delay
     */
    if ((poweronMute == 1) && (startupDelay == 0)) {
      poweronMute = 0; // Set poweronMute to 0, so this mutedelay doesn't occur every time the circuit is muted
      mute = 0; // toggleMute will unmute
      toggleMute;

    } else if (poweronMute == 1) {
      poweronMute = 0; // Set poweronMute to 0, so this mutedelay doesn't occur every time the circuit is muted
      Serial.println("[http://muffsy.com]: Turning on mute LED");
      digitalWrite(muteLed,HIGH);
      Serial.print("[http://muffsy.com]: Milliseconds delay before turning off mute: ");
      Serial.println(startupDelay);
      delay(startupDelay);
      mute = 0; // toggleMute will unmute when called next
      toggleMute();
    }
  }
} // End relayOn()

/*
 * Mute activate/deactivate
 */
void toggleMute() {
  // This function will mute if unmuted, and vice versa everytime it's called
  if (mute == 1) { // Read mute variable. if unmuted, turn on mute
    Serial.println("[http://muffsy.com]: Mute relay turned ON");
    digitalWrite(relays[4],LOW);

    if (powerState == 1){ // If powered on, also turn on mute LED
      Serial.println("[http://muffsy.com]: Turning on mute LED\n");
      digitalWrite(muteLed,HIGH);

      // Set mute to 0, next time toggleMute is called, it will turn OFF mute
      // mute = 0 now means we're muted
      mute = 0; // Unmute next time this function is called
    } // End if powered on

  } else { // Mute must be 0, turn off mute
    Serial.println("[http://muffsy.com]: Mute relay turned OFF");
    digitalWrite(relays[4],HIGH);
    Serial.println("[http://muffsy.com]: Turning off mute LED\n");
    digitalWrite(muteLed,LOW); // Turn off mute LED, no matter if it was turned on or off earlier.

    // Set mute to 1, next time toggleMute is called, it will turn ON mute
    // mute = 1 now means we're unmuted
    mute = 1; // Mute next time this function is called
  } // 
} // End toggleMute()

/*
 * Power on / off
 */
void togglePower() {

  /*
   * If powerState is 1, turn OFF: All relays OFF, power amp OFF
   */
  if (powerState == 1) {
    powerState = 2; // Setting powerState to 2 (off)

    if (mute == 1) { // If unmuted, turn on mute 
      toggleMute(); // mute variable = 1, toggleMute will turn on mute. powerState = 2, mute LED will not turn on
    } else if (mute == 0) { // If already muted, turn off mute LED
      Serial.println("[http://muffsy.com]: Mute relay stays turned ON");
      Serial.println("[http://muffsy.com]: Turning off mute LED");
      digitalWrite(muteLed,LOW); // Turning off the mute LED
    }
    
    relayOff();
    if (enableSSR == 1) { // Only turn off SSR if "enableSSR" is set to 1
      digitalWrite (SSR,LOW); // Turning off Solid State Relay
      Serial.println("[http://muffsy.com]: Solid State Relay OFF");
    }

    Serial.println("[http://muffsy.com]: Turning off power LED");
    Serial.println("[http://muffsy.com]: Power OFF\n");
    digitalWrite (LED,LOW); // Turning off the power LED
    poweronMute = 1; // Set poweronMute to 1, this will force a mute with delay on next power on

  /*
   * If powerState is 2, turn ON: Last selected relay ON, power amp (Solid State Relay ON)
   */
  } else if (powerState == 2) { // 
    powerState = 1; // Setting powerState to 1 (on)
    Serial.println("[http://muffsy.com]: Turning on power LED");
    digitalWrite (LED,HIGH); // Turning on the power lED
    previousRelay = relayCount + 1; // Trigger relayOn()
    Serial.println("[http://muffsy.com]: Power ON");

    if (enableSSR == 1) { // Only enable SSR if "enableSSR" is set to 1
      digitalWrite (SSR,HIGH); // Turning on Solid State Relay
      Serial.println("[http://muffsy.com]: Solid State Relay ON\n");
    }

    // We have just powered on
    // The circuit is muted, manually in powerOn(), toggleMute() has not been involved to set the mute variable
    // mute = 0; tells toggleMute() that we are muted, and that toggleMute() will unmute the next time it's called
    // relayOn() will call toggleMute if poweronMute = 1
    mute = 0;
    relayOn();

  } 
} // End togglePower()

/*
 * Read the rotary encoder
 */
void rotEncoder() {
  versatile_encoder->ReadEncoder();
}

/*
 * IR Remote
 */
void irRemote() { // Start irRemote function

    // Decode the infrared input
    
  if (IrReceiver.decode()) {
    long int decCode = IrReceiver.decodedIRData.command;

    if (decCode != 0) {
      Serial.print("[http://muffsy.com]: Received IR code: ");
      Serial.println(decCode);
    }
    // Read pushed remote control button and take action
    // All actions for remote control buttons are defined here
      switch (decCode) { // Start switch/case
      
        case oneButton: // Relay 1 - Input 1
          {
            Serial.println("[http://muffsy.com]: Button \"1\"");
            if (powerState == 1) {
              relayCount = 0;
            } else {
              Serial.println("[http://muffsy.com]: Powered off, doing nothing...\n");
            }
            break;
          }

        case twoButton: // Relay 2 - Input 2
          {
            Serial.println("[http://muffsy.com]: Button \"2\"");
            if (powerState == 1) {
             relayCount = 1;
            } else {
              Serial.println("[http://muffsy.com]: Powered off, doing nothing...\n");
            }
            break;
          }

        case threeButton: // Relay 3 - Input 3
          {
            Serial.println("[http://muffsy.com]: Button \"3\"");
            if (powerState == 1) {
             relayCount = 2;
            } else {
             Serial.println("[http://muffsy.com]: Powered off, doing nothing...\n");
            }
            break;
          }

        case fourButton: // Relay 4 - Input 4
          {
            Serial.println("[http://muffsy.com]: Button \"4\"");
            if (powerState == 1) {
              relayCount = 3;
            } else {
              Serial.println("[http://muffsy.com]: Powered off, doing nothing...\n");
            }
           break;
          }
          
          case fiveButton: // Button 5 - Info message
          {
            Serial.println("[http://muffsy.com]: Button \"5\"");
            if (powerState == 1) {
              ;
            } else {
              Serial.println("[http://muffsy.com]: Powered off, doing nothing...\n");
            }
           break;
          }

          case sixButton: // Button 6 - Info message
          {
            Serial.println("[http://muffsy.com]: Button \"6\"");
            if (powerState == 1) {
              ;
            } else {
              Serial.println("[http://muffsy.com]: Powered off, doing nothing...\n");
            }
           break;
          }

          case sevenButton: // Button 7 - Info message
          {
            Serial.println("[http://muffsy.com]: Button \"7\"");
            if (powerState == 1) {
              ;
            } else {
              Serial.println("[http://muffsy.com]: Powered off, doing nothing...\n");
            }
           break;
          }

          case eightButton: // Button 8 - Info message
          {
            Serial.println("[http://muffsy.com]: Button \"8\"");
            if (powerState == 1) {
              ;
            } else {
              Serial.println("[http://muffsy.com]: Powered off, doing nothing...\n");
            }
           break;
          }

          case nineButton: // Button 9 - Info message
          {
            Serial.println("[http://muffsy.com]: Button \"9\"");
            if (powerState == 1) {
              ;
            } else {
              Serial.println("[http://muffsy.com]: Powered off, doing nothing...\n");
            }
           break;
          }

          case zeroButton: // Button 0 - Info message
          {
            Serial.println("[http://muffsy.com]: Button \"0\"");
            if (powerState == 1) {
              ;
            } else {
              Serial.println("[http://muffsy.com]: Powered off, doing nothing...\n");
            }
           break;
          }

        case muteButton: // Mute
         {
            Serial.println("[http://muffsy.com]: Button \"Mute\"");
            if (powerState == 1) {
              toggleMute();
            } else {
              Serial.println("[http://muffsy.com]: Powered off, doing nothing...\n");
            }
            break;
          }

        case ffButton: // Channel UP
          {
            Serial.println("[http://muffsy.com]: Button \"UP\"");
            if (powerState == 1) {
              relayCount++;
            } else {
              Serial.println("[http://muffsy.com]: Powered off, doing nothing...\n");
            }
            break;
          }
        
        case rewButton: // Channel DOWN
          {
            Serial.println("[http://muffsy.com]: Button \"DOWN\"");
            if (powerState == 1) {
              relayCount--;
            } else {
              Serial.println("[http://muffsy.com]: Powered off, doing nothing...\n");
            };
            break;
          }

        case powerButton: // Power button
          {
            Serial.println("[http://muffsy.com]: Button \"POWER\"");
            togglePower();
            break;
          }

          /* default:
          {
            Serial.println("[http://muffsy.com]: Going back to waiting for IR remote keypress\n");
          } */
      }// End switch/case

      unsigned long irMillis = millis();
      while (millis() - irMillis < irRate) {
        ;  
      }
  
  IrReceiver.resume(); // Receives the next value from the button you press

  } // End of if IrReceiver decode
} // End irRemote() 

/*
 * Rotate the rotary encoder
 *
 * Change the encDirection value if you want to reverse the direction
 */
void handleRotate(int8_t rotation) {
  Serial.print("[http://muffsy.com]: Rotational encoder turned ");

  // encDirection set to 0
  if ( ((rotation > 0) && (encDirection == 0)) || ((rotation < 0) && (encDirection == 1)) ) { // Channel down (counter-clockwise)

    Serial.println("counter-clockwise");

    if (powerState == 1) {
      // Increase relayCount
      relayCount--;

    } else {
      Serial.println("[http://muffsy.com]: Powered off, doing nothing...\n"); // Powered off
    }
  } else if ( ((rotation > 0) && (encDirection == 1)) || ((rotation < 0) && (encDirection == 0)) ) { // Channel up (clockwise) 

	  Serial.println("clockwise");
    
    if (powerState == 1) {
      // Increase relayCount
      relayCount++;

    } else {
      Serial.println("[http://muffsy.com]: Powered off, doing nothing...\n"); // Powered off
    } 
  }

  // Wait for the amount of milliseconds defined by rotRate until reading the next IR command
  // This delay uses interrupts, so it won't hold up the rest of the program
  unsigned long rotMillis = millis();
  while (millis() - rotMillis < rotRate) {
    ;  
  }
} // End handleRotate()


/*
 * Press rotary encoder button.
 *
 * Will turn on or off the Muffsy Input selector when the button is released,
 * or mute if you have changed the encPush value and the powerState is 1 (on)
 */
void handlePressRelease() {
  if (encPush == 0) { // Power off
  	Serial.println("[http://muffsy.com]: Button \"Power\"");
    togglePower();

  } else { // Mute
    Serial.println("[http://muffsy.com]: Button \"Mute\"");
   if (powerState == 1) {
      toggleMute();

   } else {
      Serial.println("[http://muffsy.com]: Powered off, doing nothing...\n");
    }
  }
} // End handlePressRelease()

/*
 * Long press the rotary encoder button.
 *
 * It will mute or unmute, as long as powerState is 1 (on),
 * or power on/off if you have changed the encPush value
 */
void handleLongPress() {
  if (encPush == 1) { // Power off
    Serial.println("[http://muffsy.com]: Button \"Power\"");
    togglePower();

  } else { // Mute
    Serial.println("[http://muffsy.com]: Button \"Mute\"");
   if (powerState == 1) {
      toggleMute();

   } else {
      Serial.println("[http://muffsy.com]: Powered off, doing nothing...\n");
    }
  }
} // End handleLongPress()

// END OF THE PROGRAM
