/* Includes ------------------------------------------------------------------*/
#include "timer_handles.h"
#include "key_scaner.h"
#include "led_light.h"

void Time2Handler(){
	keyScanner();
	breathLed();   /* 1ms tick drives LED breathing */
}

void Time3Handler(){
}
