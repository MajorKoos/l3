#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <stdio.h>
#include <avr/eeprom.h>
#include <avr/wdt.h>
#include <util/delay.h>     /* for _delay_ms() */


#include "led.h"
#include "global.h"
#include "timer.h"
#include "keymap.h"
#include "matrix.h"
#include "hwaddress.h"
#include "macro.h"
#include "Tinycmdapi.h"

#undef LED_CONTROL_MASTER
#define LED_CONTROL_SLAVE

#define PWM_OFF                 0
#define PWM_ON                  1

#define PWM_CHANNEL_0           0
#define PWM_CHANNEL_1           1

static uint8_t *const ledport[] = {LED_NUM_PORT, LED_CAP_PORT,LED_SCR_PORT};
    
static uint8_t const ledpin[] = {LED_NUM_PIN, LED_CAP_PIN, LED_SCR_PIN};
uint8_t ledmodeIndex;


#ifdef LED_CONTROL_MASTER    

uint8_t ledmode[LEDMODE_INDEX_MAX][LED_BLOCK_MAX] = {
    { 0, 0, 0, LED_EFFECT_FADING, LED_EFFECT_FADING },
    { 0, 0, 0, LED_EFFECT_PUSH_ON, LED_EFFECT_PUSH_ON },
    { 0, 0, 0, LED_EFFECT_ALWAYS, LED_EFFECT_ALWAYS },
};



static uint8_t speed[LED_BLOCK_MAX] = {0, 0, 0, 5, 5};
static uint8_t brigspeed[LED_BLOCK_MAX] = {0, 0, 0, 3, 3};
static uint8_t pwmDir[LED_BLOCK_MAX] = {0, 0, 0, 0, 0};
static uint16_t pwmCounter[LED_BLOCK_MAX] = {0, 0, 0, 0, 0};

static uint16_t pushedLevelStay[LED_BLOCK_MAX] = {0, 0, 0, 0, 0};
static uint8_t pushedLevel[LED_BLOCK_MAX] = {0, 0, 0, 0, 0};
static uint16_t pushedLevelDuty[LED_BLOCK_MAX] = {0, 0, 0, 0, 0};

extern uint16_t scankeycntms;
#endif


void led_off(LED_BLOCK block)
{
    uint8_t i;
    switch(block)
    {
        case LED_PIN_NUMLOCK:
        case LED_PIN_CAPSLOCK:
        case LED_PIN_SCROLLOCK:
            *(ledport[block]) |= BV(ledpin[block]);
            break;
        case LED_PIN_BASE:
//            tinycmd_led_level(PWM_CHANNEL_0, PWM_DUTY_MIN);
            break;
        case LED_PIN_WASD:
//            tinycmd_led_level(PWM_CHANNEL_1, PWM_DUTY_MIN);
            break;                    
        default:
            return;
    }
}

void led_on(LED_BLOCK block)
{
    uint8_t i;
    switch(block)
    {
        case LED_PIN_NUMLOCK:
        case LED_PIN_CAPSLOCK:
        case LED_PIN_SCROLLOCK: 
            *(ledport[block]) &= ~BV(ledpin[block]);
            break;
        case LED_PIN_BASE:
//            tinycmd_led_level(PWM_CHANNEL_0, PWM_DUTY_MAX);
            break;
        case LED_PIN_WASD:
//            tinycmd_led_level(PWM_CHANNEL_1, PWM_DUTY_MAX);
            break;
        default:
            return;
    }
}


void led_wave_on(LED_BLOCK block)
{
#ifdef LED_CONTROL_MASTER
    switch(block)
    {
        case LED_PIN_BASE:
            tinycmd_led_level(PWM_CHANNEL_0, PWM_DUTY_MAX-1);
            break;
        case LED_PIN_WASD:
            tinycmd_led_level(PWM_CHANNEL_1, PWM_DUTY_MAX-1);
            break;
        default:
            break;
    }
#endif // LED_CONTROL_MASTER
}

void led_wave_off(LED_BLOCK block)
{
#ifdef LED_CONTROL_MASTER
    switch(block)
    {
        case LED_PIN_BASE:
            tinycmd_led_level(PWM_CHANNEL_0, PWM_DUTY_MIN);
            break;
        case LED_PIN_WASD:
            tinycmd_led_level(PWM_CHANNEL_0, PWM_DUTY_MIN);
            break;
        default:
            break;
    }
#endif // LED_CONTROL_MASTER
}




void led_wave_set(LED_BLOCK block, uint16_t duty)
{
#ifdef LED_CONTROL_MASTER
    switch(block)
    {
        case LED_PIN_BASE:
            tinycmd_led_level(PWM_CHANNEL_0, duty);
            break;
        case LED_PIN_WASD:
            tinycmd_led_level(PWM_CHANNEL_1, duty);
            break;
       default:
            break;
    }
#endif // LED_CONTROL_MASTER
}




void led_blink(int matrixState)
{
    if(tinyExist)
    {
        static int matrixStateOld = 0;

        if(matrixStateOld != matrixState)
        {
            matrixStateOld = matrixState;
            tinycmd_dirty(matrixState & SCAN_DIRTY);
        }
    }
#ifdef LED_CONTROL_MASTER
    else
    {
        LED_BLOCK ledblock;
    
        for (ledblock = LED_PIN_BASE; ledblock <= LED_PIN_WASD; ledblock++)
        {
            
            if(matrixState & SCAN_DIRTY)      // 1 or more key is pushed
            {
                switch(ledmode[ledmodeIndex][ledblock])
                {
    
                    case LED_EFFECT_FADING_PUSH_ON:
                    case LED_EFFECT_PUSH_ON:
                        led_on(ledblock);
                        break;
                    case LED_EFFECT_PUSH_OFF:
                        led_wave_off(ledblock);
                        led_wave_set(ledblock, 0);
                        led_off(ledblock);
                        break;
                    default :
                        break;
                }             
            }else{          // none of keys is pushed
                switch(ledmode[ledmodeIndex][ledblock])
                     {
                         case LED_EFFECT_FADING_PUSH_ON:
                         case LED_EFFECT_PUSH_ON:
                            led_off(ledblock);
                            break;
                         case LED_EFFECT_PUSH_OFF:
                            led_on(ledblock);
                            break;
                         default :
                             break;
                     }
            }
        }
    }
#endif // LED_CONTROL_MASTER
}

void led_fader(void)
{
#ifdef LED_CONTROL_MASTER    
    uint8_t ledblock;
    for (ledblock = LED_PIN_BASE; ledblock <= LED_PIN_WASD; ledblock++)
    {
        if((scankeycntms > 1000)
            && (ledmode[ledmodeIndex][ledblock] == LED_EFFECT_FADING)
                || ((ledmode[ledmodeIndex][ledblock] == LED_EFFECT_FADING_PUSH_ON)))
        {
            if(pwmDir[ledblock]==0)
            {
                led_wave_set(ledblock, ((uint16_t)(pwmCounter[ledblock]/brigspeed[ledblock])));        // brighter
                if(pwmCounter[ledblock]>=255*brigspeed[ledblock])
                    pwmDir[ledblock] = 1;
                    
            }
            else if(pwmDir[ledblock]==2)
            {
                led_wave_set(ledblock, ((uint16_t)(255-pwmCounter[ledblock]/speed[ledblock])));    // darker
                if(pwmCounter[ledblock]>=255*speed[ledblock])
                    pwmDir[ledblock] = 3;

            }
            else if(pwmDir[ledblock]==1)
            {
                if(pwmCounter[ledblock]>=255*speed[ledblock])
                   {
                       pwmCounter[ledblock] = 0;
                       pwmDir[ledblock] = 2;
                   }
            }else if(pwmDir[ledblock]==3)
            {
                if(pwmCounter[ledblock]>=255*brigspeed[ledblock])
                   {
                       pwmCounter[ledblock] = 0;
                       pwmDir[ledblock] = 0;
                   }
            }


            led_wave_on(ledblock);

            // pwmDir 0~3 : idle
       
            pwmCounter[ledblock]++;

        }
        else if (ledmode[ledmodeIndex][ledblock] == LED_EFFECT_PUSHED_LEVEL)
        {
    		// 일정시간 유지

    		if(pushedLevelStay[ledblock] > 0){
    			pushedLevelStay[ledblock]--;
    		}else{
    			// 시간이 흐르면 레벨을 감소 시킨다.
    			if(pushedLevelDuty[ledblock] > 0){
    				pwmCounter[ledblock]++;
    				if(pwmCounter[ledblock] >= speed[ledblock]){
    					pwmCounter[ledblock] = 0;			
    					pushedLevelDuty[ledblock]--;
    					pushedLevel[ledblock] = PUSHED_LEVEL_MAX - (255-pushedLevelDuty[ledblock]) / (255/PUSHED_LEVEL_MAX);
    					/*if(pushedLevel_prev != pushedLevel){
    						DEBUG_PRINT(("---------------------------------decrease pushedLevel : %d, life : %d\n", pushedLevel, pushedLevelDuty));
    						pushedLevel_prev = pushedLevel;
    					}*/
    				}
    			}else{
    				pushedLevel[ledblock] = 0;
    				pwmCounter[ledblock] = 0;
    			}
    		}
    		led_wave_set(ledblock, pushedLevelDuty[ledblock]);

    	}
    	else
        {
            led_wave_set(ledblock, 0);
            led_wave_off(ledblock);
            pwmCounter[ledblock]=0;
            pwmDir[ledblock]=0;
        }
    }
#endif // LED_CONTROL_MASTER
}

void led_check(uint8_t forward)
{
    led_on(LED_PIN_NUMLOCK);
    _delay_ms(100);
    led_on(LED_PIN_SCROLLOCK);
    _delay_ms(100);
    led_on(LED_PIN_NUMLOCK);
    _delay_ms(100);
}


void led_3lockupdate(uint8_t LEDstate)
{
    if (tinyExist)
    {
        tinycmd_three_lock((LEDstate & LED_NUM), (LEDstate & LED_CAPS), (LEDstate & LED_SCROLL), FALSE);
    }
    else
    {
        if (LEDstate & LED_NUM)
        { // light up caps lock
            led_on(LED_PIN_NUMLOCK);
        }
        else
        {
            led_off(LED_PIN_NUMLOCK);
        }
        if (LEDstate & LED_CAPS)
        { // light up caps lock
            led_on(LED_PIN_CAPSLOCK);
        }
        else
        {
            led_off(LED_PIN_CAPSLOCK);
            if (LEDstate & LED_SCROLL)
            { // light up caps lock
                led_on(LED_PIN_SCROLLOCK);
            }
            else
            {
                led_off(LED_PIN_SCROLLOCK);
            }
        }
    }
}


void led_mode_init(void)
{
    LED_BLOCK ledblock;
#if 0
    int16_t i;
    uint8_t *buf;
    
    ledmodeIndex = eeprom_read_byte(EEPADDR_LEDMODE_INDEX); 
    if (ledmodeIndex >= LEDMODE_INDEX_MAX)
        ledmodeIndex = 0;
    buf = ledmode;
    for (i = 0; i < LEDMODE_ARRAY_SIZE; i++)
    {
        *buf++ = pgm_read_byte(LEDMODE_ADDRESS+i);
//        *buf++ = eeprom_read_byte(EEPADDR_LED_MODE+i);
    }
#else
    ledmodeIndex = 0;
#endif

    for (ledblock = LED_PIN_BASE; ledblock <= LED_PIN_WASD; ledblock++)
    {
#ifdef LED_CONTROL_MASTER
        pwmDir[ledblock ] = 0;
        pwmCounter[ledblock] = 0;
        led_mode_change(ledblock, ledmode[ledmodeIndex][ledblock]);
#endif // LED_CONTROL_MASTER
    }
    
    led_3lockupdate(gLEDstate);

#if 0//def LED_CONTROL_SLAVE
    tinycmd_led_config_preset((uint8_t *)&kbdConf.led_preset[0][0]);
    tinycmd_led_set_effect(kbdConf.led_preset_index);

#endif // LED_CONTROL_SLAVE
}
#ifdef LED_CONTROL_MASTER

void led_mode_change (LED_BLOCK ledblock, int mode)
{
    switch (mode)
    {
        case LED_EFFECT_FADING :
        case LED_EFFECT_FADING_PUSH_ON :
            break;
        case LED_EFFECT_PUSH_OFF :
        case LED_EFFECT_ALWAYS :
            led_on(ledblock);
            break;
        case LED_EFFECT_PUSH_ON :
        case LED_EFFECT_OFF :
        case LED_EFFECT_PUSHED_LEVEL :
        case LED_EFFECT_BASECAPS :
            led_off(ledblock);
            led_wave_set(ledblock,0);
            led_wave_on(ledblock);
            break;
        default :
            ledmode[ledmodeIndex][ledblock] = LED_EFFECT_FADING;
            break;
     }
}
#endif

#if 0
void led_mode_save(void)
{
    eeprom_write_byte(EEPADDR_LEDMODE_INDEX, ledmodeIndex);
}
#endif

#ifdef LED_CONTROL_MASTER

void led_pushed_level_cal(void)
{
    LED_BLOCK ledblock;
    // update pushed level

    for (ledblock = LED_PIN_BASE; ledblock <= LED_PIN_WASD; ledblock++)
    { 
        if(pushedLevel[ledblock] < PUSHED_LEVEL_MAX)
        {
            pushedLevelStay[ledblock] = 511;
            pushedLevel[ledblock]++;
            pushedLevelDuty[ledblock] = (255 * pushedLevel[ledblock]) / PUSHED_LEVEL_MAX;
        }
    }
}
#endif
