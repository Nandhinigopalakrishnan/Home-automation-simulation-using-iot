/*************************************************************
Title         :   Home automation using blynk
Description   :   To control light's brigntness with brightness,monitor temperature , monitor water level in the tank through blynk app
Pheripherals  :   Arduino UNO , Temperature system, LED, LDR module, Serial Tank, Blynk cloud, Blynk App.
 *************************************************************/

// Template ID, Device Name and Auth Token are provided by the Blynk.Cloud
// See the Device Info tab, or Template settings
#define BLYNK_TEMPLATE_ID "T****XG"
#define BLYNK_DEVICE_NAME "Home automation"
#define BLYNK_AUTH_TOKEN "eRV6********************"

// Comment this out to disable prints 
//#define BLYNK_PRINT Serial

#include <SPI.h>
#include <Ethernet.h>
#include <BlynkSimpleEthernet.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#include "main.h"
#include "temperature_system.h"
#include "ldr.h"
#include "serial_tank.h"

char auth[] = BLYNK_AUTH_TOKEN;
bool heater_sw,inlet_sw,outlet_sw;
unsigned int tank_volume;

BlynkTimer timer;

LiquidCrystal_I2C lcd(0x27, 16, 2); // set the LCD address to 0x27 for a 16 chars and 2 line display

// This function is called every time the Virtual Pin 0 state changes
/*To turn ON and OFF cooler based virtual PIN value*/
BLYNK_WRITE(COOLER_V_PIN)
{
  int value =param.asInt();
  if (value)
  {
    cooler_control(ON);
    lcd.setCursor(7,0);
    lcd.print("CO_LR ON ");
  }
  else 
  {
    cooler_control(OFF);
     lcd.setCursor(7,0);
    lcd.print("CO_LR OFF");
  }
}


/*To turn ON and OFF heater based virtual PIN value*/ 
BLYNK_WRITE(HEATER_V_PIN )
{
  heater_sw =param.asInt();
   if (heater_sw)
  {
    heater_control(ON);
    lcd.setCursor(7,0);
    lcd.print("HT_R  ON ");
  }
  else 
  {
    heater_control(OFF);
     lcd.setCursor(7,0);
    lcd.print("HT_R  OFF");
  }
  
}
/*To turn ON and OFF inlet vale based virtual PIN value*/
BLYNK_WRITE(INLET_V_PIN)
{
  /*reading the value present on the virtual pin INLET_V_PIN and storing it in inlet_sw*/
  inlet_sw = param.asInt();
  if(inlet_sw)
  {
    enable_inlet();
    lcd.setCursor(7,1);
    lcd.print("IN_FL_ON");
  }
  else
  {
    disable_inlet();
    lcd.setCursor(7,1);
    lcd.print("IN_FL_OFF");
  }
}
/*To turn ON and OFF outlet value based virtual switch value*/
BLYNK_WRITE(OUTLET_V_PIN)
{
  outlet_sw = param.asInt();
  if(outlet_sw)
  {
    enable_outlet();
    lcd.setCursor(7,1);
    lcd.print("OT_FL_ON");
  }
  else
  {
    disable_outlet();
    lcd.setCursor(7,1);
    lcd.print("OT_FL_OFF");
  }
}
/* To display temperature and water volume as gauge on the Blynk App*/  
void update_temperature_reading()
{
  // You can send any value at any time.
  // Please don't send more that 10 values per second.
  /*sending temperature read to temp gauge for every 1 sec*/
  Blynk.virtualWrite(TEMPERATURE_GAUGE,read_temperature());
  /*send volume of water in tank for every 1 sec*/
  Blynk.virtualWrite(WATER_VOL_GAUGE  ,volume());
}

/*To turn off the heater if the temperature raises above 35 deg C*/
void handle_temp(void)
{
  if ((read_temperature() > float (35)) && heater_sw)
  {
    heater_sw=0;
    heater_control(OFF);
     /*send notification to dashboard*/
    lcd.setCursor(7,0);
    lcd.print("HT_R  OFF");
    /*send notification to Blynk Iot app*/
    Blynk.virtualWrite(BLYNK_TERMINAL_V_PIN, "temperature is above 35 degree celcius\n");
    Blynk.virtualWrite(BLYNK_TERMINAL_V_PIN, "turning off the heater\n");
    /*to turnoff the heater widget button*/
    Blynk.virtualWrite(HEATER_V_PIN , 0);
   }


   
}

/*To control water volume above 2000ltrs*/
void handle_tank(void)
{
  /*vol < 2000 and inlet valve off*/
  if ((tank_volume < 2000) && (inlet_sw ==0))
{
    enable_inlet();
    inlet_sw = 1;
    /*to print notification on dashboard*/
    lcd.setCursor(7,1);
    lcd.print("IN_FL_ON");
    /*to print notification on blynk iot app*/
    Blynk.virtualWrite(BLYNK_TERMINAL_V_PIN, "water level is less than 2000\n");
    Blynk.virtualWrite(BLYNK_TERMINAL_V_PIN, "water inflow enabled\n");

    /* reflect the status ON on the widget button of Inlet valve*/
    Blynk.virtualWrite(INLET_V_PIN , 1);


}


  /*if vol is 3000 and if inlet valve is ON disable inflow*/
  if ((tank_volume == 3000) &&(inlet_sw ==1))

  {
    disable_inlet();
    inlet_sw = 0;
    /*to print notification on dashboard*/
    lcd.setCursor(7,1);
    lcd.print("IN_FL_ON");
    /*to print notification on blynk iot app*/
    Blynk.virtualWrite(BLYNK_TERMINAL_V_PIN, "water level is full\n");
    Blynk.virtualWrite(BLYNK_TERMINAL_V_PIN, "water inflow disabled\n");

    /* reflect the status OFF on the widget button of Inlet valve*/
    Blynk.virtualWrite(INLET_V_PIN , 0);
  }

}
  



void setup(void)
{
    Blynk.begin(auth);
     /*initialize the lcd*/
    lcd.init();        
    /*turn the backlight*/             
    lcd.backlight();
    /*clear the clcd*/
    lcd.clear();
    /*cursor to the home*/
    lcd.home();
    /* to display temp*/
    lcd.setCursor(0,0);
    lcd.print("T=");
    /*display the volume*/
    lcd.setCursor(0,1);
    lcd.print("v=");
     
    /*initialisation temp system*/
    init_temperature_system();
    /*initialisation LDR*/
    //init_LDR();
    /*initialisation Serial tank*/
    init_serial_tank();
    //update temperature to blynk app for ever 1 sec
    timer.setInterval(1000L, update_temperature_reading);
}

void loop(void) 
{ 
   /* to run blynk related function*/
      Blynk.run();
      /*to call setinterval at perticulat period*/
      timer.run();

      /*read temperature and displayit on the dashboard*/
      String temperature;
      temperature = String (read_temperature(), 2);//27.37 27
      lcd.setCursor(2,0);
      lcd.print(temperature);
      /*read the volume of water in the tank and display it on the dashboard*/
      tank_volume = volume();
      lcd.setCursor(2,1);
      lcd.print(tank_volume);

      /* to control the garden lights bassen on light intensity*/
      brightness_control();
      /*to control thershold temperature of 35 degree*/
      handle_temp();

      /*to control volume of the water in the tank*/
      handle_tank();
}
