#include <WiFi.h>
#include <esp_ota_ops.h>
#include <painlessMesh.h>
#include "ota_protocol.h"

#define MESH_PREFIX "meshOTA"
#define MESH_PASSWORD "meshpassword"
#define MESH_PORT 5555

Scheduler userScheduler;
painlessMesh mesh;

uint8_t buffer[OTA_CHUNK];

esp_ota_handle_t ota_handle;
const esp_partition_t *update_partition;

uint32_t parentNode=0;

uint32_t firmwareSize=0;
uint32_t received=0;
uint16_t seqExpected=0;

bool otaRunning=false;
bool f_test=false;

void resetOTA()
{
    otaRunning=false;
    firmwareSize=0;
    received=0;
    seqExpected=0;

    Serial.println("OTA reset");
}

void requestOTA(uint32_t node)
{
    ota_req_t req;

    req.type=OTA_REQ;
    req.version=FW_VERSION;

    String msg((char*)&req,sizeof(req));

    mesh.sendSingle(node,msg);
}

void startOTA()
{
    update_partition = esp_ota_get_next_update_partition(NULL);

    esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &ota_handle);

    received=0;
    seqExpected=0;

    otaRunning=true;

    Serial.println("OTA started");
}

void finishOTA()
{
    esp_ota_end(ota_handle);
    esp_ota_set_boot_partition(update_partition);

    Serial.println("OTA done");

    delay(1000);

    ESP.restart();
}

void sendACK(uint16_t seq)
{
    ota_packet_t pkt;
    pkt.type=OTA_ACK;
    pkt.seq=seq;

    String msg((char*)&pkt,sizeof(pkt));

    mesh.sendSingle(parentNode,msg);
}

void handlePacket(uint32_t from,uint8_t *data,int len)
{
    uint8_t type=data[0];

    if(type==OTA_NOTIFY)
    {
        parentNode=from;
		f_test=true;
        Serial.println("Notify");

        requestOTA(from);
    }
    else if(type==OTA_READY)
    {
        startOTA();
    }
    else if(type==OTA_DATA && otaRunning)
    {
        ota_packet_t *h=(ota_packet_t*)data;

        if(h->seq!=seqExpected)
            return;

        uint8_t *fw=data+sizeof(ota_packet_t);

        esp_ota_write(ota_handle,fw,h->len);

        sendACK(h->seq);

        seqExpected++;
    }
    else if(type==OTA_END && otaRunning)
    {
        finishOTA();
    }
    else if(type==OTA_CANCEL)
    {
        resetOTA();
    }
}

void receivedCallback(uint32_t from,String &msg)
{
    uint8_t *data=(uint8_t*)msg.c_str();

    handlePacket(from,data,msg.length());
}

void setup()
{
    Serial.begin(115200);

    mesh.init(
        MESH_PREFIX,
        MESH_PASSWORD,
        &userScheduler,
        MESH_PORT
    );

    mesh.onReceive(receivedCallback);

    Serial.println("Mesh init");
}

void loop()
{
    mesh.update();
    ota_req_t req;

    req.type=OTA_REQ;
    req.version=FW_VERSION;

    String msg((char*)&req,3);
    mesh.sendBroadcast(msg);
}
