#include <Arduino.h>
#include <SPI.h>

#include <DW1000.h>
#include <DW1000Time.h>
#include <DW1000Device.h>

static constexpr uint8_t PIN_RST = 27;
static constexpr uint8_t PIN_IRQ = 34;
static constexpr uint8_t PIN_SS  = 5;

static constexpr uint16_t PAN_ID = 0xDECA;
static constexpr uint16_t SELF_SHORT = 0x0202;
static constexpr uint16_t ANCHOR_SHORT = 0x0102;

static constexpr uint16_t REPLY_DELAY_US = 3000;
static constexpr uint32_t SESSION_TIMEOUT_MS = 1000;

enum class SessionState : uint8_t {
  IDLE = 0,
  WAIT_POLL_ACK = 1,
  WAIT_RANGE_REPORT = 2,
  FINISHED = 3,
  FAILED = 4
};

enum UwbMsgCode : uint8_t {
  UWB_POLL         = 0xE0,
  UWB_POLL_ACK     = 0xE1,
  UWB_RANGE        = 0xE2,
  UWB_RANGE_REPORT = 0xE3
};

volatile bool g_sentFlag = false;
volatile bool g_recvFlag = false;
volatile bool g_rxTimeoutFlag = false;
volatile bool g_rxFailedFlag = false;

void handleSent()      { g_sentFlag = true; }
void handleReceived()  { g_recvFlag = true; }
void handleRxTimeout() { g_rxTimeoutFlag = true; }
void handleRxFailed()  { g_rxFailedFlag = true; }

#pragma pack(push, 1)
struct UwbFrame {
  uint8_t code;
  uint16_t src;
  uint16_t dst;
  uint32_t seq;

  uint8_t pollTxTs[5];
  uint8_t pollAckRxTs[5];
  uint8_t rangeTxTs[5];

  float rangeM;
  float rxPower;
  float fpPower;
  float quality;

  uint16_t tagValue1;
  uint16_t tagValue2;
};
#pragma pack(pop)

static void writeTs5(uint8_t out[5], const DW1000Time& t) {
  t.getTimestamp(out);
}

class TagInitiator {
public:
  void begin() {
    anchorDev_.setReplyTime(REPLY_DELAY_US);
    byte sa[2];
    sa[0] = (byte)(ANCHOR_SHORT & 0xFF);
    sa[1] = (byte)((ANCHOR_SHORT >> 8) & 0xFF);
    anchorDev_.setShortAddress(sa);
  }

  void loop() {
    if (state_ == SessionState::IDLE) return;

    if (millis() - startedAtMs_ > SESSION_TIMEOUT_MS) {
      fail();
      return;
    }

    if (g_rxTimeoutFlag || g_rxFailedFlag) {
      g_rxTimeoutFlag = false;
      g_rxFailedFlag = false;
      fail();
      return;
    }

    if (g_recvFlag) {
      g_recvFlag = false;
      UwbFrame f{};
      DW1000.getData((byte*)&f, sizeof(UwbFrame));
      onFrameReceived(f);
    }
  }

  void startSession() {
    if (state_ != SessionState::IDLE) return;

    seq_++;
    sendPoll();
    state_ = SessionState::WAIT_POLL_ACK;
    startedAtMs_ = millis();
  }

private:
  SessionState state_ = SessionState::IDLE;
  uint32_t seq_ = 1000;
  uint32_t startedAtMs_ = 0;
  DW1000Device anchorDev_;

  // tag가 전달하고 싶은 데이터 예시
  uint16_t tagValue1_ = 123;
  uint16_t tagValue2_ = 456;

private:
  void startRxOneShot() {
    DW1000.newReceive();
    DW1000.setDefaults();
    DW1000.receivePermanently(false);
    DW1000.startReceive();
  }

  void fail() {
    state_ = SessionState::FAILED;
    Serial.println("[TAG] session failed");
    DW1000.idle();
    delay(100);
    state_ = SessionState::IDLE;
  }

  void finish() {
    state_ = SessionState::FINISHED;
    Serial.println("[TAG] session finished");
    DW1000.idle();
    delay(100);
    state_ = SessionState::IDLE;
  }

  void sendFrame(UwbFrame& f, bool delayed = false, DW1000Time delay = DW1000Time()) {
    g_sentFlag = false;
    DW1000.newTransmit();
    DW1000.setDefaults();
    if (delayed) DW1000.setDelay(delay);
    DW1000.setData((byte*)&f, sizeof(UwbFrame));
    DW1000.startTransmit();
  }

  void sendPoll() {
    UwbFrame f{};
    f.code = UWB_POLL;
    f.src = SELF_SHORT;
    f.dst = ANCHOR_SHORT;
    f.seq = seq_;

    sendFrame(f, false);
    while (!g_sentFlag) { delayMicroseconds(50); }
    g_sentFlag = false;

    DW1000.getTransmitTimestamp(anchorDev_.timePollSent);
    startRxOneShot();
  }

  void sendRange() {
    UwbFrame f{};
    f.code = UWB_RANGE;
    f.src = SELF_SHORT;
    f.dst = ANCHOR_SHORT;
    f.seq = seq_;

    writeTs5(f.pollTxTs, anchorDev_.timePollSent);
    writeTs5(f.pollAckRxTs, anchorDev_.timePollAckReceived);

    DW1000Time delta(REPLY_DELAY_US, DW1000Time::MICROSECONDS);
    anchorDev_.timeRangeSent = DW1000.setDelay(delta);
    writeTs5(f.rangeTxTs, anchorDev_.timeRangeSent);

    // tag application data
    f.tagValue1 = tagValue1_;
    f.tagValue2 = tagValue2_;

    g_sentFlag = false;
    DW1000.newTransmit();
    DW1000.setDefaults();
    DW1000.setData((byte*)&f, sizeof(UwbFrame));
    DW1000.startTransmit();

    while (!g_sentFlag) { delayMicroseconds(50); }
    g_sentFlag = false;

    startRxOneShot();
  }

  void onFrameReceived(const UwbFrame& f) {
    if (f.dst != SELF_SHORT) {
      startRxOneShot();
      return;
    }
    if (f.src != ANCHOR_SHORT) {
      startRxOneShot();
      return;
    }
    if (f.seq != seq_) {
      startRxOneShot();
      return;
    }

    switch (f.code) {
      case UWB_POLL_ACK:
        if (state_ != SessionState::WAIT_POLL_ACK) {
          fail();
          return;
        }
        DW1000.getReceiveTimestamp(anchorDev_.timePollAckReceived);
        sendRange();
        state_ = SessionState::WAIT_RANGE_REPORT;
        break;

      case UWB_RANGE_REPORT:
        if (state_ != SessionState::WAIT_RANGE_REPORT) {
          fail();
          return;
        }

        Serial.printf("[TAG] range=%.3f rx=%.2f fp=%.2f q=%.2f echoed=(%u,%u)\n",
                      f.rangeM, f.rxPower, f.fpPower, f.quality,
                      f.tagValue1, f.tagValue2);
        finish();
        break;

      default:
        startRxOneShot();
        break;
    }
  }
};

TagInitiator tag;

// ============================================================
// Setup
// ============================================================
void setupDw1000() {
  DW1000.begin(PIN_IRQ, PIN_RST);
  DW1000.select(PIN_SS);

  DW1000.newConfiguration();
  DW1000.setDefaults();
  DW1000.setNetworkId(PAN_ID);
  DW1000.setDeviceAddress(SELF_SHORT);
  DW1000.commitConfiguration();

  DW1000.attachSentHandler(handleSent);
  DW1000.attachReceivedHandler(handleReceived);
  DW1000.attachReceiveTimeoutHandler(handleRxTimeout);
  DW1000.attachReceiveFailedHandler(handleRxFailed);
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  setupDw1000();
  tag.begin();

  Serial.println("Tag node started (mesh unused for UWB control)");
}

void loop() {
  tag.loop();

  // 예시: 주기적으로 anchor에 능동 측정
  static uint32_t lastStart = 0;
  if (millis() - lastStart > 5000) {
    lastStart = millis();
    tag.startSession();
  }
}
