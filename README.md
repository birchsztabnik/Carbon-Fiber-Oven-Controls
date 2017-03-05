# Carbon Fiber Oven Control System
Designed for RPI's FSAE team, this code controls the carbon fiber curing oven. The oven starts after the user selects the target temperature, 
heat time, and cook time using the potentiometer and button. This is done using 2 MAX6675 sensors and a 220V heater on a relay. With linear interpolation, the arduino figures out what temperature the oven should be
every two seconds during the heat cycle and will turn on and off the heater for a smooth rise in temperature. A timer with percentage tells the user how
long the heat cycle will take. Once the desired temperature is reached, the arduino will turn on and off the heater to keep the oven at the
target temperature until the cook cycle is over.
