#include "Wiegand.h"

// port to interrupt mapping is different for different boards
// mapping ports other than these results in init failure
// Board			int0	int1	int2	int3	int4	int5	
// Uno, Ethernet	2		3         
// Mega2560			2		3		21		20		19		18	
// Leonardo			3		2		0		1		7			
// Please note that Arduino interruptt numbers don't allways corespond with Atmel numbers
// i.e. Leonard INT4 is mapped to 32U4 INT6, which means that indexes used with attachInterrupt
// are different from those used for direct register manipulation
// for more info check http://www.gammon.com.au/images/Arduino/attachInterruptPinMappings.png
bool Wiegand::attachInterrupts(const byte pin, bool meaning)
{
	switch(pin)
	{
#if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega168__)
		case 2:
			if (_int0_instance == NULL)
				_int0_instance = this;
			else
				return false;				
			if (meaning)
				attachInterrupt(0, isr0_high, FALLING);
			else
				attachInterrupt(0, isr0_low, FALLING);
			bitSet(EIFR, INTF0);
			break;
		case 3:
			if (_int1_instance == NULL)
				_int1_instance = this;
			else
				return false;				
			if (meaning)
				attachInterrupt(1, isr1_high, FALLING);
			else
				attachInterrupt(1, isr1_low, FALLING);
			bitSet(EIFR, INTF1);
			break;
#elif defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
		case 2:
			if (_int0_instance == NULL)
				_int0_instance = this;
			else
				return false;				
			if (meaning)
				attachInterrupt(0, isr0_high, FALLING);
			else
				attachInterrupt(0, isr0_low, FALLING);
			bitSet(EIFR, INTF4);
			break;
		case 3:
			if (_int1_instance == NULL)
				_int1_instance = this;
			else
				return false;				
			if (meaning)
				attachInterrupt(1, isr1_high, FALLING);
			else
				attachInterrupt(1, isr1_low, FALLING);
			bitSet(EIFR, INTF5);
			break;
		case 21:
			if (_int2_instance == NULL)
				_int2_instance = this;
			else
				return false;				
			if (meaning)
				attachInterrupt(2, isr2_high, FALLING);
			else
				attachInterrupt(2, isr2_low, FALLING);
			bitSet(EIFR, INTF0);
			break;
		case 20:
			if (_int3_instance == NULL)
				_int3_instance = this;
			else
				return false;				
			if (meaning)
				attachInterrupt(3, isr3_high, FALLING);
			else
				attachInterrupt(3, isr3_low, FALLING);
			bitSet(EIFR, INTF1);
			break;
		case 19:
			if (_int4_instance == NULL)
				_int4_instance = this;
			else
				return false;				
			if (meaning)
				attachInterrupt(4, isr4_high, FALLING);
			else
				attachInterrupt(4, isr4_low, FALLING);
			bitSet(EIFR, INTF2);
			break;
		case 18:
			if (_int5_instance == NULL)
				_int5_instance = this;
			else
				return false;				
			if (meaning)
				attachInterrupt(5, isr5_high, FALLING);
			else
				attachInterrupt(5, isr5_low, FALLING);
			bitSet(EIFR, INTF3);
			break;
#elif defined (__AVR_ATmega32U4__)
		case 3:
			if (_int0_instance == NULL)
				_int0_instance = this;
			else
				return false;				
			if (meaning)
				attachInterrupt(0, isr0_high, FALLING);
			else
				attachInterrupt(0, isr0_low, FALLING);
			bitSet(EIFR, INTF0);
			break;
		case 2:
			if (_int1_instance == NULL)
				_int1_instance = this;
			else
				return false;				
			if (meaning)
				attachInterrupt(1, isr1_high, FALLING);
			else
				attachInterrupt(1, isr1_low, FALLING);
			bitSet(EIFR, INTF1);
			break;
		case 0:
			if (_int2_instance == NULL)
				_int2_instance = this;
			else
				return false;				
			if (meaning)
				attachInterrupt(2, isr2_high, FALLING);
			else
				attachInterrupt(2, isr2_low, FALLING);
			bitSet(EIFR, INTF2);
			break;
		case 1:
			if (_int3_instance == NULL)
				_int3_instance = this;
			else
				return false;				
			if (meaning)
				attachInterrupt(3, isr3_high, FALLING);
			else
				attachInterrupt(3, isr3_low, FALLING);
			bitSet(EIFR, INTF3);
			break;
		case 7:
			if (_int4_instance == NULL)
				_int4_instance = this;
			else
				return false;				
			if (meaning)
				attachInterrupt(4, isr4_high, FALLING);
			else
				attachInterrupt(4, isr4_low, FALLING);
			bitSet(EIFR, INTF6);
			break;
#endif
		default:
			return false;
	}
	return true;
}

// constructor
// low_int_pin is number of pin connected to DATA0
// high_int_pin is number of pin connected to DATA1
Wiegand::Wiegand(const byte low_int_pin, const byte high_int_pin)
	:_low_int_pin(low_int_pin), _high_int_pin(high_int_pin), _status(Uninitialized) {}

// initializer
// returns true if successfull, false otherwise
bool Wiegand::begin()
{
	
	// if instance is already initialized return false
	if (_status != Uninitialized)
	{
		return false;
	}

	// Arduino seems to ignore Atmel datasheet regarding correct way to attach external interrupts
	// and triggers ISR on attacnh, so we clear interrupt flag manually after attaching interrupts
	// also, somewhere was mentioned that attaching interrupts to static class members should be atomic
	noInterrupts();
	
	// set ports as inputs
	pinMode(_low_int_pin, INPUT);
	pinMode(_high_int_pin, INPUT);

	// attach ISRs depending on target board
	if (!attachInterrupts(_low_int_pin, LOW))
	{	
		_status = Error;
		return false;
	}
	if (!attachInterrupts(_high_int_pin, HIGH))
	{	
		_status = Error;
		return false;
	}

	// mark the instance as initialized
	_status = Idle;

	// clear internal state
	// but don't disable/enable interrupts internally
	clear(false);
	
	interrupts();

	return true;
}

// ISR glue routines
void Wiegand::isr0_low () {_int0_instance->readBit(LOW);}
void Wiegand::isr1_low () {_int1_instance->readBit(LOW);}
void Wiegand::isr2_low () {_int2_instance->readBit(LOW);}
void Wiegand::isr3_low () {_int3_instance->readBit(LOW);}
void Wiegand::isr4_low () {_int4_instance->readBit(LOW);}
void Wiegand::isr5_low () {_int5_instance->readBit(LOW);}
void Wiegand::isr0_high () {_int0_instance->readBit(HIGH);}
void Wiegand::isr1_high () {_int1_instance->readBit(HIGH);}
void Wiegand::isr2_high () {_int2_instance->readBit(HIGH);}
void Wiegand::isr3_high () {_int3_instance->readBit(HIGH);}
void Wiegand::isr4_high () {_int4_instance->readBit(HIGH);}
void Wiegand::isr5_high () {_int5_instance->readBit(HIGH);}

// for use by ISR glue routines
Wiegand * Wiegand::_int0_instance;
Wiegand * Wiegand::_int1_instance;
Wiegand * Wiegand::_int2_instance;
Wiegand * Wiegand::_int3_instance;
Wiegand * Wiegand::_int4_instance;
Wiegand * Wiegand::_int5_instance;

// class instance to handle an interrupt
void Wiegand::readBit(bool val)
{

	// if instance is not initialized don't do anything
	if (_status == Uninitialized)
	{
		return;
	}

	// we can't receive anything while in error, bailing out
	if (_status == Error)
	{	
		return;
	}
	
	unsigned long current_micros = micros();

	// new message started before last one was fully consumed
	// TODO - implement some kind of message buffer if needed
	if (_status == Done)
	{
		// clear internal state
		_status = Idle;
		_first_micros = micros();
		_bit_count = 0;
		for (byte i = 0; i < WIEGAND_MAX_BYTES; i++)
		{
			_rcv_buffer[i] = 0;
		}
Serial.print("buffer overrun");
	}
	// this covers rare situation when micros counter overflows in the middle of message
	// since it's very rare we simply set internal eror and let the rest of method to propagate it to public state
	else if (_status == Receiving && current_micros < _bit_micros)
	{
		_status = Error;
		return;
	}
	// new message started before last one was consumed
	// TODO - implement some kind of message buffer if needed
	else if (_status == Receiving && current_micros - _bit_micros > WIEGAND_MAX_BIT_INTERVAL)
	{
		// clear internal state
		_status = Idle;
		_first_micros = micros();
		_bit_count = 0;
		for (byte i = 0; i < WIEGAND_MAX_BYTES; i++)
		{
			_rcv_buffer[i] = 0;
		}
Serial.print("buffer overrun");
	}
	// if new message is just starting, record starting timestamp
	else if (_status == Idle)
	{
		_first_micros = micros();
	}

	// check bit counter against max and increment it
	if (_bit_count++ >= WIEGAND_MAX_BITS)
	{
		_status = Error;
		return;
	}
	
	// enter receiving status and store bit timestamp
	_bit_micros = micros();
	_status = Receiving;

	// rotate bits in buffer and store new bit in LSB position
	for (int8_t i = WIEGAND_MAX_BYTES - 1; i > 0 ; i--)
	{
		_rcv_buffer[i] <<= 1;
		_rcv_buffer[i] |= _rcv_buffer[i - 1] >> 7;
	}
	_rcv_buffer[0] <<= 1;
	_rcv_buffer[0] |= val;

	//Serial.print(bit_value, DEC);
	//Serial.print(_status);
	//Serial.print(status);
	//Serial.print(" ");
}

void Wiegand::clear(bool make_atomic)
{
	// if instance is not initialized don't do anything
	if (_status == Uninitialized)
	{
		return;
	}

	if (make_atomic)
		noInterrupts();
	
	_status = Idle;
	status = Idle;
	_first_micros = micros();
	_bit_micros = _first_micros;
	_bit_count = 0;
	bit_count = 0;
	for (byte i = 0; i < WIEGAND_MAX_BYTES; i++)
	{
		_rcv_buffer[i] = 0;
		rcv_buffer[i] = 0;
	}
		
	if (make_atomic)
		interrupts();
}

void Wiegand::print()
{
	
	// if instance is not initialized don't do anything
	if (_status == Uninitialized)
	{
		return;
	}

	switch(status)
	{
		case Done:			
			Serial.print("Wiegand status = Done, Received ");
			Serial.print(bit_count);
			Serial.print(" bits, in ");
			Serial.print(total_micros);
			Serial.print("us rcv_buffer = {");
			for (int8_t i = WIEGAND_MAX_BYTES - 1; i >= 0 ; i--)
			{
					Serial.print(rcv_buffer[i], HEX);
				if (i > 0)
					Serial.print(", ");
			}
			Serial.println("}");
			break;
		case Idle: 
			Serial.println("Wiegand status = Idle"); 
			break;
		case Receiving: 
			Serial.println("Wiegand status = Receveing"); 
			break;
		case Error: 
			Serial.println("Wiegand status = Error");
			break;
	}
}

bool Wiegand::finishRead()
{
	// if instance is not initialized don't do anything
	if (_status == Uninitialized)
	{
		return false;
	}

	// this method is usually called from high frequency loops
	// so it must be atomic
	noInterrupts();
	unsigned long current_micros = micros();
	// this covers rare situation when micros counter overflows in the middle of message
	// since it's very rare we simply set internal eror and let the rest of method to propagate it to public state
	if (_status == Receiving && current_micros < _bit_micros)
	{
		_status = Error;
	}
	if (_status == Receiving && current_micros - _bit_micros > WIEGAND_MAX_BIT_INTERVAL)
	{	
		// latch all internal data into public members
		_status = Done;
		status = Done;
		bit_count = _bit_count;
		total_micros = _bit_micros - _first_micros;
		for (byte i = 0; i < WIEGAND_MAX_BYTES; i++)
			rcv_buffer[i] = _rcv_buffer[i];

		interrupts();
		return true;
	}

	status = _status;
	
	interrupts();
	
	return false;
}

void Wiegand::suspend()
{
	// if instance is not initialized don't do anything
	if (_status == Uninitialized)
	{
		return;
	}

	detachPin(_low_int_pin);
	detachPin(_high_int_pin);
}

void Wiegand::resume()
{
	// if instance is not initialized don't do anything
	if (_status == Uninitialized)
	{
		return;
	}

	attachPin(_low_int_pin);
	attachPin(_high_int_pin);
}


void Wiegand::attachPin(const byte pin)
{
	switch(pin)
	{
#if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega168__)
		case 2: bitSet(EIFR, INT0); bitSet(EIMSK, INT0); break;
		case 3: bitSet(EIFR, INT1); bitSet(EIMSK, INT1); break;
#elif defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
		case 2: bitSet(EIFR, INT4); bitSet(EIMSK, INT4); break;
		case 3: bitSet(EIFR, INT5); bitSet(EIMSK, INT5); break;
		case 21: bitSet(EIFR, INT0); bitSet(EIMSK, INT0); break;
		case 20: bitSet(EIFR, INT1); bitSet(EIMSK, INT1); break;
		case 19: bitSet(EIFR, INT2); bitSet(EIMSK, INT2); break;
		case 18: bitSet(EIFR, INT3); bitSet(EIMSK, INT3); break;
#elif defined (__AVR_ATmega32U4__)
		case 3: bitSet(EIFR, INT0); bitSet(EIMSK, INT0); break;
		case 2: bitSet(EIFR, INT1); bitSet(EIMSK, INT1); break;
		case 0: bitSet(EIFR, INT2); bitSet(EIMSK, INT2); break;
		case 1: bitSet(EIFR, INT3); bitSet(EIMSK, INT3); break;
		case 7: bitSet(EIFR, INT6); bitSet(EIMSK, INT6); break;
#endif
	}
}

void Wiegand::detachPin(const byte pin)
{
	switch(pin)
	{
#if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega168__)
		case 2: bitClear(EIMSK, INTF0); break;
		case 3: bitClear(EIMSK, INTF1); break;
#elif defined(__AVR_ATmega1280__) && defined(__AVR_ATmega2560__)
		case 2: bitClear(EIMSK, INTF4); break;
		case 3: bitClear(EIMSK, INTF5); break;
		case 21: bitClear(EIMSK, INTF0); break;
		case 20: bitClear(EIMSK, INTF1); break;
		case 19: bitClear(EIMSK, INTF2); break;
		case 18: bitClear(EIMSK, INTF3); break;
#elif defined (__AVR_ATmega32U4__)
		case 3: bitClear(EIMSK, INTF0); break;
		case 2: bitClear(EIMSK, INTF1); break;
		case 0: bitClear(EIMSK, INTF2); break;
		case 1: bitClear(EIMSK, INTF3); break;
		case 7: bitClear(EIMSK, INTF6); break;
#endif
	}
}
