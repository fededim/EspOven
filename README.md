# EspOven © 2017 Federico Di Marco


A fully featured temperature controlled oven based on ESP8266 using either on-off or pid control. The hardware is at https://easyeda.com/fededim/Esp_Oven-d68bf5fe30fc45c9997a2ea43eb93fc2 and supports 2 independent temperature probes and up to 2 relais for heating output. All user operation is performed through a CGI web interface provided by ESP8266 webserver and can be used with any standard clients like a hmtl webpage or a mobile app. An optional I2C/SPI/HMI display may be attached to the board by using any of the provided expansion headers or the optional SX1509 digital io extender.

# How to program the HW board

You have to connect the board to a pc with the a micro USB cable to power it and then connect a USB FTDI cable (pin TX to RX, RX to TX, GND to GND) to the 4 pin header. Unluckily the 4 pin header on board has 3.3V pin instead of a 5V pin (an oversight, it will be changed in next release), otherwise you would be able to connect also 5V pin of FTDI cable to power without using the micro usb cable.

1. Install Arduino IDE
2. Install ESP8266 Boards in Arduino IDE
3. Select Board NodeMCU 1.0
4. Install libraries: ESP8266WiFi, NTPClient, ArduinoJson, PID
5. Set in EspOven.ino SSID and PASSWORD
6. Set in EspOven.ino default configuration (search // Default configuration if flash memory is uninitialised)
7. Hit upload on Arduino IDE. When it is trying to connect keep pushed the PROG button on hw board and press once RESET button. It should connect and upload the code.

# Some pictures

## Board

![espoven board front](https://github.com/user-attachments/assets/e3a4cd36-7c8e-4508-acd9-8f51a11540e7)
![espoven board back](https://github.com/user-attachments/assets/5c67778a-48c4-45b2-ba95-79830964ff48)
![espoven board detail](https://github.com/user-attachments/assets/e57934db-be83-4762-96b9-8aaaa33e0dae)

# Proof of concept using a modded DPE Nerone oven

![EspOven_Overview](https://github.com/user-attachments/assets/1b5e3519-e3ea-49f9-9dcd-5e72b97ee3c5)
![EspOven_Overview_2](https://github.com/user-attachments/assets/a4233ba0-4d09-4150-a704-599cc3ed5887)
![EspOven_Electrical_Connections](https://github.com/user-attachments/assets/25175d43-4391-447d-84c1-ff20c8dac4cd)
![image](https://github.com/user-attachments/assets/99d628ee-4911-4ae2-a7d5-4bc0e955b383)
![image](https://github.com/user-attachments/assets/b2c9a775-49cb-432b-ba28-af0f10baf149)
