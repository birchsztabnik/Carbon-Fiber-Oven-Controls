#include <max6675.h>
#include <LiquidCrystal.h>
#include <Wire.h>
#include <Time.h>
#define SCK 8
#define HEATER_CS 7
#define HEATER_SO 6
#define FAN_CS 10
#define FAN_SO 9
#define HEATER_PIN A2
// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

MAX6675 heater_thermo(SCK, HEATER_CS, HEATER_SO);
MAX6675 fan_thermo(SCK, FAN_CS, FAN_SO);


//analog pin that the potentiometer uses
const int potentiometer_pin = A0;
const int button_pin = 13;

//value of the potentiometer
int p_val = 0;
int button_state = 0;
bool time_set = false;

//when we set the time we capture this
unsigned long current_millis = 0;
int start_total_seconds;
String last_time = "";

int cook_seconds = 0;
int cook_minutes = 0;
int cook_hours   = 0;

int ramp_seconds = 0;
int ramp_minutes = 0;
int ramp_hours   = 0;

int current_ramp_cook_seconds = 0;

int target_temp  = 0;
int og_temp = 0;
//slope of line from start temp to target_temp;
double ramp_slope=0;

//these are used for detecting if the heater and thermo are working
bool fan_fail=false;
bool heater_fail=false;


void setup() {
  // set up the LCD's number of columns and rows:
  lcd.begin(20, 4);
  pinMode(A2,OUTPUT);
  Serial.begin(9600);
}
void print_screen()
{
  lcd.setCursor(0, 0);
  lcd.print("FSAE Carbon Oven");
}
void read_p()
{
  p_val = analogRead(potentiometer_pin);
}
void read_b()
{
  button_state = digitalRead(button_pin);
}
String get_time(int hours, int minutes, int seconds)
{
  //format into hh:mm:ss and add 0s if needed
  String h;
  if (hours < 10)
  {
    h = "0" + String(hours);
  }
  else
  {
    h = String(hours);
  }
  String m;
  if (minutes < 10)
  {
    m = "0" + String(minutes);
  }
  else
  {
    m = String(minutes);
  }
  String s;
  if (seconds < 10)
  {
    s = "0" + String(seconds);
  }
  else
  {
    s = String(seconds);
  }

  return h + ":" + m + ":" + s;
}
bool are_we_done_yet(int hours, int minutes, int seconds)
{
  //were done if there are 0 seconds left
  if (hours == 0 && minutes == 0 && seconds == 0)
  {
    return true;
  }
  else
  {
    return false;
  }
}
void calculate_time(int& hours, int& minutes, int& seconds)
{
  //check if there is any time left
  if (!are_we_done_yet(hours, minutes, seconds))
  {
    if ((millis() - current_millis) >= 1000)
    {
      seconds -= 1;
      if (seconds < 0)
      {
        seconds = 59;
        minutes -= 1;
        if (minutes < 0)
        {
          minutes = 59;
          //no need to check for <0 because of are_we_done_yet
          hours -= 1;
        }
      }
      current_millis = millis();
    }
  }
}
int calc_total_seconds(int hours, int minutes, int seconds)
{
  return (3600 * hours) + (60 * minutes) + seconds;
}
int calc_loading(int hours, int minutes, int seconds)
{
  //we calc percentage done by each phase
  int now = calc_total_seconds(hours, minutes, seconds);
  return (1.0F - ((float)now / (float)start_total_seconds)) * 100;
}

void get_input(int& var, String message, int lower_lim, int upper_lim)
{
  while (button_state == LOW)
  {
    read_b();
    lcd.clear();
    print_screen();
    lcd.setCursor(0, 2);
    lcd.print(message);
    read_p();
    var = map(p_val, 20, 1023, lower_lim, upper_lim);
    lcd.setCursor(0, 3);
    lcd.print(var);
    delay(100);
  }
  lcd.clear();
  lcd.print("Received");
  delay(1000);
  button_state = LOW;
}

String get_temps()
{
  double f = fan_thermo.readFahrenheit();
  double h = heater_thermo.readFahrenheit();
  String f_s = String((int)f);
  String h_s = String((int)h);
  return "Fan:" + f_s+" Heater:"+h_s;
}

void max_cook()
{
  
  if (fan_thermo.readFahrenheit() > target_temp)
  {
    digitalWrite(A2,LOW);
  	lcd.setCursor(0, 1);
  	lcd.print("HEATER OFF");
  }
  else
  {
  	digitalWrite(A2, HIGH);
  	lcd.setCursor(0, 1);
  	lcd.print("HEATER ON");
  }
}
void ramp_cook()
{
  //every time this is called (once a second)
  //the cook power will increase
  double time_specific_temp = ((double)ramp_slope*(double)current_ramp_cook_seconds)+(double)og_temp;
  Serial.println(String(time_specific_temp));
  if (fan_thermo.readFahrenheit() > time_specific_temp)
  {
    digitalWrite(A2,LOW);
  	lcd.setCursor(0, 1);
  	lcd.print("HEATER OFF");
  }
  else
  {
  	digitalWrite(A2, HIGH);
  	lcd.setCursor(0, 1);
  	lcd.print("HEATER ON");
  }
  current_ramp_cook_seconds+=1;
}

//use a function pointer to select what cook mode we want to use
void cycle(int& hours, int& minutes, int& seconds, void (*function)())
{
	//use this to change the heater every 2 seconds instead of 1
	bool every_other=false;
  while (!are_we_done_yet(hours, minutes, seconds))
  {
    calculate_time(hours, minutes, seconds);
    String temp = get_time(hours, minutes, seconds);
    
    if (temp != last_time)
    {
      lcd.clear();
      print_screen();

      //print temperatures
      lcd.setCursor(0, 2);
      String t = get_temps();
      lcd.print(t);
      last_time = temp;
      lcd.setCursor(0, 3);
      lcd.print(last_time);
      lcd.setCursor(11, 3);
      String pctg = String(calc_loading(hours, minutes, seconds)) + "% done";
      lcd.print(pctg);
      //put in here so the cook mode is called every 2 seconds
      //used to ramp up temp
      if(every_other)
      {
      	(*function)();
      }
      every_other=!every_other;
    }
  }
}

void loop()
{
  if (!time_set)
  {
    get_input(target_temp, "Target temp?", 75, 400 );
    get_input(ramp_hours, "# of ramp hours?", 0, 24);
    get_input(ramp_minutes, "# of ramp minutes?", 0, 59);
    get_input(cook_hours, "# of cook hours?", 0, 24);
    get_input(cook_minutes, "# of cook minutes?", 0, 59);
    time_set = true;
    current_millis = millis();
    //how many seconds the cycle will take
    start_total_seconds = calc_total_seconds(ramp_hours, ramp_minutes, ramp_seconds);
    //start at 0 seconds in the ramp cycle
    current_ramp_cook_seconds = 0;
    //calculate the cook slope
    ramp_slope=((double)target_temp-fan_thermo.readFahrenheit())/(double)start_total_seconds;
    og_temp=fan_thermo.readFahrenheit();
  }
  cycle(ramp_hours, ramp_minutes, ramp_seconds, ramp_cook);
  start_total_seconds = calc_total_seconds(cook_hours, cook_minutes, cook_seconds);
  cycle(cook_hours, cook_minutes, cook_seconds, max_cook);
  lcd.clear();
  lcd.print("DONE!");
  lcd.setCursor(0,1);
  lcd.print("Restart system");
  delay(5000);
  analogWrite(HEATER_PIN,0);
}

