/*
 * Wiegand protocol library for Arduino.
 * Copyright (c) 2014 and IN2 Arduino Grupa <in2.arguino@gmail.com>
 *
 * Written by Vedran Latin, based on previous works of 
 *   Daniel Smith - HID RFID Reader Wiegand Interface for Arduino Uno, www.pagemac.com, GPL and
 *   Nick Gammon - Thread about attaching interrupts to class members, 
 *                 http://forum.arduino.cc/index.php?topic=156476.0, no license specified
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of either the GNU General Public License version 2
 * or the GNU Lesser General Public License version 2.1, both as
 * published by the Free Software Foundation.
 *
 * We would appreciate if you shared any improvements with us via email in2.arguino@gmail.com
 *
 * By using this library you assume all responsibility for any malfuncture, unexpected behaviour,
 * error and damages that result in its use. Code is not thoroughly tested and is probably not bug
 * free. PLEASE USE AT YOUR OWN RISK.
 *
 *
 *
 * PROTOCOL DESCRIPTION
 *
 * The Wiegand interface has two data lines, DATA0 and DATA1.  These lines are normally held
 * high at 5V.  When a 0 is sent, DATA0 drops to 0V for a few us.  When a 1 is sent, DATA1 drops
 * to 0V for a few us.  There is usually a few ms between the pulses.
 *
 * Based on several datasheets timings are not strictly defined. Data pulse seems to be around
 * 50us, and interval between bits around 1ms. Our implementation is insensitive to pulse timing
 * since it uses hardware interrupts. You can adjust timing for interval between two bits by
 * redefining WIEGAND_MAX_BIT_INTERVAL.
 *
 *
 *
 * THEORY OF OPERATION
 *
 * It is simple :)
 * Each of the data lines are connected to Arduino hardware interrupt lines.
 * When one drops low, an interrupt routine is called and bit is received. Routine also records
 * time at which this happened by storing the micros() value. When no new bits are received after
 * WIEGAND_MAX_BIT_INTERVAL microseconds, message is marked as complete and can be read.
 * Constructor takes two parameters, first representing a pin connected to DATA0 line, second a pin
 * connected to DATA1 line
 * Class must be initialized before use by calling Wiegand::begin method. If begin returns false,
 * initialization failed and library will not function properly. This happens if you target an
 * unsupported board, provide constructor with pins that don't support hardware interrupts, use the
 * same pin more than once or call begin on already initialized instance.
 * Most methods will return error or do nothing if class is uninitialized (Wiegand::Uninitialized)
 * Member variables rcv_buffer, bit_count and status contain received data, number of received
 * bits, and current status of instance. They are public to make the code smaller, but you should
 * treat them as READ-ONLY.
 * Class is a simple state machine starting in Wigand::Idle state. Upon receiving data, status
 * changes to Wigand::Receiving. When method Wigand::finishRead is called, it checks if last bit
 * was received more than WIEGAND_MAX_BIT_INTERVAL microseconds ago, and if so changes the state
 * to Wigand::Done. Because of that user code MUST poll method Wigand::finishRead before reading
 * rcv_buffer, bit_count or status. All three are internally double buffered and will remain stable
 * between calls to Wiegand::finishRead, but THEY SHOULD BE DISCARDED IF STATUS AFTER POLLING IS NOT
 * WIGAND::DONE. Polling approach was chosen because otherwise one timer would have to be dedicated
 * to library use.
 * After consuming the data, user should call Wigand::clear to change the state back to 
 * Wigand::Idle and prepare the instance for next message.
 * If new message starts before last one was consumed, internal status will be automatically changed
 * to Wiegand::Receiving and last message will be lost. Message buffered by Wiegand::finishRead will
 * not be cleared and can be processed regardless of this.
 * If error is encountered, state is changed to Wigand::Error and Wigand::clear method must be
 * called before any more data can be received.
 * Timing is done via micros() function and relies on standard Arduino settings for micros() timer.
 * Message start is stored in _first_micros member variable and is used for filling total_micros
 * member variable. It was added just to provide info so it can be optimized out by modifying
 * library code. Last bit time is stored in _bit_micros member variable.
 * Buffering and autofinishing were late additions and are probably not bug free. If you expect
 * receiving messages with minimum timings, you should do heavy testing.
 * Methods Wiegand::suspend and Wiegand::resume temporarily disable pin interrupts. This can be
 * useful if you need to completely ignore bus messages for a while
 * 
 *
 *
 * WIRING
 *
 * Library will decode the wiegand protocol from a HID RFID Reader (or, theoretically, any other
 * device that outputs weigand data).
 * Your reader should have at least 4 connections (some readers have more).  Connect the Red wire 
 * to 5V.  Connect the black to ground.  Connect the green wire (DATA0), and white wire (DATA1)
 * to any two pins that support hardware interrupts.
 * Uno has only two such pins available (digital pin 2 and 3 mapped to INT0 AND INT1 respectively)
 * so you must use them and are limited to only physical one Wiegand bus.
 * Mega2560 has 6 such pins (first two are physically compatible with Uno) so you can choose which
 * of them to use. Also, this allows you to connect to a maximum of 3 physical Wiegand buses.
 * Leonard has five interrupt pins, but they are not pin compatible with Uno.
 * Other boards are not supported, but you can easily fix that by mapping them in
 * Wiegand::attachInterrupts method.
 *
 */


#ifndef Wiegand_h_
#define Wiegand_h_

#if ARDUINO >= 100
	#include "Arduino.h"
#else
	#include "WProgram.h"
#endif

#define WIEGAND_MAX_BIT_INTERVAL 5000	// max time between two bits in microseconds
#define WIEGAND_MAX_BITS 36				// max number of bits 
										// max number of storage bytes calculated from MAX_BYTES (do not change)
#define WIEGAND_MAX_BYTES (WIEGAND_MAX_BITS / 8 + (WIEGAND_MAX_BITS % 8 == 0 ? 0 : 1))


class Wiegand
{
	public:
		enum WiegandStatus {Uninitialized, Idle, Receiving, Done, Error};

	private:
		static void isr0_low ();
		static void isr1_low ();
		static void isr2_low ();
		static void isr3_low ();
		static void isr4_low ();
		static void isr5_low ();

		static void isr0_high ();
		static void isr1_high ();
		static void isr2_high ();
		static void isr3_high ();
		static void isr4_high ();
		static void isr5_high ();

		static Wiegand * _int0_instance;
		static Wiegand * _int1_instance;
		static Wiegand * _int2_instance;
		static Wiegand * _int3_instance;
		static Wiegand * _int4_instance;
		static Wiegand * _int5_instance;
		
		const byte _low_int_pin;
		const byte _high_int_pin;
		
		void readBit(bool val);
		bool attachInterrupts(const byte pin, bool meaning);
		void attachPin(const byte pin);
		void detachPin(const byte pin);

		unsigned long _first_micros;
		unsigned long _bit_micros;

		uint8_t _rcv_buffer[WIEGAND_MAX_BYTES];
		uint8_t _bit_count; 
		WiegandStatus _status;

	public:
		WiegandStatus status;
		uint8_t bit_count; 
		uint8_t rcv_buffer[WIEGAND_MAX_BYTES];
		unsigned long total_micros;
		
		Wiegand(const byte low_int_pin, const byte high_int_pin);
		
		bool begin();
		void clear(bool make_atomic = true);
		void print();
		bool finishRead();
		void suspend();
		void resume();
};
#endif