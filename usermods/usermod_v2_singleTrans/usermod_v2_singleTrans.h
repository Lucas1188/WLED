#include "wled.h"

/*
 * Usermods allow you to add own functionality to WLED more easily
 * See: https://github.com/Aircoookie/WLED/wiki/Add-own-functionality
 * 
 * This is Stairway-Wipe as a v2 usermod.
 * 
 * Using this usermod:
 * 1. Copy the usermod into the sketch folder (same folder as wled00.ino)
 * 2. Register the usermod by adding #include "stairway-wipe-usermod-v2.h" in the top and registerUsermod(new StairwayWipeUsermod()) in the bottom of usermods_list.cpp
 */
#define ON_TRIGGER = 42;
#define OFF_TRIGGER = 24;
#define REVERSED = 122;
#define NO_WIPE = 99;
#define IDLE = 1;

enum WIPE_STATES
{
    OFF,
    ON_TRANS,
    STEADY_ON,
    OFF_TRANS,
};

class SingleWipeUserMod : public Usermod {
  private:
    //Private class members. You can declare variables and functions only accessible to your usermod here
    unsigned long lastTime = 0;
    byte wipeState = 0; //0: inactive 1: wiping 2: solid
    unsigned long timeStaticStart = 0;
    uint16_t previousUserVar0 = 0;
    byte steadyStateEffect = FX_MODE_STATIC;
    byte transitionEffect = FX_MODE_COLOR_WIPE;
    byte transitionSpeed = 128;
    bool directionReversed = false;

  public:
    
    void setup() 
    {
        
    }

    void loop()
    {
        //userVar0 (U0 in HTTP API):
        //has to be set to 1 if movement is detected on the PIR that is the same side of the staircase as the ESP8266
        //has to be set to 2 if movement is detected on the PIR that is the opposite side
        //can be set to 0 if no movement is detected. Otherwise LEDs will turn off after a configurable timeout (userVar1 seconds)
        
        if(userVar1>0)
        {
            steadyStateEffect = (userVar1 & 0x00FF);
            transitionEffect = (userVar1 >> 8);
        }
        


        if (userVar0 > 0)
        {
            if ((previousUserVar0 == 1 && userVar0 == 2) || (previousUserVar0 == 2 && userVar0 == 1))
            {
                wipeState = 3; //turn off if other PIR triggered
            }
        
            previousUserVar0 = userVar0;

            if (wipeState == 0) 
            {
                startWipe();
                wipeState = 1;
            } 
            else if (wipeState == 1) 
            { //wiping
                uint32_t cycleTime = 360 + (255 - effectSpeed)*75; //this is how long one wipe takes (minus 25 ms to make sure we switch in time)
                if (millis() + strip.timebase > (cycleTime - 25)) 
                {    //wipe complete
                    endWipe();
                }
            } 
            else if (wipeState == 2) 
            { //static
                
            }
            else if (wipeState == 3)
            { //switch to wipe off
                turnOff();
            } 
            else 
            { //wiping off
                if (millis() + strip.timebase > (725 + (255 - effectSpeed)*150)) //wipe complete
                {
                    turnOff();
                }
            }
        } 
        else 
        {
            wipeState = 0; //reset for next time
            
            if (previousUserVar0) 
            {
                turnOff();   
            }
            previousUserVar0 = 0;
        }
    }

    void readFromJsonState(JsonObject& root)
    {
      userVar0 = root["user0"] | userVar0; //if "user0" key exists in JSON, update, else keep old value
      userVar1 = root["user1"] | userVar1;
      //if (root["bri"] == 255) Serial.println(F("Don't burn down your garage!"));
    }

    uint16_t getId()
    {
      return USERMOD_ID_SINGLETRANS;
    }

    void startWipe()
    {
        bri = briLast; //turn on
        jsonTransitionOnce = true;
        strip.setTransition(0); //no transition
        effectCurrent = transitionEffect;
        effectSpeed = transitionSpeed;
        strip.resetTimebase(); //make sure wipe starts from beginning

        //set wipe direction
        Segment& seg = strip.getSegment(0);
        seg.setOption(1, directionReversed);
        colorUpdated(CALL_MODE_NOTIFICATION);
    }

    void endWipe()
    {
        effectCurrent = steadyStateEffect;
        timeStaticStart = millis();
        colorUpdated(CALL_MODE_NOTIFICATION);
        wipeState = 2;
    }

    void turnOff()
    {
        jsonTransitionOnce = true;
        strip.setTransition(4000); //fade out slowly
        bri = 0;
        stateUpdated(CALL_MODE_NOTIFICATION);
        wipeState = 0;
        userVar0 = 0;
        previousUserVar0 = 0;
    }



   //More methods can be added in the future, this example will then be extended.
   //Your usermod will remain compatible as it does not need to implement all methods from the Usermod base class!
};
