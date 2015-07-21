/**
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2015 Sensnology AB
 * Full contributor list: https://github.com/mysensors/Arduino/graphs/contributors
 *
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 */


#ifndef MyMessage_h
#define MyMessage_h

#ifdef __cplusplus
#include <Arduino.h>
#include <string.h>
#include <stdint.h>
#endif

#define PROTOCOL_VERSION 2
#define MAX_MESSAGE_LENGTH 32
#define HEADER_SIZE 7
#define MAX_PAYLOAD (MAX_MESSAGE_LENGTH - HEADER_SIZE)

// Message types
typedef enum {
	C_PRESENTATION = 0,
	C_SET = 1,
	C_REQ = 2,
	C_INTERNAL = 3,
	C_STREAM = 4 // For Firmware and other larger chunks of data that need to be divided into pieces.
} mysensor_command;

// Type of sensor data (for set/req/ack messages)
typedef enum {
	V_TEMP, // S_TEMP
	V_HUM, // S_HUM
	V_LIGHT, // S_LIGHT, S_DIMMER 
	V_DIMMER, // S_DIMMER
	V_PRESSURE, // S_BARO
	V_FORECAST, // S_BARO
	V_RAIN, // S_RAIN
	V_RAINRATE, // S_RAIN
	V_WIND, // S_WIND
	V_GUST,  // S_WIND
	V_DIRECTION, // S_WIND 
	V_UV, // S_UV
	V_WEIGHT, // S_WEIGHT
	V_DISTANCE, // S_DISTANCE
	V_IMPEDANCE, // S_MULTIMETER, S_WEIGHT
	V_ARMED, // S_DOOR, S_MOTION, S_SMOKE, S_SPRINKLER
	V_TRIPPED, // S_DOOR, S_MOTION, S_SMOKE, S_SPRINKLER
	V_WATT, // S_POWER, S_LIGHT, S_DIMMER
	V_KWH, // S_POWER
	V_SCENE_ON, // S_SCENE_CONTROLLER
	V_SCENE_OFF, // S_SCENE_CONTROLLER
	V_HEATER, // S_HEATER
	V_HEATER_SW,  // S_HEATER
	V_LIGHT_LEVEL, // S_LIGHT_LEVEL (light level in uncalibrated percentage)
	V_VAR1, V_VAR2, V_VAR3, V_VAR4, V_VAR5,
	V_UP, // S_COVER
	V_DOWN, // S_COVER
	V_STOP, // S_COVER
	V_IR_SEND, // S_IR
	V_IR_RECEIVE, // S_IR
	V_FLOW, // S_WATER
	V_VOLUME, // S_WATER
	V_LOCK_STATUS, // S_LOCK
	V_DUST_LEVEL, // S_DUST
	V_VOLTAGE, // S_MULTIMETER 
	V_CURRENT, // S_MULTIMETER
	V_RGB, 	// S_RGB_LIGHT, S_COLOR_SENSOR. 
					// Used for sending color information for led lighting or color sensors. 
					// Sent as ascii hex. RRGGBB (RR=red, GG=green, BB=blue component)
	V_RGBW, // S_RGB_LIGHT
					// Used for sending color information to led lighting. 
					// Sent as ascii hex. RRGGBBWW (WW=while component)
	V_ID,   // S_TEMP
					// Used for reporting the sensor internal ids (E.g. DS1820b). 
	V_LIGHT_LEVEL_LUX,  // S_LIGHT, Light level in lux
	V_UNIT_PREFIX, // Allows sensors to send in a string representing the 
								 // unit prefix to be displayed in GUI, not parsed! E.g. cm, m, km, inch.
								 // Can be used for S_DISTANCE 
	V_SOUND_DB, // S_SOUND sound level in db
	V_VIBRATION_HZ, // S_VIBRATION vibration i Hz
	V_ENCODER_VALUE, // S_ROTARY_ENCODER. Rotary encoder value.
	
} mysensor_data;

// Type of internal messages (for internal messages)
typedef enum {
	I_BATTERY_LEVEL, I_TIME, I_VERSION, I_ID_REQUEST, I_ID_RESPONSE,
	I_INCLUSION_MODE, I_CONFIG, I_FIND_PARENT, I_FIND_PARENT_RESPONSE,
	I_LOG_MESSAGE, I_CHILDREN, I_SKETCH_NAME, I_SKETCH_VERSION,
	I_REBOOT, I_GATEWAY_READY, I_REQUEST_SIGNING, I_GET_NONCE, I_GET_NONCE_RESPONSE
} mysensor_internal;

// Type of sensor  (for presentation message)
typedef enum {
	S_DOOR, // V_TRIPPED, V_ARMED
	S_MOTION,  // V_TRIPPED, V_ARMED 
	S_SMOKE,  // V_TRIPPED, V_ARMED 
	S_LIGHT, // V_LIGHT, V_WATT
	S_DIMMER, // V_LIGHT, V_DIMMER, V_WATT
	S_COVER, // V_UP, V_DOWN, V_STOP
	S_TEMP, // V_TEMP
	S_HUM, // V_HUM
	S_BARO, // V_PRESSURE, V_FORECAST
	S_WIND, // V_WIND, V_GUST
	S_RAIN, // V_RAIN, V_RAINRATE
	S_UV, // V_UV
	S_WEIGHT, // V_WEIGHT, V_IMPEDANCE
	S_POWER, // V_WATT, V_KWH
	S_HEATER, // V_HEATER, V_HEATER_SW
	S_DISTANCE, // V_DISTANCE
	S_LIGHT_LEVEL, // V_LIGHT_LEVEL
	S_ARDUINO_NODE,
	S_ARDUINO_REPEATER_NODE, 
	S_LOCK, // V_LOCK_STATUS
	S_IR, // V_IR_SEND, V_IR_RECEIVE
	S_WATER, // V_FLOW, V_VOLUME
	S_AIR_QUALITY, // V_VAR1 
	S_CUSTOM, 
	S_DUST, // V_DUST_LEVEL
	S_SCENE_CONTROLLER, // V_SCENE_ON, V_SCENE_OFF. 
	S_RGB_LIGHT, // Send data using V_RGB or V_RGBW 
	S_COLOR_SENSOR,  // Send data using V_RGB
	S_MULTIMETER, // V_VOLTAGE, V_CURRENT, V_IMPEDANCE 
	S_SPRINKLER,  // V_TRIPPED, V_ARMED
	S_WATER_LEAK, // V_TRIPPED, V_ARMED
	S_SOUND, // V_TRIPPED, V_ARMED, V_SOUND_DB
	S_VIBRATION, // V_TRIPPED, V_ARMED, V_VIBRATION_HZ 
	S_ROTARY_ENCODER, // V_ENCODER_VALUE
} mysensor_sensor;

// Type of data stream  (for streamed message)
typedef enum {
	ST_FIRMWARE_CONFIG_REQUEST, ST_FIRMWARE_CONFIG_RESPONSE, ST_FIRMWARE_REQUEST, ST_FIRMWARE_RESPONSE,
	ST_SOUND, ST_IMAGE
} mysensor_stream;

typedef enum {
	P_STRING, P_BYTE, P_INT16, P_UINT16, P_LONG32, P_ULONG32, P_CUSTOM, P_FLOAT32
} mysensor_payload;



#define BIT(n)                  ( 1<<(n) )
// Create a bitmask of length len.
#define BIT_MASK(len)           ( BIT(len)-1 )
// Create a bitfield mask of length starting at bit 'start'.
#define BF_MASK(start, len)     ( BIT_MASK(len)<<(start) )

// Prepare a bitmask for insertion or combining.
#define BF_PREP(x, start, len)  ( ((x)&BIT_MASK(len)) << (start) )
// Extract a bitfield of length len starting at bit 'start' from y.
#define BF_GET(y, start, len)   ( ((y)>>(start)) & BIT_MASK(len) )
// Insert a new bitfield value x into y.
#define BF_SET(y, x, start, len)    ( y= ((y) &~ BF_MASK(start, len)) | BF_PREP(x, start, len) )

// Getters/setters for special bit fields in header
#define mSetVersion(_msg,_version) BF_SET(_msg.version_length, _version, 0, 2)
#define mGetVersion(_msg) BF_GET(_msg.version_length, 0, 2)

#define mSetSigned(_msg,_signed) BF_SET(_msg.version_length, _signed, 2, 1)
#define mGetSigned(_msg) BF_GET(_msg.version_length, 2, 1)

#define mSetLength(_msg,_length) BF_SET(_msg.version_length, _length, 3, 5)
#define mGetLength(_msg) BF_GET(_msg.version_length, 3, 5)

#define mSetCommand(_msg,_command) BF_SET(_msg.command_ack_payload, _command, 0, 3)
#define mGetCommand(_msg) BF_GET(_msg.command_ack_payload, 0, 3)

#define mSetRequestAck(_msg,_rack) BF_SET(_msg.command_ack_payload, _rack, 3, 1)
#define mGetRequestAck(_msg) BF_GET(_msg.command_ack_payload, 3, 1)

#define mSetAck(_msg,_ackMsg) BF_SET(_msg.command_ack_payload, _ackMsg, 4, 1)
#define mGetAck(_msg) BF_GET(_msg.command_ack_payload, 4, 1)

#define mSetPayloadType(_msg, _pt) BF_SET(_msg.command_ack_payload, _pt, 5, 3)
#define mGetPayloadType(_msg) BF_GET(_msg.command_ack_payload, 5, 3)


// internal access for special fields
#define miGetCommand() BF_GET(command_ack_payload, 0, 3)

#define miSetLength(_length) BF_SET(version_length, _length, 3, 5)
#define miGetLength() BF_GET(version_length, 3, 5)

#define miSetRequestAck(_rack) BF_SET(command_ack_payload, _rack, 3, 1)
#define miGetRequestAck() BF_GET(command_ack_payload, 3, 1)

#define miSetAck(_ack) BF_SET(command_ack_payload, _ack, 4, 1)
#define miGetAck() BF_GET(command_ack_payload, 4, 1)

#define miSetPayloadType(_pt) BF_SET(command_ack_payload, _pt, 5, 3)
#define miGetPayloadType() BF_GET(command_ack_payload, 5, 3)


#ifdef __cplusplus
class MyMessage
{
private:
	char* getCustomString(char *buffer) const;

public:
	// Constructors
	MyMessage();

	MyMessage(uint8_t sensor, uint8_t type);

	char i2h(uint8_t i) const;

	/**
	 * If payload is something else than P_STRING you can have the payload value converted
	 * into string representation by supplying a buffer with the minimum size of
	 * 2*MAX_PAYLOAD+1. This is to be able to fit hex-conversion of a full binary payload.
	 */
	char* getStream(char *buffer) const;
	char* getString(char *buffer) const;
	const char* getString() const;
	void* getCustom() const;
	uint8_t getByte() const;
	bool getBool() const;
	float getFloat() const;
	long getLong() const;
	unsigned long getULong() const;
	int getInt() const;
	unsigned int getUInt() const;

	// Getter for ack-flag. True if this is an ack message.
	bool isAck() const;

	// Setters for building message "on the fly"
	MyMessage& setType(uint8_t type);
	MyMessage& setSensor(uint8_t sensor);
	MyMessage& setDestination(uint8_t destination);

	// Setters for payload
	MyMessage& set(void* payload, uint8_t length);
	MyMessage& set(const char* value);
	MyMessage& set(uint8_t value);
	MyMessage& set(float value, uint8_t decimals);
	MyMessage& set(unsigned long value);
	MyMessage& set(long value);
	MyMessage& set(unsigned int value);
	MyMessage& set(int value);

#else

typedef union {
struct
{

#endif
	uint8_t last;            	 // 8 bit - Id of last node this message passed
	uint8_t sender;          	 // 8 bit - Id of sender node (origin)
	uint8_t destination;     	 // 8 bit - Id of destination node

	uint8_t version_length;		 // 2 bit - Protocol version
			                     // 1 bit - Signed flag
			                     // 5 bit - Length of payload
	uint8_t command_ack_payload; // 3 bit - Command type
	                             // 1 bit - Request an ack - Indicator that receiver should send an ack back.
								 // 1 bit - Is ack messsage - Indicator that this is the actual ack message.
	                             // 3 bit - Payload data type
	uint8_t type;            	 // 8 bit - Type varies depending on command
	uint8_t sensor;          	 // 8 bit - Id of sensor that this message concerns.

	// Each message can transfer a payload. We add one extra byte for string
	// terminator \0 to be "printable" this is not transferred OTA
	// This union is used to simplify the construction of the binary data types transferred.
	union {
		uint8_t bValue;
		unsigned long ulValue;
		long lValue;
		unsigned int uiValue;
		int iValue;
		struct { // Float messages
			float fValue;
			uint8_t fPrecision;   // Number of decimals when serializing
		};
		struct {  // Presentation messages
			uint8_t version; 	  // Library version
   		    uint8_t sensorType;   // Sensor type hint for controller, see table above
		};
		char data[MAX_PAYLOAD + 1];
	} __attribute__((packed));
#ifdef __cplusplus
} __attribute__((packed));
#else
};
uint8_t array[HEADER_SIZE + MAX_PAYLOAD + 1];	
} __attribute__((packed)) MyMessage;
#endif

#endif
