# Istia Mecatronique Club - Power Board

## Synopsis

This repository contains the source codes and the schematics of our power board. 

## Repository architecture

This repository contains a "board" folder with the schematics of the board, a "code" folder with the source code of the ATMega and a "documentation" folder with the doxygen documentation of the source code.

## The source code

### How does it work

The board provide the differents voltages needed in our applications (IstiaBOT). It is also equipped with a CAN interface that allows to request the current and voltage values of the input (batterie) by sending a request CAN message with the ID_POWER_1 (0x091) identifier. The board also raise an alert CAN message (identifier ID_ALERT - 0x080) if the input voltage is too low (batterie with a low level).

### The files

The "code/include" folder contains 
 - a Pin interface class to handle a pin on the ATMega (this class sould not be instanciated)
 - an Input class to handle an input pin
 - an Ouput class to handle an output pin
 - a Led class to handle an led connected to the ATMega

### The ATMega output.hex file
In the code folder you can find a Makefile to generate the binary *.hex file and upload it to the ATMega. The makefile is configured for
 - avrdude compiler ( $ sudo apt-get install avrdude avr-libc)
 - AVRispmkII programmer

Then do
 - $ make hex # to generate the binary ouput.hex
 - $ make upload # to upload the output.hex file to the ATMega
 - $ make all  # to compile and then upload 

This has been tester with Ubuntu and Lubuntu on a raspberry pi 3.

## The documentation
We use Doxygen to generate the documentation of the source code. By using the makefile (in the code folder) you can do
 - $ make documentation
To generate the documentation files.

Note that you need to have doxygen and graphviz (for the graphs) installed
 - $ sudo apt-get install doxygen
 - $ sudo apt-get install graphviz


## Tests

When powering the board, the LEDs should blink a few times and then stop.

## License

GNU General Public License v3.0.
