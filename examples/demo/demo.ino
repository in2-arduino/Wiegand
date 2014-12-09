/*
 * Your reader should have at least 4 connections (some readers have more).  Connect the Red wire 
 * to 5V.  Connect the black to ground.  Connect the green wire (DATA0) to Digital Pin 2 (INT0).  
 * Connect the white wire (DATA1) to Digital Pin 3 (INT1).  That's it!
*/

#include "Wiegand.h"

#define WIEGAND_DATA_0 2			// Wiegand line pins
#define WIEGAND_DATA_1 3

// instantiate global Wiegand protocol communication class using pins 2 for Data0 and 3 for Data1
Wiegand wiegand(WIEGAND_DATA_0, WIEGAND_DATA_1);

void setup()
{

	Serial.begin(115200);

	if (wiegand.begin()) 
		Serial.println("Wiegand init successful");
	else
		Serial.println("Wiegand init failed");

}

void loop()
{

	// We need to check that we have some actual data to process
	if(wiegand.finishRead())
	{
		Serial.println("");
		wiegand.print();


		switch(wiegand.bit_count)
		{
			case 36 :      
				// 36 bits is probably a card swipe, you can read message content like this
				for (int i = 0; i < WIEGAND_MAX_BYTES; i++)
					Serial.println(wiegand.rcv_buffer[i], HEX); 
				break;

			case 8 : 
				// 8 bits is probably a reader key press
				switch(wiegand.rcv_buffer[0])
				{
					// case: ...
				}
				break;

			default :
				// handle unexpected message lengths
				Serial.println("Unsupported message length");
				break;  
		}

		// clean up and get ready for the next card or key press
		wiegand.clear();

	}
	else if (wiegand.status == Wiegand::Error)
	{
		Serial.println("");
		wiegand.print();
		wiegand.clear();
	}

}
