#ifdef ESP32

// This file contains functions specific to the ESP32.

#include "IRremote.h"
#include "IRremoteInt.h"

// "Idiot check"
#ifdef USE_DEFAULT_ENABLE_IR_IN
#error Must undef USE_DEFAULT_ENABLE_IR_IN
#endif

hw_timer_t *timer;
void IRTimer(); // defined in IRremote.cpp, masqueraded as ISR(TIMER_INTR_NAME)

//+=============================================================================
// initialization
//
void  IRrecv::enableIRIn (bool enable)
{
	// Interrupt Service Routine - Fires every 50uS
	// #ifdef ESP32
	// ESP32 has a proper API to setup timers, no weird chip macros needed
	// simply call the readable API versions :)
	// 3 timers, choose #1, 80 divider nanosecond precision, 1 to count up

	if (enable) {

		timer = timerBegin(1, 80, 1);
		timerAttachInterrupt(timer, &IRTimer, 1);
		// every 50ns, autoreload = true
		timerAlarmWrite(timer, 50, true);
		timerAlarmEnable(timer);
}
	else {

		timerEnd(timer);
		timerDetachInterrupt(timer);
	}

	// Initialize state machine variables
	irparams.rcvstate = STATE_IDLE;
	irparams.rawlen = 0;

	// Set pin modes
	pinMode(irparams.recvpin, INPUT);
}

#endif // ESP32
