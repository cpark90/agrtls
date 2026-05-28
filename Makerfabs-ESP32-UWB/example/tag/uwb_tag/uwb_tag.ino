/*

For ESP32 UWB or ESP32 UWB Pro

*/
#include <SPI.h>
#include "DW1000Ranging.h"

#include <ros.h>
#include <std_msgs/Float32MultiArray.h>

#define TAG_ADD "T0:00:00:00:00:00:00:00"

#define SPI_SCK 18
#define SPI_MISO 19
#define SPI_MOSI 23

// connection pins
const uint8_t		PIN_RST = 27; // reset pin
const uint8_t		PIN_IRQ = 34; // irq pin
const uint8_t		PIN_SS = 21;   // spi select pin

#define FLAG_NEW_RANGE 0x01
#define FLAG_NEW_DEVICE 0x02
#define FLAG_DEL_DEVICE 0x04

ros::NodeHandle		nh;

std_msgs::Float32MultiArray	msg;
ros::Publisher		uwb_pub("tag", &msg);

struct	RangePacket {
	uint8_t		flags;
	float		range;
	uint16_t	shortAddr;
	float		rxPower;
};

float			msg_buffer[4];
volatile bool	event_flag = false;
RangePacket		packet;

void newRange()
{
	DW1000Device*	device = DW1000Ranging.getDistantDevice();

	packet.flags = FLAG_NEW_RANGE;
	packet.shortAddr = device->getShortAddress();
	packet.range = device->getRange();
	packet.rxPower = device->getRXPower();

	event_flag = true;
}

void newDevice(DW1000Device *device)
{
	packet.flags = FLAG_NEW_DEVICE;
	packet.shortAddr = device->getShortAddress();

	event_flag = true;
}

void inactiveDevice(DW1000Device *device)
{
	packet.flags = FLAG_DEL_DEVICE;
	packet.shortAddr = device->getShortAddress();

	event_flag = true;
}

void setup()
{
	Serial.begin(115200);
	delay(1000);
	//init the configuration
	SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
	
	//ROS
	nh.initNode();
	while(!nh.connected()) {
		delay(100);
	}
	nh.advertise(uwb_pub);

	DW1000Ranging.initCommunication(PIN_RST, PIN_SS, PIN_IRQ); //Reset, CS, IRQ pin
	//define the sketch as anchor. It will be great to dynamically change the type of module
	DW1000Ranging.attachNewRange(newRange);
	DW1000Ranging.attachNewDevice(newDevice);
	DW1000Ranging.attachInactiveDevice(inactiveDevice);
	//Enable the filter to smooth the distance
	//DW1000Ranging.useRangeFilter(true);
	//we start the module as a tag
	DW1000Ranging.startAsTag(TAG_ADD, DW1000.MODE_LONGDATA_RANGE_LOWPOWER);

	msg.data = msg_buffer;
	msg.data_length = 4;
}

void loop()
{
    DW1000Ranging.loop();
	if (event_flag)
	{
		event_flag = false;
		msg_buffer[0] = packet.flags;
		msg_buffer[1] = packet.shortAddr;
		msg_buffer[2] = packet.range;
		msg_buffer[3] = packet.rxPower;
		uwb_pub.publish(&msg);
		nh.spinOnce();
	}
	delay(10);
}
