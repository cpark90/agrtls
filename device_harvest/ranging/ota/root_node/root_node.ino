#include <painlessMesh.h>

#define MESH_PREFIX "meshOTA"
#define MESH_PASSWORD "meshpassword"
#define MESH_PORT 5555

Scheduler userScheduler;
painlessMesh mesh;

void sendMesh(uint8_t *data,int len)
{
    String msg((char*)data,len);
    mesh.sendBroadcast(msg);
}

void receivedCallback(uint32_t from,String &msg)
{
    uint8_t *data=(uint8_t*)msg.c_str();

	Serial.println("received");
    Serial.write(data,msg.length());
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

    Serial.println("PC Mesh Router ready");
}

void loop()
{
    mesh.update();

    if(Serial.available())
    {
        uint8_t buffer[1200];

        int len = Serial.readBytes(buffer,sizeof(buffer));

        if(len>0)
        {
            sendMesh(buffer,len);
        }
    }
}
