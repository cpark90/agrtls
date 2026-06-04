#include <Arduino.h>
#include <SPI.h>
#include <painlessMesh.h>
#include <DW1000.h>
#include <DW1000Ranging.h>

// ======================
// USER CONFIG
// ======================
#define IS_TAG_NODE 1  // 1=TAG, 0=ANCHOR

static char* MY_UWB_ADDR = "A1A1A1A1A1A1A1A1";

static const char* MESH_PREFIX   = "UWB_MESH";
static const char* MESH_PASSWORD = "uwbmeshpass";
static const uint16_t MESH_PORT  = 5555;

static const uint32_t SLOT_MS = 500;
static const uint32_t PRESENCE_MS = 1500;

static const uint8_t PIN_UWB_CS  = 5;
static const uint8_t PIN_UWB_RST = 27;
static const uint8_t PIN_UWB_IRQ = 34;

// ======================
painlessMesh mesh;
Scheduler scheduler;

struct Peer {
  String addr;
  uint32_t lastSeen;
};

Peer peers[20];
int peerN = 0;

// ======================
// Peer 관리
// ======================
void upsertPeer(String a) {
  for(int i=0;i<peerN;i++){
    if(peers[i].addr == a){
      peers[i].lastSeen = millis();
      return;
    }
  }
  if(peerN < 20){
    peers[peerN++] = {a, millis()};
  }
}

void prunePeers(){
  for(int i=0;i<peerN;){
    if(millis() - peers[i].lastSeen > 8000){
      peers[i] = peers[peerN-1];
      peerN--;
    } else i++;
  }
}

// ======================
// Mesh 메시지
// ======================
String presenceMsg(){
  return "{\"type\":\"presence\",\"addr\":\""+String(MY_UWB_ADDR)+"\"}";
}

bool extractField(const String& s, const String& key, String& out){
  String needle="\""+key+"\":\"";
  int i=s.indexOf(needle);
  if(i<0) return false;
  i+=needle.length();
  int j=s.indexOf("\"",i);
  if(j<0) return false;
  out=s.substring(i,j);
  return true;
}

void receivedCallback(uint32_t from, String &msg){
  (void)from;
  String type;
  if(!extractField(msg,"type",type)) return;
  if(type=="presence"){
    String a;
    if(extractField(msg,"addr",a)) upsertPeer(a);
  }
}

// ======================
// UWB 제어
// ======================
enum Mode { RESPONDER, INITIATOR };
Mode currentMode = RESPONDER;

void setResponder(){
  if(currentMode==RESPONDER) return;
//   DW1000Ranging.removeNetworkDevices();
//   DW1000Ranging.startAsAnchor(MY_UWB_ADDR, DW1000.MODE_LONGDATA_RANGE_LOWPOWER,false);
  currentMode = RESPONDER;
}

void setInitiator(String target){
//   DW1000Ranging.removeNetworkDevices();
//   DW1000Ranging.startAsTag(MY_UWB_ADDR, DW1000.MODE_LONGDATA_RANGE_LOWPOWER,false);
//   DW1000Ranging.addDevice(target.c_str(), DW1000.MODE_LONGDATA_RANGE_LOWPOWER);
  currentMode = INITIATOR;
}

// ======================
// 슬롯 처리
// ======================
uint32_t lastSlot = 0xFFFFFFFF;

void handleSlot(){
  uint32_t slot = (millis()/SLOT_MS);

  if(slot==lastSlot) return;
  lastSlot = slot;

  prunePeers();

  if(IS_TAG_NODE){
    // TAG는 항상 initiator
    if(peerN>0){
      String target = peers[slot % peerN].addr;
      if(target != String(MY_UWB_ADDR))
        setInitiator(target);
    }
  } else {
    // ANCHOR 기본 responder
    setResponder();

    // 자기 슬롯이면 initiator로 전환
    if(peerN>1){
      int myIndex=-1;
      for(int i=0;i<peerN;i++)
        if(peers[i].addr==String(MY_UWB_ADDR)) myIndex=i;

      if(myIndex>=0 && (slot % peerN)==myIndex){
        String target = peers[(myIndex+1)%peerN].addr;
        if(target!=String(MY_UWB_ADDR)){
          setInitiator(target);
        }
      }
    }
  }
}

// ======================
// UWB Callback
// ======================
void newRange(){
//   DW1000Device* d = DW1000Ranging.getDistantDevice();
//   if(!d) return;

//   float m = d->getRange();
  float m = 3.0;
  Serial.printf("Range %.3f m\n", m);
}

void setup(){
  Serial.begin(115200);
  delay(100);

  mesh.init(MESH_PREFIX, MESH_PASSWORD, &scheduler, MESH_PORT);
  mesh.onReceive(&receivedCallback);

  scheduler.addTask(Task(PRESENCE_MS,TASK_FOREVER,[]{
    mesh.sendBroadcast(presenceMsg());
  })).enable();

  SPI.begin(18,19,23);
//   DW1000Ranging.initCommunication(PIN_UWB_RST,PIN_UWB_CS,PIN_UWB_IRQ);
//   DW1000Ranging.attachNewRange(newRange);

//   if(IS_TAG_NODE)
//     DW1000Ranging.startAsTag(MY_UWB_ADDR,DW1000.MODE_LONGDATA_RANGE_LOWPOWER,false);
//   else
//     DW1000Ranging.startAsAnchor(MY_UWB_ADDR,DW1000.MODE_LONGDATA_RANGE_LOWPOWER,false);

  upsertPeer(String(MY_UWB_ADDR));

  Serial.println(IS_TAG_NODE ? "TAG NODE" : "ANCHOR NODE");
}

void loop(){
  mesh.update();
//   DW1000Ranging.loop();
  handleSlot();
}
