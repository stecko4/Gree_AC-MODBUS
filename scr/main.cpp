/*
DeviceAddress sensor1 = {  0x28, 0x51, 0x80, 0x48, 0xF6, 0xFB, 0x3C, 0xCB };    // Adres czujnika #1
DeviceAddress sensor2 = {  0x28, 0x93, 0xC6, 0x48, 0xF6, 0x2E, 0x3C, 0xE8 };    // Adres czujnika #2
DeviceAddress sensor3 = {  0x28, 0x28, 0x07, 0x48, 0xF6, 0xA3, 0x3C, 0x5E };    // Adres czujnika #3
DeviceAddress sensor4 = {  0x28, 0xD0, 0xFB, 0x48, 0xF6, 0x71, 0x3C, 0xC3 };    // Adres czujnika #4
DeviceAddress sensor5 = {  0x28, 0x88, 0x40, 0x75, 0xD0, 0x01, 0x3C, 0x55 };    // Adres czujnika #5
DeviceAddress sensor6 = {  0x28, 0x74, 0xCE, 0x48, 0xF6, 0xB0, 0x3C, 0x65 };    // Adres czujnika #6
DeviceAddress sensor7 = {  0x28, 0x1C, 0x10, 0x48, 0xF6, 0x15, 0x3C, 0xC0 };    // Adres czujnika #7
DeviceAddress sensor8 = {  0x28, 0xDE, 0xF6, 0x48, 0xF6, 0x58, 0x3C, 0x4F };    // Adres czujnika #8
DeviceAddress sensor9 = {  0x28, 0x72, 0x9F, 0x48, 0xF6, 0xD4, 0x3C, 0xEE };    // Adres czujnika #9
*/

#include <Arduino.h>

//Biblioteki potrzebne dla AutoConnect
#include <WiFi.h>				// Replace 'ESP8266WiFi.h' with 'WiFi.h. for ESP32
#include <WebServer.h>			// Replace 'ESP8266WebServer.h'with 'WebServer.h' for ESP32
#include <AutoConnect.h>
WebServer			Server;		// Replace 'ESP8266WebServer' with 'WebServer' for ESP32
AutoConnect			Portal(Server);
AutoConnectConfig	Config;		// Enable autoReconnect supported on v0.9.4


//#define BLYNK_DEBUG				// Optional, this enables lots of prints
/* Comment this out to disable prints and save space */
//#define BLYNK_PRINT Serial
//#include <WiFi.h> //alreadydeclared for AutoConnect
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>		// Replace 'BlynkSimpleEsp8266' with 'BlynkSimpleEsp32.h. for ESP32
#include <WidgetRTC.h>
#include <SimpleTimer.h>			// https://github.com/jfturcot/SimpleTimer
#include <TimeLib.h>				// https://github.com/PaulStoffregen/Time
WidgetTerminal	terminal(V40);		// Attach virtual serial terminal to Virtual Pin V40
WidgetLED		LED_AC_OnOff(V50);	// Inicjacja diody LED dla stanu A/C ON lub OFF
WidgetLED		LED_Duct(V51);		// Inicjacja diody LED dla stanu kanału wentylacyjnego OPEN lub CLOSED
WidgetRTC		rtc;				// Inicjacja widgetu zegara czasu rzeczywistego RTC
SimpleTimer		Timer;				// Timer do sprawdzania połaczenia z BLYNKiem (co 30s)
int				timerIDReset = -1;	//Przetrzymuje ID Timera https://desire.giesecke.tk/index.php/2018/01/30/change-global-variables-from-isr/
#define BLYNK_GREEN			"#23C48E"
#define BLYNK_BLUE			"#04C0F8"
#define BLYNK_YELLOW		"#ED9D00"
#define BLYNK_RED			"#D3435C"
#define BLYNK_DARK_BLUE		"#5F7CD8"


//Definicja bibliotegi MODBUS
/*!
  We're using a MAX485-compatible RS485 Transceiver.
  Rx/Tx is hooked up to the hardware serial port at 'Serial'.
  The Data Enable and Receiver Enable pins are hooked up as follows:
*/
/*
#include <ModbusMaster.h>
#define MAX485_DE      3
#define MAX485_RE_NEG  2
ModbusMaster node;					// instantiate ModbusMaster object
String tmpstr2;						// Zmienna przechoduje kody błędów z magistrali MODBUS
*/

#include "ModbusMaster.h" //https://github.com/4-20ma/ModbusMaster
#include <HardwareSerial.h>
#define Slave_ID 1				// Numer ID urządzenia z którym się komunikujemy
ModbusMaster node;				// instantiate ModbusMaster object
//HardwareSerial Serial1(1);
//HardwareSerial Serial2(2); 	// there is no any sense to define since it already defined in HardwareSerial.h 
#define RXD2 16 				// RX pin do odbierania danych
#define TXD2 17 				// TX do wysyłania danych
String tmpstr2;


//STAŁE
//const char	ssid[]			= "Your SSID";							// Not required with AutoConnect
//const char	pass[]			= "Router password";					// Not required with AutoConnect
const char		auth[]			= "YwyJKqXF5e00nYj__w66tyJtq2tBuCBa";	// Blynk token for Gree A/C
const char		server[] 		= "stecko.duckdns.org";  				// IP for your Local Server or DNS server addres stecko.duckdns.org
const int		port 			= 8080;									// Port na którym jest serwer Blykn

// Definicja pinów sterujących przekaźnikami
#define			DuctRelay	 	  22									// Numer pinu dla przekaźnika sterującego otwieranien/zamykaniem kanału wentylacyjnego [HIGH / LOW]

float	IndorTemp		= 0;		// Indor temp from indor unit of Gree A/C
int 	FanSpeed		= 1;		// Fan speed setting from indor unit of Gree A/C, default 1 = auto
int		OnOff			= 0x55;		// Gree air conditioning status, ON = 0x55, OFF = 0xAA 
int		Tryb_Sterownika	= 0;		// Word 103, strona 13
float	SetTempActual	= 60;		// Temperatura według której załączany jest wentylator
boolean	Vent_Status		= false;	// Domyślnie ventylacja wyłączone		(false = OFF, true = ON)
boolean	AC_Status		= false;	// Domyślnie A/C wyłączona				(false = OFF, true = ON)
boolean	Duct_Status		= false;	// Domyślnie klapa wentylacji zamknięta	(false = CLOSED, true = OPENED)
int		StartHour		= 0;		// Godzina włączenia wentylacji
int 	StartMinute		= 0;		// Minuta włączenia wentylacji
int		StopHour		= 0;		// Godzina wyłączenia wentylacji
int 	StopMinute		= 0;		// Minuta wyłączenia wentylacji
boolean	DayOfWeek[7]	= {false,false,false,false,false,false,false}; // Dni tygodznia kiedy załączy się wentylacja
int		dzien			= 1;		// day of the week (1-7), Sunday is day 1
int		godzina			= 0;		// the hour now (0-23)
int		minuta			= 0;		// the minute now (0-59)

//---------------------------------------------------------------------------------------------------------------------------------------------


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

// Soft restart sterownika
void RestartESP32()
{
	ESP.restart(); 	// Restartuje sterownik
}

//Informacja że połączono z serwerem Blynk, synchronizacja danych
BLYNK_CONNECTED()
{
	//Serial.println("Reconnected, syncing with cloud.");
	rtc.begin();
	//Blynk.syncAll();
	Blynk.syncVirtual(V45, V41, V51, V50, V11, V10, V13); // Synchronizacja tego co jest w aplikacji Blynk, tylno Virtual Pins które czymś sterują, nie te krure wysyłają dane do alpikacji
}

//Sprawdza czy połączone z serwerem Blynk
void blynkCheck()
{
	if (WiFi.status() == WL_CONNECTED)		//WL_CONNECTED: assigned when connected to a WiFi network
	{
		if (!Blynk.connected())
		{
			//Serial.println("WiFi OK, trying to connect to the Blynk server...");
			Blynk.connect();
		}
	}

	if (WiFi.status() == WL_NO_SSID_AVAIL)		//WL_NO_SSID_AVAIL: assigned when no SSID are available
	{
		Serial.println("No WiFi connection, offline mode.");
	}

	if (WiFi.status() == WL_IDLE_STATUS)		//WL_IDLE_STATUS is a temporary status assigned when WiFi.begin() is called and remains active until the number of attempts expires (resulting in WL_CONNECT_FAILED) or a connection is established (resulting in WL_CONNECTED)
	{
		Serial.println("WL_IDLE_STATUS: WiFi.begin() is called");
	}

	if (WiFi.status() == WL_SCAN_COMPLETED)		//WL_SCAN_COMPLETED: assigned when the scan networks is completed
	{
		Serial.println("WL_SCAN_COMPLETED: networks is completed");
	}

	if (WiFi.status() == WL_CONNECT_FAILED)		//WL_CONNECT_FAILED: assigned when the connection fails for all the attempts
	{
		Serial.println("WL_CONNECT_FAILED: connection fails for all the attempts");
	}

	if (WiFi.status() == WL_CONNECTION_LOST)	//WL_CONNECTION_LOST: assigned when the connection is lost
	{
		Serial.println("WL_CONNECTION_LOST: the connection is lost");
	}

	if (WiFi.status() == WL_DISCONNECTED)		//WL_DISCONNECTED: assigned when disconnected from a network
	{
		Serial.println("WL_DISCONNECTED: disconnected from a network");
	}
}

//Ustawienie trybów sterowania wentylacja
void TrybManAuto()
{
	dzien = weekday(now());					// day of the week (1-7), Sunday is day 1
	godzina = hour(now());					// the hour now  (0-23)
	minuta =  minute(now());				// the minute now (0-59)

	if (Tryb_Sterownika == 0 && DayOfWeek[dzien-1] && godzina * 60 + minuta >= StartHour * 60 + StartMinute && godzina * 60 + minuta < StopHour * 60 + StopMinute && StartHour != -1 && StartMinute != -1 && StopHour != -1 && StopMinute != -1)		//Tryb AUTO ON
	{	// Tryb AUTO
		if(Vent_Status == false)
		{
			Vent_Status = true; 
			LED_Duct.on();					// Widget dioda zaświecona
			digitalWrite(DuctRelay, LOW);	// Kanał wentylacyjny otwarty
		}
	}
	else if (Tryb_Sterownika == 0 && (!DayOfWeek[dzien-1] || godzina * 60 + minuta < StartHour * 60 + StartMinute || godzina * 60 + minuta >= StopHour * 60 + StopMinute || StartHour == -1 || StartMinute == -1 || StopHour == -1 || StopMinute == -1))	//Tryb AUTO OFF
	{	//Tryb AUTO
		if(Vent_Status == true)
		{
			Vent_Status = false; 
			LED_Duct.off();					// Widget dioda zgaszona
			digitalWrite(DuctRelay, HIGH);	// Lampa wyłączona
		}		
	}
	else if (Tryb_Sterownika == 1)
	{	// Tryb ON
		if(Vent_Status == false)
		{
			Vent_Status = true;
			LED_Duct.on();					// Widget dioda zaświecona
			digitalWrite(DuctRelay, LOW);	// Lampa włączona
		}
	}
	else if (Tryb_Sterownika == 2)
	{	// Tryb OFF
		if(Vent_Status == true)
		{
			Vent_Status = false;
			LED_Duct.off();					// Widget dioda zgaszona
			digitalWrite(DuctRelay, HIGH);	// Wentylator wyłączony
		}
	}
}

//Zwraca siłę sygnału WiFi sieci do której jest podłączony w %. REF: https://www.adriangranados.com/blog/dbm-to-percent-conversion
int WiFi_Strength (long Signal)
{
	return constrain(round((-0.0154*Signal*Signal)-(0.3794*Signal)+98.182), 0, 100);
}

//signal strength levels https://www.netspotapp.com/what-is-rssi-level.html
String WiFi_levels(long Signal)
{
	if (Signal >= -50)
	{
		return "Excellent";
	}
	else if (Signal < -50 && Signal >= -60)
	{
		return "Very good";
	}
	else if (Signal < -60 && Signal >= -70)
	{
		return "Good";
	}
	else if (Signal < -70 && Signal >= -80)
	{
		return "Low";
	}
	else if (Signal < -80 && Signal >= -90)
	{
		return "Very low";
	}
	return "Unusable";
}

// Zwraca kody błędów z magistrali MODBUS
bool getResultMsg(ModbusMaster *node, uint8_t result) 
{
	tmpstr2="";
	switch (result) 
	{
		case node->ku8MBSuccess:
			return true;
			break;
		case node->ku8MBIllegalFunction:
			tmpstr2 += "Illegal Function";
			break;
		case node->ku8MBIllegalDataAddress:
			tmpstr2 += "Illegal Data Address";
			break;
		case node->ku8MBIllegalDataValue:
			tmpstr2 += "Illegal Data Value";
			break;
		case node->ku8MBSlaveDeviceFailure:
			tmpstr2 += "Slave Device Failure";
			break;
		case node->ku8MBInvalidSlaveID:
			tmpstr2 += "Invalid Slave ID";
			break;
		case node->ku8MBInvalidFunction:
			tmpstr2 += "Invalid Function";
			break;
		case node->ku8MBResponseTimedOut:
			tmpstr2 += "Response Timed Out";
			break;
		case node->ku8MBInvalidCRC:
			tmpstr2 += "Invalid CRC";
			break;
		default:
			tmpstr2 += "Unknown error: " + String(result);
			break;
	}
	return false;
}

// Cykliczne odczytywanie wartości z klimatyzacji poprzez moduł MODBUS
void Periodic_read()
{		node.clearResponseBuffer();		// Clear the response buffer. This function acts like a flush of the response buffer and delete all previous information. It is recommended to use this function before starting a new reading process. 
		uint8_t result = node.readHoldingRegisters(115, 1);
		if (getResultMsg(&node, result)) 
		{
			IndorTemp = node.getResponseBuffer(0) * 0.1;
		}
		node.clearResponseBuffer();		// Clear the response buffer. This function acts like a flush of the response buffer and delete all previous information. It is recommended to use this function before starting a new reading process. 
		result = node.readHoldingRegisters(101, 1);
		if (getResultMsg(&node, result)) 
		{
			OnOff = node.getResponseBuffer(0);
			if (OnOff == 0x55)	// Gree air conditioning status, ON = 0x55, OFF = 0xAA 
			{
				LED_AC_OnOff.on();					// Widget dioda zaświecona
			}
			else
			{
				LED_AC_OnOff.off();					// Widget dioda zaświecona
			}

		}
}

//Wysyłanie danych na serwer Blynka
void Wyslij_Dane()
{
	Blynk.virtualWrite(V12, IndorTemp);			// Indor temp from indor unit of Gree A/C
}

//Temperature setting
BLYNK_WRITE(V11)
{
	int temp = param.asFloat() * 10;
	node.writeSingleRegister(103,temp);	//Temperature setting
}

//Fan speed setting
BLYNK_WRITE(V13)
{
	int FanSpeed = param.asInt();
	node.writeSingleRegister(104,FanSpeed);	//Fan speed setting
		switch (param.asInt())
	{
		case 0:						//Invalid
			node.writeSingleRegister(104,FanSpeed);	//Fan speed setting
			Blynk.virtualWrite(V14, "INVALID");		// Wysłanie do aplikacji blynk nazwy ustawienia
			break;
		case 1:						//Auto fan speed
			node.writeSingleRegister(104,FanSpeed);	//Fan speed setting
			Blynk.virtualWrite(V14, "AUTO");		// Wysłanie do aplikacji blynk nazwy ustawienia
			break;
		case 2:						//Low fan speed
			node.writeSingleRegister(104,FanSpeed);	//Fan speed setting
			Blynk.virtualWrite(V14, "LOW");		// Wysłanie do aplikacji blynk nazwy ustawienia
			break;
		case 3:						//medium-low fan speed
			node.writeSingleRegister(104,FanSpeed);	//Fan speed setting
			Blynk.virtualWrite(V14, "MEDIUM-LOW");		// Wysłanie do aplikacji blynk nazwy ustawienia
			break;
		case 4:						//medium fan speed
			node.writeSingleRegister(104,FanSpeed);	//Fan speed setting
			Blynk.virtualWrite(V14, "MEDIUM");		// Wysłanie do aplikacji blynk nazwy ustawienia
			break;
		case 5:						//medium-high fan speed
			node.writeSingleRegister(104,FanSpeed);	//Fan speed setting
			Blynk.virtualWrite(V14, "MEDIUM_HIGH");		// Wysłanie do aplikacji blynk nazwy ustawienia
			break;
		case 6:						//high fan speed
			node.writeSingleRegister(104,FanSpeed);	//Fan speed setting
			Blynk.virtualWrite(V14, "HIGH");		// Wysłanie do aplikacji blynk nazwy ustawienia
			break;
		case 7:						//Turbo
			node.writeSingleRegister(104,FanSpeed);	//Fan speed setting
			Blynk.virtualWrite(V14, "TURBO");		// Wysłanie do aplikacji blynk nazwy ustawienia
			break;

			default:				//Wartość domyślna 4: "automatic"
			node.writeSingleRegister(104,1);	//Fan speed setting
			Blynk.virtualWrite(V14, "AUTO");		// Wysłanie do aplikacji blynk nazwy ustawienia
	}
}

//Operating mode setting
BLYNK_WRITE(V10)
{
	switch (param.asInt())
	{
		case 1:						//Cooling
			Tryb_Sterownika = 1;
			node.writeSingleRegister(101,0xAA);	//On
			node.writeSingleRegister(102,1);	//Cooling
			break;
		case 2:						//Heating
			Tryb_Sterownika = 2;
			node.writeSingleRegister(101,0xAA);	//On
			node.writeSingleRegister(102,4);	//Heating
			break;
		case 3:						//Fan
			Tryb_Sterownika = 3;
			node.writeSingleRegister(101,0xAA);	//On
			node.writeSingleRegister(102,3);	//Fan
			break;
		case 4:						//Automatic
			Tryb_Sterownika = 4;
			node.writeSingleRegister(101,0xAA);	//On
			node.writeSingleRegister(102,5);	//Automatic
			break;
		case 5:						//Off
			Tryb_Sterownika = 5;
			node.writeSingleRegister(101,0x55);	//Off
			break;
			default:				//Wartość domyślna 4: "automatic"
			Tryb_Sterownika = 4;
	}

}

//Obsługa terminala REFRESH
BLYNK_WRITE(V0)
{
	if (param.asInt() == 1)
	{
		terminal.clear();
		//https://forum.arduino.cc/t/modbus-to-esp32-modbus-library-in-arduino/586163/2
		node.clearResponseBuffer();		// Clear the response buffer. This function acts like a flush of the response buffer and delete all previous information. It is recommended to use this function before starting a new reading process. 
		uint8_t result = node.readHoldingRegisters(100, 8);
		if (getResultMsg(&node, result)) 
		{
			terminal.print("No. of indoor unit: ");
			terminal.println(node.getResponseBuffer(0));

			terminal.print("On/off: ");
			terminal.println(node.getResponseBuffer(1));

			terminal.print("Operation mode setting: ");
			terminal.println(node.getResponseBuffer(2));

			terminal.print("Temperature setting: ");
			terminal.print(node.getResponseBuffer(3) * 0.1,1);
			terminal.println("°C");

			terminal.print("Fan speed setting: ");
			terminal.println(node.getResponseBuffer(4));

			terminal.print("Lower limit of cooling temp.: ");
			terminal.print(node.getResponseBuffer(5) * 0.1,1);
			terminal.println("°C");

			terminal.print("Upper limit of heating temp.: ");
			terminal.print(node.getResponseBuffer(6) * 0.1,1);
			terminal.println("°C");

			terminal.print("Lower limit of dehumidifying temp.: ");
			terminal.print(node.getResponseBuffer(7) * 0.1,1);
			terminal.println("°C");

			terminal.flush();
		}
		else
		{
			terminal.print(tmpstr2);	// Wyrzuci jaki błąd napotkał w transmisji MODBUS
			terminal.flush();
		}
		node.clearResponseBuffer();		// Clear the response buffer. This function acts like a flush of the response buffer and delete all previous information. It is recommended to use this function before starting a new reading process. 
		result = node.readHoldingRegisters(115, 3);
		if (getResultMsg(&node, result)) 
		{
			terminal.print("Indoor ambient temperature: ");
			terminal.print(node.getResponseBuffer(0) * 0.1,1);
			terminal.println("°C");

			terminal.print("Gate control status: ");
			terminal.println(node.getResponseBuffer(1));

			terminal.print("No. of outdor unit: ");
			terminal.println(node.getResponseBuffer(2));
			terminal.flush();
		}
		node.clearResponseBuffer();		// Clear the response buffer. This function acts like a flush of the response buffer and delete all previous information. It is recommended to use this function before starting a new reading process. 
		result = node.readHoldingRegisters(122, 1);
		if (getResultMsg(&node, result)) 
		{
			terminal.print("Rated capacity of indoor unit: ");
			terminal.println(node.getResponseBuffer(0));
			terminal.flush();
		}
	}
}

//Obsługa terminala CLEAR
BLYNK_WRITE(V39)
{
	terminal.clear();
}

//Sterowanie wentylacją (AUTO, ON, OFF)
BLYNK_WRITE(V41)
{
	switch (param.asInt())
	{
		case 1:						//AUTO
			Tryb_Sterownika = 0;
			Blynk.setProperty(V45,"color","#ffffff");	// Kolor biały
			break;
		case 2:						//ON
			Tryb_Sterownika = 1;
			Blynk.setProperty(V45,"color","#4f4f4f");	// Kolor szary
			break;
		case 3:						//OFF
			Tryb_Sterownika = 2;
			Blynk.setProperty(V45,"color","#4f4f4f");	// Kolor szary
			break;
			default:				//Wartość domyślna AUTO
			Tryb_Sterownika = 0;
	}
	TrybManAuto();
}

//Obsługa timera Start i Stop (Time Input Widget)
BLYNK_WRITE(V45)
{
	TimeInputParam t(param);
	// Process start time
	if (t.hasStartTime())
	{
		StartHour = t.getStartHour();
		StartMinute = t.getStartMinute();
	}
	else
	{
		StartHour = -1;
		StartMinute = -1;
	}
	

	// Process stop time
	if (t.hasStopTime())
	{
		StopHour = t.getStopHour();
		StopMinute = t.getStopMinute();
	}
	else
	{
		StopHour = -1;
		StopMinute = -1;
	}

	if (StartHour * 60 + StartMinute >= StopHour * 60 + StopMinute && StartHour != -1 && StopHour  != -1)
	{
		Blynk.notify("Stop time myst be after start time. please correct!");
		Blynk.virtualWrite(V45, (StartHour*60+StartMinute)*60, -1, "Europe/Warsaw");	// Kasowanie wpisu Stop Time w aplikacji Blynk
	}

	// Process weekdays (1. Mon, 2. Tue, 3. Wed, ...) but I need (1. Sat 2. Mon, 3. Tue, 4. Wed, ...)
	if (t.isWeekdaySelected(7))
	{
		DayOfWeek[0] = true;
	}
	else
	{
		DayOfWeek[0] = false;
	}
	for (int i = 1; i <= 6; i++)
	{
		if (t.isWeekdaySelected(i))
		{
			DayOfWeek[i] = true;
		}
		else
		{
			DayOfWeek[i] = false;
		}
		
	}
}

//Uruchamia po kolei wszystkie niezbędne funcje
void MainFunction()
{	
		TrybManAuto();		// Sterowanie wentylacją
		Periodic_read();	// Cykliczne odczytywanie wartości z klimatyzacji poprzez moduł MODBUS
		Wyslij_Dane();		// Indor temp from indor unit of Gree A/C
}

// Definicja strony z wersją firmwaru. Definitions of AutoConnectAux page
static const char Version[] PROGMEM = R"(
{
	"title": "Version",
	"uri": "/page",
	"menu": true,
	"element": [
	{
		"name": "cap",
		"type": "ACText",
		"value": "<b>Gree A/C controller</b><br>Version: 0.0.1_BETA<br>Date: 10.04.2022"
	}
	]
}
)";

void setup()
{

	pinMode(RXD2, OUTPUT);									// Deklaradja pinu dla MODBUS
	pinMode(TXD2, OUTPUT);									// Deklaradja pinu dla MODBUS
	// Init in receive mode
	digitalWrite(RXD2, 0);									// Ustawienie punu dla dla MODBUS do stanu 0
	digitalWrite(TXD2, 0);									// Ustawienie punu dla dla MODBUS do stanu 0
	pinMode(DuctRelay, OUTPUT);								// Deklaracja pinu dla przekaźnika klapy wentylacji
	digitalWrite(DuctRelay, LOW);							// Ustawienie pinu w stan niski, klapa zamknięta

	// Autoconnect
	Config.hostName 		= "Gree_AC";					// Sets host name to SotAp identification
	Config.apid 			= "Gree_AC";					// SoftAP's SSID.
	Config.psk 				= "12345678";					// Sets password for SoftAP. The length should be from 8 to up to 63.
	Config.homeUri 			= "/_ac";						// Sets home path of Sketch application
	Config.retainPortal 	= true;							// Launch the captive portal on-demand at losing WiFi
	Config.autoReconnect 	= true;							// Automatically will try to reconnect with the past established access point (BSSID) when the current configured SSID in ESP8266/ESP32 could not be connected.
	Config.ota 				= AC_OTA_BUILTIN;				// Specifies to include AutoConnectOTA in the Sketch.
	Portal.load(FPSTR(Version));							// Load AutoConnectAux custom web page
	Config.menuItems = Config.menuItems | AC_MENUITEM_DELETESSID;	// https://hieromon.github.io/AutoConnect/apiconfig.html#menuitems
	Portal.config(Config);									// Don't forget it.
	if (Portal.begin())										// Starts and behaves captive portal
	{	
		Serial.println("WiFi connected: " + WiFi.localIP().toString());
	}

	//WiFi.begin(ssid, pass);
	Blynk.config(auth, server, port);					// for local servernon-blocking, even if no server connection
	//Blynk.config(auth);								// For cloud

	//Inicjalizacja Timerów
	Timer.setInterval(30000, blynkCheck);				// Co 30s zostanie sprawdzony czy jest sieć Wi-Fi i czy połączono z serwerem Blynk
	Timer.setInterval(10000, MainFunction);				// Uruchamia wszystko w pętli co 10s

	//MODBUS
	// Modbus communication runs at 9600 baud
	Serial.begin(9600, SERIAL_8N1);						// Tego portu szeregowego używamy do wypluwania danych na komputer
	Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);		// Tego portu szeregowego używamy do komunikazni protokołem MODBUS
	while (!Serial) { }									// Wait for serial port to connect. Needed for native USB 
	while (!Serial2) { }								// Wait for serial2 port to connect. Needed for native USB
	// Modbus slave ID 1
	node.begin(Slave_ID, Serial2);
	node.preTransmission(preTransmission);
	node.postTransmission(postTransmission);
	node.readHoldingRegisters(100, 1);					// Pierwsze wywołanie zawsze zwracało błąd więc dywołujemy ale go nie odczytujemy
	//node.idle(yield);
}


void loop()
{
	Timer.run();
	Portal.handleClient();

	if (WiFi.status() == WL_CONNECTED)
	{
		// Here to do when WiFi is connected.
		if (Blynk.connected())
		{
			Blynk.run();
		} 
		else
		{
			Blynk.connect();
		}

		if(Timer.isEnabled( timerIDReset ))
		{
			Timer.deleteTimer( timerIDReset );
		}
	}
	else	// Zrestartuje sterownik jeśli brak sieci przez 5min
	{
		if (Timer.isEnabled( timerIDReset ) == false)
		{
			timerIDReset = Timer.setTimeout( 300000, RestartESP32 ); 		// 300000 Milliseconds = 5 Minutes
		}
		delay(1500);
	}

}