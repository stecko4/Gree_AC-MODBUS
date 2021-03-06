/*
Program testowy odczytuje wszystkie wartości z klimatyzacji Gree poprzez Modbus.
Celem jest weryfikacja adresów modbus
*/


#include <Arduino.h>

#include "ModbusMaster.h" //https://github.com/4-20ma/ModbusMaster
#include <HardwareSerial.h>
#define Slave_ID 1				// Numer ID urządzenia z którym się komunikujemy
ModbusMaster node;				// instantiate ModbusMaster object
//HardwareSerial Serial1(1);
//HardwareSerial Serial2(2); 	// there is no any sense to define since it already defined in HardwareSerial.h 
#define RXD2 16 				// RX pin do odbierania danych
#define TXD2 17 				// TX do wysyłania danych
String ErrorCode;				// Zmienna przechoduje kody błędów z magistrali MODBUS

void preTransmission()
{
  digitalWrite(RXD2, 1);
  digitalWrite(TXD2, 1);
}

void postTransmission()
{
  digitalWrite(RXD2, 0);
  digitalWrite(TXD2, 0);
}

// Zwraca kody błędów z magistrali MODBUS
bool getResultMsg(ModbusMaster *node, uint8_t result) 
{
	ErrorCode="";
	switch (result) 
	{
		case node->ku8MBSuccess:
			return true;
			break;
		case node->ku8MBIllegalFunction:
			ErrorCode += "Illegal Function";
			break;
		case node->ku8MBIllegalDataAddress:
			ErrorCode += "Illegal Data Address";
			break;
		case node->ku8MBIllegalDataValue:
			ErrorCode += "Illegal Data Value";
			break;
		case node->ku8MBSlaveDeviceFailure:
			ErrorCode += "Slave Device Failure";
			break;
		case node->ku8MBInvalidSlaveID:
			ErrorCode += "Invalid Slave ID";
			break;
		case node->ku8MBInvalidFunction:
			ErrorCode += "Invalid Function";
			break;
		case node->ku8MBResponseTimedOut:
			ErrorCode += "Response Timed Out";
			break;
		case node->ku8MBInvalidCRC:
			ErrorCode += "Invalid CRC";
			break;
		default:
			ErrorCode += "Unknown error: " + String(result);
			break;
	}
	return false;
}

void ReadRegisters(){
		uint8_t result;
		int i;
		node.clearResponseBuffer();
		// Odczyt wszystkich rejestrów z zakresu adresów od 100 do 122 (Data of 1# indor unit)
		Serial.println("Odczyt wszystkich rejestrów z zakresu adresów od 100 do 122 (Data of 1# indor unit)");
		for (i = 40100; i <= 40122; i++)		// indor unit
		{	
			result = node.readHoldingRegisters(i, 1);
			if (getResultMsg(&node, result)) 
			{
				Serial.print("Register ");
				Serial.print(i);
				Serial.print(" = ");
				Serial.print('\t');      // prints a tab character
				Serial.println(node.getResponseBuffer(0));
			}
			else
			{
				Serial.print("Error reading Register  ");
				Serial.print(i);
				Serial.print("; ERROR = ");
				Serial.println(ErrorCode);
			}
			node.clearResponseBuffer();
		}
		Serial.print('\n');
		// Odczyt wszystkich rejestrów z zakresu adresów od 3301 do 3308  (Data of 1# indor unit)
		Serial.println("Odczyt wszystkich rejestrów z zakresu adresów od 3301 do 3308 (Data of 1# outdor unit)");
		for (i = 43301; i <= 43308; i++)		// outdor unit
		{	
			result = node.readHoldingRegisters(i, 1);
			if (getResultMsg(&node, result)) 
			{
				Serial.print("Register ");
				Serial.print(i);
				Serial.print(" = ");
				Serial.print('\t');      // prints a tab character
				Serial.println(node.getResponseBuffer(0));
			}
			else
			{
				Serial.print("Error reading Register  ");
				Serial.print(i);
				Serial.print("; ERROR = ");
				Serial.println(ErrorCode);
			}
			node.clearResponseBuffer();
		}
		Serial.print('\n');
}

void ReadCoils(){
		uint8_t result;
		int i;
		
  // read 17 coils starting at NANO_FLAG(0) to RX buffer
  // bits 15..0 are available via nanoLC.getResponseBuffer(0)
  // bit 16 is available via zero-padded nanoLC.getResponseBuffer(1)
  //node.readCoils(i, 17);


		node.clearResponseBuffer();		// Clear the response buffer. This function acts like a flush of the response buffer and delete all previous information. It is recommended to use this function before starting a new reading process. 
		// READ ONLY
		// Odczyt wszystkich coilsów z zakresu adresów od 87 do 102 (Outdor uni1 1~16, with or without)
		Serial.println("Odczyt wszystkich coilsów z zakresu adresów od 87 do 102 (Outdor uni1 1~16, 1: with or 0: without)");
		Serial.println("READ ONLY");
		for (i = 30100; i <= 30102; i++)		// indor unit
		{	
			result = node.readCoils(i, 1);
			if (getResultMsg(&node, result)) 
			{
				Serial.print("Coil ");
				Serial.print(i);
				Serial.print(" = ");
				Serial.print('\t');      // prints a tab character
				Serial.println(node.getResponseBuffer(0));
			}
			else
			{
				Serial.print("Error reading Coil  ");
				Serial.print(i);
				Serial.print("; ERROR = ");
				Serial.println(ErrorCode);
			}
			node.clearResponseBuffer();
		}
		// Serial.print('\n'); will print '\n', which is a newline character (sometimes called a "line feed").
		// Serial.println(); will print '\r' and '\n', which is a carriage return character followed by a newline
		Serial.print('\n');

		// READ ONLY
		// Odczyt wszystkich coilsów z zakresu adresów od 119 do 247  (Indor unit 1~16, with or without)
		Serial.println("Odczyt wszystkich coilsów z zakresu adresów od 119 do 247  (Indor unit 1~16, 1: with or 0: without)");
		Serial.println("READ ONLY");
		for (i = 30119; i <= 30247; i++)		// outdor unit
		{	
			result = node.readCoils(i, 1);
			if (getResultMsg(&node, result)) 
			{
				Serial.print("Coil ");
				Serial.print(i);
				Serial.print(" = ");
				Serial.print('\t');      // prints a tab character
				Serial.println(node.getResponseBuffer(0));
			}
			else
			{
				Serial.print("Error reading Coil  ");
				Serial.print(i);
				Serial.print("; ERROR = ");
				Serial.println(ErrorCode);
			}
			node.clearResponseBuffer();
		}
		Serial.print('\n');

		// READ & WRITE
		// Odczyt wszystkich coilsów z zakresu adresów od 247 do 262  (Outdor unit 1~16, remote emergency stop signal, 0:off or 1: on)
		Serial.println("Odczyt wszystkich coilsów z zakresu adresów od 247 do 262  (Outdor unit 1~16, remote emergency stop signal, 0:off or 1: on)");
		Serial.println("READ & WRITE");
		for (i = 30247; i <= 30262; i++)		// outdor unit
		{	
			result = node.readCoils(i, 1);
			if (getResultMsg(&node, result)) 
			{
				Serial.print("Coil ");
				Serial.print(i);
				Serial.print(" = ");
				Serial.print('\t');      // prints a tab character
				Serial.println(node.getResponseBuffer(0));
			}
			else
			{
				Serial.print("Error reading Coil  ");
				Serial.print(i);
				Serial.print("; ERROR = ");
				Serial.println(ErrorCode);
			}
			node.clearResponseBuffer();
		}
		Serial.print('\n');

		// WRITE ONLY
		// Odczyt wszystkich coilsów z zakresu adresów od 247 do 262  (Outdor unit 1~16, remote emergency stop signal, 0:off or 1: on)
		Serial.println("Odczyt wszystkich coilsów z zakresu adresów od 247 do 262  (Outdor unit 1~16, remote emergency stop signal, 0:off or 1: on)");
		Serial.println("WRITE ONLY");
		for (i = 30279; i <= 30280; i++)		// outdor unit
		{	
			result = node.readCoils(i, 1);
			if (getResultMsg(&node, result)) 
			{
				Serial.print("Coil ");
				Serial.print(i);
				Serial.print(" = ");
				Serial.print('\t');      // prints a tab character
				Serial.println(node.getResponseBuffer(0));
			}
			else
			{
				Serial.print("Error reading Coil  ");
				Serial.print(i);
				Serial.print("; ERROR = ");
				Serial.println(ErrorCode);
			}
			node.clearResponseBuffer();
		}
		Serial.print('\n');

		// READ & WRITE
		// Odczyt wszystkich coilsów z zakresu adresów od 287 do 307  (Data of 1# indor unit)
		Serial.println("Odczyt wszystkich coilsów z zakresu adresów od 287 do 307  (Data of 1# indor unit)");
		Serial.println("READ & WRITE");
		for (i = 30287; i <= 30307; i++)		// outdor unit
		{	
			result = node.readCoils(i, 1);
			if (getResultMsg(&node, result)) 
			{
				Serial.print("Coil ");
				Serial.print(i);
				Serial.print(" = ");
				Serial.print('\t');      // prints a tab character
				Serial.println(node.getResponseBuffer(0));
			}
			else
			{
				Serial.print("Error reading Coil  ");
				Serial.print(i);
				Serial.print("; ERROR = ");
				Serial.println(ErrorCode);
			}
			node.clearResponseBuffer();
		}
		Serial.print('\n');

		// READ ONLY
		// Odczyt wszystkich coilsów z zakresu adresów od 314 do 318  (Data of 1# indor unit)
		Serial.println("Odczyt wszystkich coilsów z zakresu adresów od 314 do 318  (Data of 1# indor unit)");
		Serial.println("READ ONLY");
		for (i = 30314; i <= 30318; i++)		// outdor unit
		{	
			result = node.readCoils(i, 1);
			if (getResultMsg(&node, result)) 
			{
				Serial.print("Coil ");
				Serial.print(i);
				Serial.print(" = ");
				Serial.print('\t');      // prints a tab character
				Serial.println(node.getResponseBuffer(0));
			}
			else
			{
				Serial.print("Error reading Coil  ");
				Serial.print(i);
				Serial.print("; ERROR = ");
				Serial.println(ErrorCode);
			}
			node.clearResponseBuffer();
		}
		Serial.print('\n');


		// READ ONLY
		// Odczyt wszystkich coilsów z zakresu adresów od 8487 do 8494  (Data of 1# outdor unit)
		Serial.println("Odczyt wszystkich coilsów z zakresu adresów od 8487 do 8494  (Data of 1# outdor unit)");
		Serial.println("READ ONLY");
		for (i = 38487; i <= 38494; i++)		// outdor unit
		{	
			result = node.readCoils(i, 1);
			if (getResultMsg(&node, result)) 
			{
				Serial.print("Coil ");
				Serial.print(i);
				Serial.print(" = ");
				Serial.print('\t');      // prints a tab character
				Serial.println(node.getResponseBuffer(0));
			}
			else
			{
				Serial.print("Error reading Coil  ");
				Serial.print(i);
				Serial.print("; ERROR = ");
				Serial.println(ErrorCode);
			}
			node.clearResponseBuffer();
		}
		Serial.print('\n');

		// READ & WRITE
		// Odczyt wszystkich coilsów z zakresu adresów od 9247 do 9254  (D0 area)
		Serial.println("Odczyt wszystkich coilsów z zakresu adresów od 9247 do 9254  (D0 area)");
		Serial.println("READ & WRITE");
		for (i = 39247; i <= 39254; i++)		// outdor unit
		{	
			result = node.readCoils(i, 1);
			if (getResultMsg(&node, result)) 
			{
				Serial.print("Coil ");
				Serial.print(i);
				Serial.print(" = ");
				Serial.print('\t');      // prints a tab character
				Serial.println(node.getResponseBuffer(0));
			}
			else
			{
				Serial.print("Error reading Coil  ");
				Serial.print(i);
				Serial.print("; ERROR = ");
				Serial.println(ErrorCode);
			}
			node.clearResponseBuffer();
		}
		Serial.print('\n');

		// READ ONLY
		// Odczyt wszystkich coilsów z zakresu adresów od 9255 do 9262  (D1 area)
		Serial.println("Odczyt wszystkich coilsów z zakresu adresów od 9255 do 9262  (D1 area)");
		Serial.println("READ ONLY");
		for (i = 39255; i <= 39262; i++)		// outdor unit
		{	
			result = node.readCoils(i, 1);
			if (getResultMsg(&node, result)) 
			{
				Serial.print("Coil ");
				Serial.print(i);
				Serial.print(" = ");
				Serial.print('\t');      // prints a tab character
				Serial.println(node.getResponseBuffer(0));
			}
			else
			{
				Serial.print("Error reading Coil  ");
				Serial.print(i);
				Serial.print("; ERROR = ");
				Serial.println(ErrorCode);
			}
			node.clearResponseBuffer();
		}
		Serial.print('\n');

}

void setup() {
	// Init in receive mode
	pinMode(RXD2, OUTPUT);			// Deklaradja pinu dla MODBUS
	pinMode(TXD2, OUTPUT);			// Deklaradja pinu dla MODBUS
	digitalWrite(RXD2, 0);			// Ustawienie punu dla dla MODBUS do stanu 0
	digitalWrite(TXD2, 0);			// Ustawienie punu dla dla MODBUS do stanu 0

	//MODBUS
	// Modbus communication runs at 9600 baud
	Serial.begin(9600, SERIAL_8N1);						// Tego portu szeregowego używamy do wypluwania danych na komputer
	Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);		// Tego portu szeregowego używamy do komunikazni protokołem MODBUS
	while (!Serial) {
		; // wait for serial port to connect. Needed for native USB 
	}
	while (!Serial2) {
		; // wait for serial2 port to connect. Needed for native USB
	}
	delay(500);
	// Modbus slave ID 1
	node.begin(Slave_ID, Serial2);
	// Callbacks allow us to configure the RS485 transceiver correctly
	node.preTransmission(preTransmission);
	node.postTransmission(postTransmission);
	node.readHoldingRegisters(40100, 1);		//Pierwsze wywołanie zawsze zwracało błąd więc dywołujemy ale go nie odczytujemy
	//node.idle(yield);

	ReadRegisters();
	ReadCoils();
}

void loop() {
	delay(100);		// Nie potrzebujemy tutaj pętli, program ma się wykonać raz
}