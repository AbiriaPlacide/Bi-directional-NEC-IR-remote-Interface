# Bi-directional-NEC-IR-remote-Interface
Firmware interfacing a NEC IR remote, IR sensor (TSOPxx38), Buzzer, and an IR LED for transmission using the NEC IR Transmission Protocol.


### features
  - shell interface using UART
  - Transmision and Decoding
  - Learning new commands using EEPROM as storage
  - retrieve info on a given command
  - delete commands
  - alert on good/bad command by buzzing a different pitch
  
###  board and hardware used
  - Tiva Series tm4c123g 
  - resistors 
  - 10x2 headers 
  - LED's
  - 1 electrolytic capacitor with an npn transistor. Both are used in the driver circuit for the transducer
  - 1 IR sensor: TSOPxx38
  - 1 Piezo Transducer (Buzzer)

### how to build firmware
  1. create a new project in CCStudio. Target: Tiva C series TM4C123GH6PM | Connection: Stellaris In-Circuit Debug Interface.
  2. Make sure to use the TI Compiler or the _delay_cycles() function will not work. 
  3. Dump all .c and .h files in the same directory then  build it.
  4. Connect Dev board to PC then flash the code to MCU. 
  
### GPIO Ports used 
  - Receiving:    PE1 (connected to IR sensor)
  - Transmission: PB4 (connected to IR LED)
  - Buzzer:       PB5 (connected to Piezo Transducer)
  
### POWER
   - 3.3v for IR sensor and IR LED
   - 5.0v for Trans
  

### shell interface
![alt text](https://github.com/AbiriaPlacide/Bi-directional-NEC-IR-remote-Interface/blob/main/images/shell_interface.png)

### Physical Circuit with dev board attached
![alt text](https://github.com/AbiriaPlacide/Bi-directional-NEC-IR-remote-Interface/blob/main/images/finalProjPicture.jpg)
