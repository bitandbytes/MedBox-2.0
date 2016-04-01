# MedBox 2.0
MedBox 2.0 Programme is a sample programme written to work with the [LED Power Circuit for RPi]( https://github.com/bitandbytes/LED-power-circuit-for-RPi). The repository include the [source code]( https://github.com/bitandbytes/MedBox-2.0-progamme/blob/master/mb-listener.c) for the C programme and a [bash script]( https://github.com/bitandbytes/MedBox-2.0-progamme/blob/master/mb-listener.sh) that compile and load topics from the input file at /root/shine/input.config file. 
## Dependencies:
#### wiringPi
```sh
git clone git://git.drogon.net/wiringPi
cd wiringPi
./build
```
More information is at [wiringpi.com](http://wiringpi.com/download-and-install/)
#### MQTT Paho
```sh
git clone https://github.com/eclipse/paho.mqtt.c.git
cd org.eclipse.paho.mqtt.c.git
make
sudo make install
```
More information at [www.eclipse.org/paho](https://www.eclipse.org/paho/clients/c/)

## Functions of the [bash script]( https://github.com/bitandbytes/MedBox-2.0-progamme/blob/master/mb-listener.sh)
* Get input from input.config file
* Compile the C file if the binary is not generated. 

## Functions of the [C source code]( https://github.com/bitandbytes/MedBox-2.0-progamme/blob/master/mb-listener.c)
* Getting the inputs from the GPIO pins
* Denounce the input signal 
* Publish to the MQTT broker
* Subscribe to the MQTT broker for the LED light signals 
* Light the LED light sets when MQTT message is received 

The programme includes a software denouncer for the input signal. Due to problem of losing published MQTT packets when the function call is too frequent (call before the packet is published) it was required to include a data buffer. Currently the MQTT publishing is done in a separate thread using the buffer. This made the programme very much sensitive to the switch signal and literally unable to fool the switch sensor. 

#### MQTT input publishing format
The drawer number will be shown by the first digit and the last digit will tell whether it is opened or not. 

Eg:   

30 -> Drawer 3 closed

21 -> Drawer 2 open 

#### MQTT LED ON/OFFg format
LED messages range only from 1-6

1 – Set 1 ON

2 - Set 2 ON

3 – Set 3 ON

4 – Set 1 OFF

5 – Set 2 OFF

6 – Set 3 OFF
