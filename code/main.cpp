//! \file main.cpp
//! \brief Main file
//! \author Baptiste Hamard, Franck Mercier, Remy Guyonneau
//! \date 2017 05 11
//!
//! Main file for the Power card.
//! To change the warning identifier : modify the ID_ALERT constant
//! To change the request identifier : modify the ID_POWER_1 constant
//! To change the min reference voltage : modify the MIN_VOLTAGE constant
#define FOSC           16000
#define F_CPU          16000000UL

#include <util/delay.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#include "led.h"
#include "asf/adc.h"

// Configuration declaration for the RED LED wiring
#define LED_RED_PORT PORTB //!< The ATMega port where the red LED is wired
#define LED_RED_PIN  3     //!< The ATMega pin where the red LED is wired 
#define LED_RED_POL  0     //!< The polarity of the red LED 

// Configuration declaration for the YELLOW LED wiring
#define LED_YELLOW_PORT PORTB //!< The ATMega port where the red LED is wired
#define LED_YELLOW_PIN  2     //!< The ATMega pin where the red LED is wired 
#define LED_YELLOW_POL  0     //!< The polarity of the red LED 


#define ID_POWER_1  0x091 //!< the identifier of the request Power card message
#define ID_ALERT    0x080 //!< the identifier of the Voltage Alert message

//Supply alert : 14,4V = 864
#define MIN_VOLTAGE 1440//864
//calcul : ADC = Voltage_Alert * 60


//! \fn void initCANBus()
//! \brief Intialize the CAN Bus.
//!
//! Function that initializes the CANBus according to the wiring and the needed speed (500kb/s).
//! It also enable the CAN module interruptions
void initCANBus(){
    // Enable of CAN transmit
    DDRC|=0x80; // Configuration of the standby pin as an output
                // (for a transceiver MCP2562 with the stby pin wired to the uC PC7)
    // Enable the MCP2562 (STANDBY to 0)
    PORTC &= 0x7F; // Activation of the MCP2562
                   // (for a transceiver MCP2562 with the stby pin wired to the uC PC7)

    // Initialization of CAN module
    CANGCON  = (1 << SWRES); // Reset CAN module
    CANGCON  = 0x02; // Enable CAN module

    CANBT1 = 0x06; // Speed Config (500kb/s)
    CANBT2 = 0x04;
    CANBT3 = 0x13;

    CANHPMOB = 0x00; // No priority between the CAN MOB

    // Enable CAN interruptions and especially reception interruptions
    CANGIE |= 0xA0;
}

//! \fn void initCANMOB()
//! \brief Intialize the CAN MOB.
//!
//! Intialize the CAN MOB. A MOB can be seen as a "CAN socket".
//! In this case the MOB1 is used to receive remote request
//! and MOB0 is used to send the Ultra sound value
void initCANMOB(){
    CANPAGE = 0x10; // selection of MOB1 (reveice request)

    CANIDT4 = 0x04; // Config as reception remote (rtr = 1)
    CANIDT3 = 0x00;
    CANIDT2 = (uint8_t)( (ID_POWER_1 & 0x00F)<< 5 ); // using the constant to configure the remote request identifier
    CANIDT1 = (uint8_t)( ID_POWER_1 >> 3 );

    CANIDM4 = 0x04; // mask over the rtr value
    CANIDM3 = 0xFF; // mask over the identifier (we only want an interruption if the received CAN message is a remote request with the corresponding identifier)
    CANIDM2 = 0xFF;
    CANIDM1 = 0xFF;

    CANCDMOB = 0x80; // Config MOB as reception
    
    CANIE2 = 0x02; // enable the interruptions over the MOB 1

    sei(); // set enable interruption
}

Led redLed(&LED_RED_PORT, LED_RED_PIN, LED_RED_POL); //!< The RED Led
Led yellowLed(&LED_YELLOW_PORT, LED_YELLOW_PIN, LED_YELLOW_POL); //!< The RED Led

//! \fn int main(void)
//! \brief Main function.
//! It makes the led blink when starting, then initialize the CANBus and finally loop forever
int main(void) {
    for(int i=0; i<4; i++){
        redLed.blink();
		_delay_ms(200);
        yellowLed.blink();
		_delay_ms(200);
    }

    initCANBus(); // intialization of the CAN Bus
    initCANMOB(); // intialization of the CAN MOB

    adc_init(ADC_PRESCALER_DIV128); //Init ADC (prescaler)

    TIFR1  |= 0x04; //Clear match interrupt bit between Timer and data in OCR1B
    TCCR1B |= 0x0D; //Prescaler = 1024 + set CTC bit
    TCCR1A |= 0;    //select CTC (comparaison) mode + Normal port operation (OCnA and OCnB disconected = no compare Pins bit)
    OCR1A   = 15624; //Timer compare value (65532 -> 4sec, 7812 -> 1sec)
    
    //              2 * Prescaler(TCCR1B register)    
    //T(secondes) = ------------------------------ * (OCR1A + 1)
    //                     16 000 000
    TIMSK1 |= (1<<OCIE1A);
    
    while(1){ }

}


//! \fn ISR(CAN_INT_vect)
//! \brief CAN interruption.
//! This function is called when an CAN interruption is raised.
//! When it appends, the US sensor value is readed and sent using the MOB 0
ISR(CAN_INT_vect) {
    cli(); // disable the interruption (no to be disturbed when dealing with one)
    uint16_t  v_5,v_12_1, v_bat, i_bat;

    yellowLed.blink(50); // blink the LED
	//redLed.blink();

    CANPAGE = 0x10; // Selection of MOB 1
    CANSTMOB = 0x00; // Reset the status of the MOB
    CANCDMOB = 0x80; // Config as reception MOB
    CANIE2 = 0x02; // Enable the interruption over MOB 1 (for the next one)

    v_12_1 = adc_read_10bit(ADC_MUX_ADC7, ADC_VREF_AVCC); //ADC_MUX_ADC6 = PB5
    v_5    = adc_read_10bit(ADC_MUX_ADC4, ADC_VREF_AVCC); //ADC_MUX_ADC4 = PB7
    v_bat  = adc_read_10bit(ADC_MUX_ADC2, ADC_VREF_AVCC); //ADC_MUX_ADC2 = PD5
    i_bat  = adc_read_10bit(ADC_MUX_ADC3, ADC_VREF_AVCC); //ADC_MUX_ADC3 = PD6

	v_12_1 /= 0.824121;
	v_5 /= 2.046;
	v_bat /= 0.613636;
	i_bat /= 3.2222;

    CANPAGE  = 0x00; //Selection of MOB 0

    CANIDT4 = 0x00; // Config as data (rtr = 0)
    CANIDT3 = 0x00;
    CANIDT2 = (uint8_t)( (ID_POWER_1 & 0x00F)<< 5 ); // use the same identifier as the remote request
    CANIDT1 = (uint8_t)( ID_POWER_1 >> 3 );


    CANMSG = (uint8_t) (v_12_1 >> 8); CANPAGE = 0x01;
    CANMSG = (uint8_t) (v_12_1);      CANPAGE = 0x02;
    CANMSG = (uint8_t) (v_5 >> 8);    CANPAGE = 0x03;
    CANMSG = (uint8_t) (v_5);         CANPAGE = 0x04;
    CANMSG = (uint8_t) (v_bat >> 8);  CANPAGE = 0x05;
    CANMSG = (uint8_t) (v_bat);       CANPAGE = 0x06;
    CANMSG = (uint8_t) (i_bat >> 8);  CANPAGE = 0x07;
    CANMSG = (uint8_t) (i_bat);


    CANCDMOB = 0x48;// send the message using the MOB 0 - DLC = 8

    sei(); // enable the interruptions
}

ISR(TIMER1_COMPA_vect){
    cli();
	//redLed.blink();
    uint16_t v_bat = adc_read_10bit(ADC_MUX_ADC2, ADC_VREF_AVCC);
	v_bat /= 0.613636;
    TIFR1 |= 0;

//alert : battery low
    if(v_bat < MIN_VOLTAGE){
		redLed.blink();
        CANPAGE  = 0x20; //Selection of MOB 2

        CANIDT4 = 0x00; // Config as data (rtr = 0)
        CANIDT3 = 0x00;
        CANIDT2 = (uint8_t)( (ID_ALERT & 0x00F)<< 5 ); // use the same identifier as the remote request
        CANIDT1 = (uint8_t)( ID_ALERT >> 3 );


        CANMSG = (uint8_t) (v_bat >> 8); CANPAGE = 0x21;
        CANMSG = (uint8_t) (v_bat);

        CANCDMOB = 0x42; // send the message using the MOB 0 - DLC = 2
    }
    sei();
}
