 A simple Weather station receiver for and not limited to Bresser/Garni, It shows info via a local web UI, Made with a CC1101 and D1 mini (ESP8266) 
 
![image](https://github.com/user-attachments/assets/3abae9a5-da13-4954-bd19-1b025aa06c66)

 Connections:
 
![image](https://github.com/user-attachments/assets/78f303c8-a2fe-48b2-8f60-2aa954468118)
![image](https://github.com/user-attachments/assets/07f7664a-dc10-4586-98b4-dfb3d8cedcca)
![image](https://github.com/user-attachments/assets/d5714e7a-674e-4970-b3ef-adf265de4cbe)
**Note: There are green modules for sale which says 868M but it doesnt change anything plus the antenna is impossible to solder on, get the blue one**
| CC1101 (433) | D1 Mini Pin | CC1101 (868) |
|------------|-------------|---------------|
| 1          | GND         |2|
| 2          | 3.3V        |1|
| 3          | D2          |7|
| 4          | D8          |8|
| 5          | D5          |4|
| 6          | D7          |3|
| 7          | D6          |5|
| 8          | D1          |6|

![image](https://github.com/user-attachments/assets/06f738db-46e2-4a0f-88e7-0058c893d462)


### Software setup:
When you first program your D1 mini and everything is wired up,
you'll get a new Acsess point called `WeatherSetupAP`, connect to it
the password is `ICantMakeThings` Next go to `http://192.168.4.1/` press "Configure WiFi" Wifi, select your
home wifi from the list and enter the password, and press save.
A window saying its saving the password will show, it should
take about 1 minuite to save, once done the AP will dissapear.
to acsess your weather info, open this link: [http://weather.lan](http://weather.lan/)
!!!Only on supported home networks!!! else you have to get the local ip, 192.168.x.x (find out via router or look at logs [here](https://serial.huhn.me/))
If you have multiple recievers (make sure they are away enough
from eachother to not get data from the wrong weather station!)
the next link will be [http://weather1.lan](http://weather1.lan/) then 2, 3, 4 and so on.
to reset wifi, go to a serial terminal like [this](https://serial.huhn.me/) and type in terminal `resetwifi` to reset to factory settings.

### Flashing:
Download latest [release](https://github.com/ICantMakeThings/Bresser-Garni-Weather-Info-Reciever/releases/download/Release/BGWIR-V2.bin),
If your default download folder on your browser is downloads then all you need to do is:
`esptool.py --port /dev/ttyUSB0 write_flash 0x00000 Downloads/BGWIR-V2.bin`
If your on windows then download and run esptool.py and do the same command as above except you replace
`/dev/ttyUSB0` with `COM` and the number of the com port that the D1 mini is connected to eg: `COM4`
