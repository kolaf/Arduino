 /*
 The MySensors library adds a new layer on top of the RF24 library.
 It handles radio network routing, relaying and ids.

 Created by Henrik Ekblad <henrik.ekblad@gmail.com>

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 version 2 as published by the Free Software Foundation.
 */

#include "MySensor.h"
#include "utility/LowPower.h"



// Inline function and macros
inline MyMessage& build (MyMessage &msg, uint8_t sender, uint8_t destination, uint8_t sensor, uint8_t command, uint8_t type, bool enableAck) {
	msg.sender = sender;
	msg.destination = destination;
	msg.sensor = sensor;
	msg.type = type;
	mSetCommand(msg,command);
	mSetRequestAck(msg,enableAck);
	mSetAck(msg,false);
	return msg;
}


MySensor::MySensor(uint8_t _intpin, uint8_t _cepin, uint8_t _cspin) {
	intpin=_intpin;
	cspin=_cspin;
	cepin=_cepin;
}


void MySensor::begin(void (*_msgCallback)(const MyMessage &), uint8_t _nodeId, uint8_t _parentNodeId, uint8_t paLevel, uint16_t frequency) {
	Serial.begin(BAUD_RATE);
	isGateway = false;
	msgCallback = _msgCallback;



	// Read settings from EEPROM
	eeprom_read_block((void*)&nc, (void*)EEPROM_NODE_ID_ADDRESS, sizeof(NodeConfig));
	// Read latest received controller configuration from EEPROM
	eeprom_read_block((void*)&cc, (void*)EEPROM_LOCAL_CONFIG_ADDRESS, sizeof(ControllerConfig));
	if (cc.isMetric == 0xff) {
		// Eeprom empty, set default to metric
		cc.isMetric = 0x01;
	}
	setupRadio(paLevel, frequency);
	if (_nodeId != AUTO) {
		// Set static id
		nc.nodeId = _nodeId;
	}
	manager->setThisAddress(nc.nodeId);


	// Try to fetch node-id from gateway
	if (nc.nodeId == AUTO) {
		requestNodeId();
	}

	debug(PSTR("%s started, id %d\n"), "sensor", nc.nodeId);

	// Open reading pipe for messages directed to this node (set write pipe to same)
	
	// Send presentation for this radio node (attach
	present(NODE_SENSOR_ID, S_ARDUINO_NODE);

	// Send a configuration exchange request to controller
	// Node sends parent node. Controller answers with latest node configuration
	// which is picked up in process()
	sendRoute(build(msg, nc.nodeId, GATEWAY_ADDRESS, NODE_SENSOR_ID, C_INTERNAL, I_CONFIG, false).set(""));

	// Wait configuration reply.'s
	waitForReply();
}

void MySensor::setupRadio(uint8_t paLevel, uint16_t frequency) {
	failedTransmissions = 0;
	
	// Start up the radio library
#ifdef DRH_RF69
	driver=new RH_RF69(cspin,intpin);
#elif defined DRH_RF24	
	driver=new RH_RF24(intpin,cspin);
#endif	
	if (driver) {
		manager=new RHMesh(*driver, nc.nodeId);
		if (!manager->init())
			debug(PSTR("Radio initialisation failed\n"));
	} else {
		debug(PSTR("No valid driver found\n"));
	}
///	driver.init(); Is initialised when initialising the manager..
#ifdef DRH_RF69
	driver->setFrequency(frequency);
	driver->setTxPower(paLevel);
	//driver->setModemConfig(modemChoice);
#endif
}


uint8_t MySensor::getNodeId() {
	return nc.nodeId;
}

ControllerConfig MySensor::getConfig() {
	return cc;
}

void MySensor::requestNodeId() {
	debug(PSTR("req node id\n"));
//	RF24::openReadingPipe(CURRENT_NODE_PIPE, TO_ADDR(nc.nodeId));
	sendRoute(build(msg, nc.nodeId, GATEWAY_ADDRESS, NODE_SENSOR_ID, C_INTERNAL, I_ID_REQUEST, false).set(""));
	waitForReply();
}



void MySensor::waitForReply() {
	unsigned long enter = millis();
	// Wait a couple of seconds for response
	while (millis() - enter < 2000) {
		process();
	}
}

boolean MySensor::sendRoute(MyMessage &message) {
	// Make sure to process any incoming messages before sending (could this end up in recursive loop?)
	// process();
	bool isInternal = mGetCommand(message) == C_INTERNAL;

	// If we still don't have any node id, re-request and skip this message.
	if (nc.nodeId == AUTO && !(isInternal && message.type == I_ID_REQUEST)) {
		requestNodeId();
		return false;
	}

			// --- debug(PSTR("route parent\n"));
		// Should be routed back to gateway.
		bool ok = sendWrite(message);

		if (!ok) {
			// Failure when sending.  This means that the current is no route to the Gateway.  Retry in case some radios become online.
			failedTransmissions++;
		} else {
			failedTransmissions = 0;
		}
		return ok;
	
	return false;
}

boolean MySensor::sendWrite(MyMessage &message) {
	uint8_t length = mGetLength(message);
	mSetVersion(message, PROTOCOL_VERSION);
	uint8_t status = 9;
	status = manager->sendtoWait((uint8_t*) &message, sizeof(message), message.destination);
	debug(PSTR("sent: %d-%d s=%d,c=%d,t=%d,pt=%d,l=%d,st=%d:%s\n"),
			message.sender,message.destination, message.sensor, mGetCommand(message), message.type, mGetPayloadType(message), mGetLength(message), status, message.getString(convBuf));
	if(status != RH_ROUTER_ERROR_NONE){
		return false;
	}

	
	return true;
}


bool MySensor::send(MyMessage &message, bool enableAck) {
	message.sender = nc.nodeId;
	mSetCommand(message,C_SET);
    mSetRequestAck(message,enableAck);
	return sendRoute(message);
}

void MySensor::sendBatteryLevel(uint8_t value, bool enableAck) {
	sendRoute(build(msg, nc.nodeId, GATEWAY_ADDRESS, NODE_SENSOR_ID, C_INTERNAL, I_BATTERY_LEVEL, enableAck).set(value));
}

void MySensor::present(uint8_t childSensorId, uint8_t sensorType, bool enableAck) {
	sendRoute(build(msg, nc.nodeId, GATEWAY_ADDRESS, childSensorId, C_PRESENTATION, sensorType, enableAck).set(LIBRARY_VERSION));
}

void MySensor::sendSketchInfo(const char *name, const char *version, bool enableAck) {
	if (name != NULL) {
		sendRoute(build(msg, nc.nodeId, GATEWAY_ADDRESS, NODE_SENSOR_ID, C_INTERNAL, I_SKETCH_NAME, enableAck).set(name));
	}
    if (version != NULL) {
    	sendRoute(build(msg, nc.nodeId, GATEWAY_ADDRESS, NODE_SENSOR_ID, C_INTERNAL, I_SKETCH_VERSION, enableAck).set(version));
    }
}

void MySensor::request(uint8_t childSensorId, uint8_t variableType, uint8_t destination) {
	sendRoute(build(msg, nc.nodeId, destination, childSensorId, C_REQ, variableType, false));
}

void MySensor::requestTime(void (* _timeCallback)(unsigned long)) {
	timeCallback = _timeCallback;
	sendRoute(build(msg, nc.nodeId, GATEWAY_ADDRESS, NODE_SENSOR_ID, C_INTERNAL, I_TIME, false));
}


boolean MySensor::process() {
	boolean available = manager->available();

	//if (!available)
	///	return false;
	
	uint8_t len = sizeof(msg);
	uint8_t from;
	if (manager->recvfromAck((uint8_t *) &msg,  &len,  &from)) {
		// Add string termination, good if we later would want to print it.
		msg.data[mGetLength(msg)] = '\0';
		debug(PSTR("read: %d s=%d,c=%d,t=%d,pt=%d,l=%d:%s\n"),
				msg.sender, msg.sensor, mGetCommand(msg), msg.type, mGetPayloadType(msg), mGetLength(msg), msg.getString(convBuf));
	
	if(!(mGetVersion(msg) == PROTOCOL_VERSION)) {
		debug(PSTR("version mismatch\n"));
		return false;
	}

	uint8_t command = mGetCommand(msg);
	uint8_t type = msg.type;
	uint8_t sender = msg.sender;
	uint8_t last = msg.last;
	uint8_t destination = msg.destination;

	if (destination == nc.nodeId) {
		// Check if sender requests an ack back.
		if (mGetRequestAck(msg)) {
			// Copy message
			ack = msg;
			mSetRequestAck(ack,false); // Reply without ack flag (otherwise we would end up in an eternal loop)
			mSetAck(ack,true);
			ack.sender = nc.nodeId;
			ack.destination = msg.sender;
			sendRoute(ack);
		}

		if (command == C_INTERNAL) {
			if (sender == GATEWAY_ADDRESS) {
				bool isMetric;

				if (type == I_REBOOT) {
					wdt_enable(WDTO_15MS);
					for (;;);
				} else if (type == I_ID_RESPONSE) {
					if (nc.nodeId == AUTO) {
						nc.nodeId = msg.getByte();
						// Write id to EEPROM
						if (nc.nodeId == AUTO) {
							// sensor net gateway will return max id if all sensor id are taken
							debug(PSTR("full\n"));
							while (1); // Wait here. Nothing else we can do...
						} else {
							manager->setThisAddress(nc.nodeId);
							///RF24::openReadingPipe(CURRENT_NODE_PIPE, TO_ADDR(nc.nodeId));
							eeprom_write_byte((uint8_t*)EEPROM_NODE_ID_ADDRESS, nc.nodeId);
						}
						debug(PSTR("id=%d\n"), nc.nodeId);
					}
				} else if (type == I_CONFIG) {
					// Pick up configuration from controller (currently only metric/imperial)
					// and store it in eeprom if changed
					isMetric = msg.getByte() == 'M' ;
					if (cc.isMetric != isMetric) {
						cc.isMetric = isMetric;
						eeprom_write_byte((uint8_t*)EEPROM_CONTROLLER_CONFIG_ADDRESS, isMetric);
						//eeprom_write_block((const void*)&cc, (uint8_t*)EEPROM_CONTROLLER_CONFIG_ADDRESS, sizeof(ControllerConfig));
					}
				} else if (type == I_TIME) {
					if (timeCallback != NULL) {
						// Deliver time to callback
						timeCallback(msg.getULong());
					}
				}
				return false;
			}
		}
		// Call incoming message callback if available
		if (msgCallback != NULL) {
			msgCallback(msg);
		}
		// Return true if message was addressed for this node...
		return true;
	}
	}
	return false;
}


MyMessage& MySensor::getLastMessage() {
	return msg;
}


void MySensor::saveState(uint8_t pos, uint8_t value) {
	if (loadState(pos) != value) {
		eeprom_write_byte((uint8_t*)(EEPROM_LOCAL_CONFIG_ADDRESS+pos), value);
	}
}
uint8_t MySensor::loadState(uint8_t pos) {
	return eeprom_read_byte((uint8_t*)(EEPROM_LOCAL_CONFIG_ADDRESS+pos));
}


int MySensor::getInternalTemp(void)
{
  long result;
  // Read internal temp sensor against 1.1V reference
  ADMUX = _BV(REFS1) | _BV(REFS0) | _BV(MUX3);
  delay(20); // Wait until Vref has settled
  ADCSRA |= _BV(ADSC);
  while (bit_is_set(ADCSRA,ADSC));
  result = ADCL;
  result |= ADCH<<8;
  result = (result - 125) * 1075  + 500; // add 500 to round to nearest full degree

  return result/10000;
}

int continueTimer = true;
void wakeUp()	 //place to send the interrupts
{
	continueTimer = false;
}

void MySensor::internalSleep(unsigned long ms) {
	while (continueTimer && ms >= 8000) { LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF); ms -= 8000; }
	if (continueTimer && ms >= 4000)    { LowPower.powerDown(SLEEP_4S, ADC_OFF, BOD_OFF); ms -= 4000; }
	if (continueTimer && ms >= 2000)    { LowPower.powerDown(SLEEP_2S, ADC_OFF, BOD_OFF); ms -= 2000; }
	if (continueTimer && ms >= 1000)    { LowPower.powerDown(SLEEP_1S, ADC_OFF, BOD_OFF); ms -= 1000; }
	if (continueTimer && ms >= 500)     { LowPower.powerDown(SLEEP_500MS, ADC_OFF, BOD_OFF); ms -= 500; }
	if (continueTimer && ms >= 250)     { LowPower.powerDown(SLEEP_250MS, ADC_OFF, BOD_OFF); ms -= 250; }
	if (continueTimer && ms >= 125)     { LowPower.powerDown(SLEEP_120MS, ADC_OFF, BOD_OFF); ms -= 120; }
	if (continueTimer && ms >= 64)      { LowPower.powerDown(SLEEP_60MS, ADC_OFF, BOD_OFF); ms -= 60; }
	if (continueTimer && ms >= 32)      { LowPower.powerDown(SLEEP_30MS, ADC_OFF, BOD_OFF); ms -= 30; }
	if (continueTimer && ms >= 16)      { LowPower.powerDown(SLEEP_15Ms, ADC_OFF, BOD_OFF); ms -= 15; }
}

void MySensor::sleep(unsigned long ms) {
	// Let serial prints finish (debug, log etc)
	Serial.flush();
#ifdef DRH_RF69	
	driver->setModeIdle();
#endif
	continueTimer = true;
	internalSleep(ms);
}

bool MySensor::sleep(int interrupt, int mode, unsigned long  ms) {
	// Let serial prints finish (debug, log etc)
	bool pinTriggeredWakeup = true;
	Serial.flush();
#ifdef DRH_RF69	
	driver->setModeIdle();
#endif
	attachInterrupt(interrupt, wakeUp, mode); //Interrupt on pin 3 for any change in solar power
	if (ms>0) {
		continueTimer = true;
		sleep(ms);
		pinTriggeredWakeup = !continueTimer;
	} else {
		Serial.flush();
		LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
	}
	detachInterrupt(interrupt);
	return pinTriggeredWakeup;
}



#ifdef DEBUG
void MySensor::debugPrint(const char *fmt, ... ) {
	char fmtBuffer[300];
	if (isGateway) {
		// prepend debug message to be handled correctly by gw (C_INTERNAL, I_LOG_MESSAGE)
		snprintf_P(fmtBuffer, 299, PSTR("0;0;%d;0;%d;"), C_INTERNAL, I_LOG_MESSAGE);
		Serial.print(fmtBuffer);
	}
	va_list args;
	va_start (args, fmt );
	va_end (args);
	if (isGateway) {
		// Truncate message if this is gateway node
		vsnprintf_P(fmtBuffer, 60, fmt, args);
		fmtBuffer[59] = '\n';
		fmtBuffer[60] = '\0';
	} else {
		vsnprintf_P(fmtBuffer, 299, fmt, args);
	}
	va_end (args);
	Serial.print(fmtBuffer);
	Serial.flush();

	///Serial.write(freeRam());
}
#endif


#ifdef DEBUG
int MySensor::freeRam (void) {
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}
#endif
