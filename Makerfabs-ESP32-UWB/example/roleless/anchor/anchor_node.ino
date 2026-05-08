#include <Arduino.h>
#include <SPI.h>
#include <painlessMesh.h>
#include <ArduinoJson.h>

#include <DW1000.h>
#include <DW1000Time.h>
#include <DW1000Device.h>

// ============================================================
// Mesh config
// ============================================================
#define MESH_PREFIX   "uwb-mesh"
#define MESH_PASSWORD "uwb-mesh-pass"
#define MESH_PORT     5555

Scheduler userScheduler;
painlessMesh mesh;

// ============================================================
// Pin / local config
// ============================================================
static constexpr uint8_t PIN_RST = 27;
static constexpr uint8_t PIN_IRQ = 34;
static constexpr uint8_t PIN_SS  = 5;

static constexpr uint16_t PAN_ID = 0xDECA;
static constexpr uint16_t SELF_SHORT = 0x0102;
static constexpr uint32_t MASTER_NODE_ID = 999999999;   // 예시: master mesh node id

static constexpr uint16_t REPLY_DELAY_US      = 3000;
static constexpr uint32_t REQUEST_TIMEOUT_MS  = 1200;
static constexpr uint32_t SESSION_TIMEOUT_MS  = 1000;
static constexpr uint32_t BACKOFF_MIN_MS      = 50;
static constexpr uint32_t BACKOFF_MAX_MS      = 180;
static constexpr uint16_t MAX_FAIL_COUNT      = 3;
static constexpr size_t   MAX_ANCHORS         = 16;
static constexpr size_t   MAX_TAGS            = 32;

enum class AnchorMode : uint8_t {
  AA = 0,
  AT = 1
};

enum class RunState : uint8_t {
  FREE = 0,
  REQUESTING = 1,
  MATCHED = 2,
  BACKOFF = 3
};

enum class SessionRole : uint8_t {
  INITIATOR = 0,
  RESPONDER = 1
};

enum class SessionState : uint8_t {
  IDLE = 0,
  WAIT_POLL = 1,
  WAIT_POLL_ACK = 2,
  WAIT_RANGE = 3,
  WAIT_RANGE_REPORT = 4,
  FINISHED = 5,
  FAILED = 6
};

enum UwbMsgCode : uint8_t {
  UWB_POLL         = 0xE0,
  UWB_POLL_ACK     = 0xE1,
  UWB_RANGE        = 0xE2,
  UWB_RANGE_REPORT = 0xE3
};

static uint32_t randRange32(uint32_t a, uint32_t b) {
  return (uint32_t)random((long)a, (long)(b + 1));
}

static void shortToBytes(uint16_t shortAddr, byte out[2]) {
  out[0] = (byte)(shortAddr & 0xFF);
  out[1] = (byte)((shortAddr >> 8) & 0xFF);
}

static void writeTs5(uint8_t out[5], const DW1000Time& t) {
  t.getTimestamp(out);
}

static DW1000Time readTs5(const uint8_t in[5]) {
  DW1000Time t;
  t.setTimestamp((byte*)in);
  return t;
}

// ============================================================
// IRQ bridge
// ============================================================
volatile bool g_sentFlag = false;
volatile bool g_recvFlag = false;
volatile bool g_rxTimeoutFlag = false;
volatile bool g_rxFailedFlag = false;

void handleSent()      { g_sentFlag = true; }
void handleReceived()  { g_recvFlag = true; }
void handleRxTimeout() { g_rxTimeoutFlag = true; }
void handleRxFailed()  { g_rxFailedFlag = true; }

// ============================================================
// UWB frame
// ============================================================
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

  // tag 측 부가 데이터
  uint16_t tagValue1;
  uint16_t tagValue2;
};
#pragma pack(pop)

// ============================================================
// Neighbor slots
// ============================================================
struct AnchorSlot {
  bool used = false;
  uint32_t meshNodeId = 0;
  bool alive = true;
  uint32_t lastUpdateMs = 0;
  uint16_t failCount = 0;
  DW1000Device dev;
};

struct TagSlot {
  bool used = false;
  uint16_t shortAddr = 0;
  bool alive = true;
  uint32_t lastSeenMs = 0;
  uint32_t lastUpdateMs = 0;
  uint16_t failCount = 0;
  float lastRange = 0.0f;
  float lastRxPower = 0.0f;
  float lastFpPower = 0.0f;
  float lastQuality = 0.0f;
  uint16_t tagValue1 = 0;
  uint16_t tagValue2 = 0;
};

// ============================================================
// Pairwise UWB session
// ============================================================
class UwbTwrSession {
public:
  void begin(uint16_t selfShortAddr) {
    selfShortAddr_ = selfShortAddr;
    reset();
  }

  void reset() {
    active_ = false;
    state_ = SessionState::IDLE;
    peerAnchor_ = nullptr;
    peerTag_ = nullptr;
    role_ = SessionRole::RESPONDER;
    seq_ = 0;
    startedAtMs_ = 0;
    memset(&lastRxFrame_, 0, sizeof(lastRxFrame_));
  }

  bool finished() const { return state_ == SessionState::FINISHED; }
  bool failed() const   { return state_ == SessionState::FAILED; }

  // AA mode
  void startInitiatorAA(DW1000Device* peer, uint32_t seq) {
    reset();
    active_ = true;
    role_ = SessionRole::INITIATOR;
    peerAnchor_ = peer;
    seq_ = seq;
    startedAtMs_ = millis();
    sendPoll(peerAnchor_->getShortAddress());
    state_ = SessionState::WAIT_POLL_ACK;
  }

  void startResponderAA(DW1000Device* peer, uint32_t seq) {
    reset();
    active_ = true;
    role_ = SessionRole::RESPONDER;
    peerAnchor_ = peer;
    seq_ = seq;
    startedAtMs_ = millis();
    state_ = SessionState::WAIT_POLL;
    startRxPermanent();
  }

  // AT mode: anchor is responder only
  void startResponderAT(TagSlot* peerTag, uint32_t seq) {
    reset();
    active_ = true;
    role_ = SessionRole::RESPONDER;
    peerTag_ = peerTag;
    seq_ = seq;
    startedAtMs_ = millis();
    state_ = SessionState::WAIT_POLL;
    startRxPermanent();
  }

  void loop() {
    if (!active_) return;

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

  float lastRange() const { return lastRange_; }
  float lastRxPower() const { return lastRxPower_; }
  float lastFpPower() const { return lastFpPower_; }
  float lastQuality() const { return lastQuality_; }
  uint16_t lastTagValue1() const { return lastTagValue1_; }
  uint16_t lastTagValue2() const { return lastTagValue2_; }

private:
  bool active_ = false;
  SessionState state_ = SessionState::IDLE;
  SessionRole role_ = SessionRole::RESPONDER;
  uint16_t selfShortAddr_ = 0;
  DW1000Device* peerAnchor_ = nullptr;
  TagSlot* peerTag_ = nullptr;
  uint32_t seq_ = 0;
  uint32_t startedAtMs_ = 0;
  UwbFrame lastRxFrame_{};

  float lastRange_ = 0.0f;
  float lastRxPower_ = 0.0f;
  float lastFpPower_ = 0.0f;
  float lastQuality_ = 0.0f;
  uint16_t lastTagValue1_ = 0;
  uint16_t lastTagValue2_ = 0;

private:
  uint16_t peerShortAddr() const {
    if (peerAnchor_) return peerAnchor_->getShortAddress();
    if (peerTag_) return peerTag_->shortAddr;
    return 0;
  }

  void startRxPermanent() {
    DW1000.newReceive();
    DW1000.setDefaults();
    DW1000.receivePermanently(true);
    DW1000.startReceive();
  }

  void startRxOneShot() {
    DW1000.newReceive();
    DW1000.setDefaults();
    DW1000.receivePermanently(false);
    DW1000.startReceive();
  }

  void fail() {
    active_ = false;
    state_ = SessionState::FAILED;
    DW1000.idle();
  }

  void finish() {
    active_ = false;
    state_ = SessionState::FINISHED;
    DW1000.idle();
  }

  void sendFrame(UwbFrame& f, bool delayed = false, DW1000Time delay = DW1000Time()) {
    g_sentFlag = false;
    DW1000.newTransmit();
    DW1000.setDefaults();
    if (delayed) DW1000.setDelay(delay);
    DW1000.setData((byte*)&f, sizeof(UwbFrame));
    DW1000.startTransmit();
  }

  void sendPoll(uint16_t dst) {
    UwbFrame f{};
    f.code = UWB_POLL;
    f.src = selfShortAddr_;
    f.dst = dst;
    f.seq = seq_;

    sendFrame(f, false);
    while (!g_sentFlag) { delayMicroseconds(50); }
    g_sentFlag = false;

    if (peerAnchor_) {
      DW1000.getTransmitTimestamp(peerAnchor_->timePollSent);
    }
    startRxOneShot();
  }

  void sendPollAck(uint16_t dst, DW1000Time* storeTxTs) {
    UwbFrame f{};
    f.code = UWB_POLL_ACK;
    f.src  = selfShortAddr_;
    f.dst  = dst;
    f.seq  = seq_;

    DW1000Time delta(REPLY_DELAY_US, DW1000Time::MICROSECONDS);
    sendFrame(f, true, delta);
    while (!g_sentFlag) { delayMicroseconds(50); }
    g_sentFlag = false;

    DW1000.getTransmitTimestamp(*storeTxTs);
    startRxOneShot();
  }

  void sendRange(uint16_t dst, DW1000Device* dev) {
    UwbFrame f{};
    f.code = UWB_RANGE;
    f.src  = selfShortAddr_;
    f.dst  = dst;
    f.seq  = seq_;

    writeTs5(f.pollTxTs, dev->timePollSent);
    writeTs5(f.pollAckRxTs, dev->timePollAckReceived);

    DW1000Time delta(REPLY_DELAY_US, DW1000Time::MICROSECONDS);
    dev->timeRangeSent = DW1000.setDelay(delta);
    writeTs5(f.rangeTxTs, dev->timeRangeSent);

    g_sentFlag = false;
    DW1000.newTransmit();
    DW1000.setDefaults();
    DW1000.setData((byte*)&f, sizeof(UwbFrame));
    DW1000.startTransmit();

    while (!g_sentFlag) { delayMicroseconds(50); }
    g_sentFlag = false;

    startRxOneShot();
  }

  void sendRangeReportToAnchorPeer(DW1000Device* dev) {
    UwbFrame f{};
    f.code = UWB_RANGE_REPORT;
    f.src  = selfShortAddr_;
    f.dst  = dev->getShortAddress();
    f.seq  = seq_;
    f.rangeM  = dev->getRange();
    f.rxPower = dev->getRXPower();
    f.fpPower = dev->getFPPower();
    f.quality = dev->getQuality();

    DW1000Time delta(REPLY_DELAY_US, DW1000Time::MICROSECONDS);
    sendFrame(f, true, delta);
    while (!g_sentFlag) { delayMicroseconds(50); }
    g_sentFlag = false;
  }

  void sendRangeReportToTagPeer(TagSlot* tag) {
    UwbFrame f{};
    f.code = UWB_RANGE_REPORT;
    f.src  = selfShortAddr_;
    f.dst  = tag->shortAddr;
    f.seq  = seq_;
    f.rangeM  = lastRange_;
    f.rxPower = lastRxPower_;
    f.fpPower = lastFpPower_;
    f.quality = lastQuality_;
    f.tagValue1 = lastTagValue1_;
    f.tagValue2 = lastTagValue2_;

    DW1000Time delta(REPLY_DELAY_US, DW1000Time::MICROSECONDS);
    sendFrame(f, true, delta);
    while (!g_sentFlag) { delayMicroseconds(50); }
    g_sentFlag = false;
  }

  void computeRangeAsymmetric(DW1000Device* dev, DW1000Time* myTOF) {
    DW1000Time round1 = (dev->timePollAckReceived - dev->timePollSent).wrap();
    DW1000Time reply1 = (dev->timePollAckSent - dev->timePollReceived).wrap();
    DW1000Time round2 = (dev->timeRangeReceived - dev->timePollAckSent).wrap();
    DW1000Time reply2 = (dev->timeRangeSent - dev->timePollAckReceived).wrap();

    myTOF->setTimestamp((round1 * round2 - reply1 * reply2) /
                        (round1 + round2 + reply1 + reply2));
  }

  void onFrameReceived(const UwbFrame& f) {
    if (f.dst != selfShortAddr_) {
      startRxOneShot();
      return;
    }

    if (role_ == SessionRole::RESPONDER) {
      onResponderFrame(f);
    } else {
      onInitiatorFrame(f);
    }
  }

  void onResponderFrame(const UwbFrame& f) {
    // AA responder
    if (peerAnchor_ && f.src == peerAnchor_->getShortAddress()) {
      switch (f.code) {
        case UWB_POLL:
          if (state_ != SessionState::WAIT_POLL) { fail(); return; }
          DW1000.getReceiveTimestamp(peerAnchor_->timePollReceived);
          sendPollAck(peerAnchor_->getShortAddress(), &peerAnchor_->timePollAckSent);
          state_ = SessionState::WAIT_RANGE;
          break;

        case UWB_RANGE:
          if (state_ != SessionState::WAIT_RANGE) { fail(); return; }
          DW1000.getReceiveTimestamp(peerAnchor_->timeRangeReceived);
          peerAnchor_->timePollSent        = readTs5(f.pollTxTs);
          peerAnchor_->timePollAckReceived = readTs5(f.pollAckRxTs);
          peerAnchor_->timeRangeSent       = readTs5(f.rangeTxTs);

          {
            DW1000Time tof;
            computeRangeAsymmetric(peerAnchor_, &tof);
            peerAnchor_->setRange(tof.getAsMeters());
            peerAnchor_->setRXPower(DW1000.getReceivePower());
            peerAnchor_->setFPPower(peerAnchor_->getRXPower());
            peerAnchor_->setQuality(1.0f);

            lastRange_ = peerAnchor_->getRange();
            lastRxPower_ = peerAnchor_->getRXPower();
            lastFpPower_ = peerAnchor_->getFPPower();
            lastQuality_ = peerAnchor_->getQuality();

            sendRangeReportToAnchorPeer(peerAnchor_);
            finish();
          }
          break;

        default:
          startRxOneShot();
          break;
      }
      return;
    }

    // AT responder to tag
    if (peerTag_ && f.src == peerTag_->shortAddr) {
      switch (f.code) {
        case UWB_POLL:
          if (state_ != SessionState::WAIT_POLL) { fail(); return; }
          tempDev_.timePollReceived = DW1000Time();
          DW1000.getReceiveTimestamp(tempDev_.timePollReceived);
          sendPollAck(peerTag_->shortAddr, &tempDev_.timePollAckSent);
          state_ = SessionState::WAIT_RANGE;
          break;

        case UWB_RANGE:
          if (state_ != SessionState::WAIT_RANGE) { fail(); return; }
          DW1000.getReceiveTimestamp(tempDev_.timeRangeReceived);
          tempDev_.timePollSent        = readTs5(f.pollTxTs);
          tempDev_.timePollAckReceived = readTs5(f.pollAckRxTs);
          tempDev_.timeRangeSent       = readTs5(f.rangeTxTs);

          {
            DW1000Time tof;
            computeRangeAsymmetric(&tempDev_, &tof);
            lastRange_ = tof.getAsMeters();
            lastRxPower_ = DW1000.getReceivePower();
            lastFpPower_ = lastRxPower_;
            lastQuality_ = 1.0f;
            lastTagValue1_ = f.tagValue1;
            lastTagValue2_ = f.tagValue2;

            peerTag_->lastRange = lastRange_;
            peerTag_->lastRxPower = lastRxPower_;
            peerTag_->lastFpPower = lastFpPower_;
            peerTag_->lastQuality = lastQuality_;
            peerTag_->tagValue1 = lastTagValue1_;
            peerTag_->tagValue2 = lastTagValue2_;
            peerTag_->lastSeenMs = millis();

            sendRangeReportToTagPeer(peerTag_);
            finish();
          }
          break;

        default:
          startRxOneShot();
          break;
      }
      return;
    }

    startRxOneShot();
  }

  void onInitiatorFrame(const UwbFrame& f) {
    if (!peerAnchor_) {
      fail();
      return;
    }

    if (f.src != peerAnchor_->getShortAddress()) {
      startRxOneShot();
      return;
    }

    switch (f.code) {
      case UWB_POLL_ACK:
        if (state_ != SessionState::WAIT_POLL_ACK) { fail(); return; }
        DW1000.getReceiveTimestamp(peerAnchor_->timePollAckReceived);
        sendRange(peerAnchor_->getShortAddress(), peerAnchor_);
        state_ = SessionState::WAIT_RANGE_REPORT;
        break;

      case UWB_RANGE_REPORT:
        if (state_ != SessionState::WAIT_RANGE_REPORT) { fail(); return; }
        peerAnchor_->setRange(f.rangeM);
        peerAnchor_->setRXPower(f.rxPower);
        peerAnchor_->setFPPower(f.fpPower);
        peerAnchor_->setQuality(f.quality);

        lastRange_ = f.rangeM;
        lastRxPower_ = f.rxPower;
        lastFpPower_ = f.fpPower;
        lastQuality_ = f.quality;

        finish();
        break;

      default:
        startRxOneShot();
        break;
    }
  }

  DW1000Device tempDev_;
};

// ============================================================
// Anchor agent
// ============================================================
class AnchorAgent {
public:
  void begin() {
    session_.begin(SELF_SHORT);
  }

  void setModeExternal(AnchorMode m) {
    pendingMode_ = m;
    modeDirty_ = true;
  }

  void loop() {
    const uint32_t now = millis();

    applyPendingModeIfSafe();
    session_.loop();

    if (runState_ == RunState::MATCHED) {
      if (session_.finished()) {
        onSessionFinished(now);
      } else if (session_.failed()) {
        onSessionFailed(now);
      }
      return;
    }

    expireAnchors();
    expireTags();
    pruneFailedAnchors();
    pruneFailedTags();

    switch (runState_) {
      case RunState::REQUESTING:
        if (now - requestStartedAtMs_ > REQUEST_TIMEOUT_MS) {
          resetRequesting();
          enterBackoff(now);
        }
        break;
      case RunState::BACKOFF:
        if ((int32_t)(now - backoffUntilMs_) >= 0) {
          runState_ = RunState::FREE;
        }
        break;
      case RunState::FREE:
      default:
        if (currentMode_ == AnchorMode::AA) {
          tryScheduleAA(now);
        } else {
          // AT mode: anchor does not initiate to tag
        }
        break;
    }
  }

  void onMeshMessage(uint32_t from, const String& msg) {
    StaticJsonDocument<256> doc;
    if (deserializeJson(doc, msg)) return;

    const char* kind = doc["t"] | "";

    if (strcmp(kind, "HEART") == 0) {
      const char* role = doc["r"] | "";
      if (strcmp(role, "anchor") == 0) {
        uint16_t sa = doc["sa"] | 0;
        ensureAnchor(from, sa);
      } else if (strcmp(role, "master") == 0) {
        masterNodeId_ = from;
      }
      return;
    }

    if (strcmp(kind, "REQ_AA") == 0)      handleReqAA(from, doc);
    else if (strcmp(kind, "ACC_AA") == 0) handleAccAA(from, doc);
    else if (strcmp(kind, "REJ_AA") == 0) handleRejAA(from, doc);
    else if (strcmp(kind, "DONE_AA") == 0) handleDoneAA(from);
  }

  void broadcastHeartbeat() {
    StaticJsonDocument<96> doc;
    doc["t"] = "HEART";
    doc["r"] = "anchor";
    doc["sa"] = SELF_SHORT;
    String out;
    serializeJson(doc, out);
    mesh.sendBroadcast(out);
  }

  // Tag discovered by UWB or preconfigured externally
  TagSlot* ensureTag(uint16_t shortAddr) {
    for (size_t i = 0; i < MAX_TAGS; ++i) {
      if (tags_[i].used && tags_[i].shortAddr == shortAddr) {
        tags_[i].alive = true;
        tags_[i].lastSeenMs = millis();
        if (tags_[i].failCount > 0) tags_[i].failCount--;
        return &tags_[i];
      }
    }

    for (size_t i = 0; i < MAX_TAGS; ++i) {
      if (!tags_[i].used) {
        tags_[i].used = true;
        tags_[i].shortAddr = shortAddr;
        tags_[i].alive = true;
        tags_[i].lastSeenMs = millis();
        Serial.printf("[TAG] added short=0x%04X\n", shortAddr);
        return &tags_[i];
      }
    }
    return nullptr;
  }

  // Called when AT mode wants to accept tag UWB
  void acceptTagUwbSession(uint16_t tagShort, uint32_t seq) {
    TagSlot* t = ensureTag(tagShort);
    if (!t) return;
    activeTag_ = t;
    activeAA_ = nullptr;
    runState_ = RunState::MATCHED;
    session_.startResponderAT(activeTag_, seq);
  }

private:
  AnchorMode currentMode_ = AnchorMode::AT;
  AnchorMode pendingMode_ = AnchorMode::AT;
  bool modeDirty_ = false;

  RunState runState_ = RunState::FREE;
  UwbTwrSession session_;

  AnchorSlot anchors_[MAX_ANCHORS];
  TagSlot tags_[MAX_TAGS];

  AnchorSlot* activeAA_ = nullptr;
  TagSlot* activeTag_ = nullptr;

  uint32_t seqCounter_ = 1;
  uint32_t requestedMeshNodeId_ = 0;
  uint32_t requestedSeq_ = 0;
  uint32_t requestStartedAtMs_ = 0;
  uint32_t backoffUntilMs_ = 0;
  uint32_t masterNodeId_ = MASTER_NODE_ID;

private:
  AnchorSlot* ensureAnchor(uint32_t meshNodeId, uint16_t shortAddr) {
    for (size_t i = 0; i < MAX_ANCHORS; ++i) {
      if (anchors_[i].used && anchors_[i].meshNodeId == meshNodeId) {
        anchors_[i].alive = true;
        if (anchors_[i].failCount > 0) anchors_[i].failCount--;
        anchors_[i].dev.noteActivity();

        byte sa[2];
        shortToBytes(shortAddr, sa);
        anchors_[i].dev.setShortAddress(sa);
        anchors_[i].dev.setReplyTime(REPLY_DELAY_US);
        return &anchors_[i];
      }
    }

    for (size_t i = 0; i < MAX_ANCHORS; ++i) {
      if (!anchors_[i].used) {
        anchors_[i].used = true;
        anchors_[i].meshNodeId = meshNodeId;
        anchors_[i].alive = true;
        byte sa[2];
        shortToBytes(shortAddr, sa);
        anchors_[i].dev.setShortAddress(sa);
        anchors_[i].dev.setReplyTime(REPLY_DELAY_US);
        anchors_[i].dev.noteActivity();
        Serial.printf("[AA] added anchor mesh=%u short=0x%04X\n",
                      (unsigned)meshNodeId, shortAddr);
        return &anchors_[i];
      }
    }
    return nullptr;
  }

  AnchorSlot* findAnchor(uint32_t meshNodeId) {
    for (size_t i = 0; i < MAX_ANCHORS; ++i) {
      if (anchors_[i].used && anchors_[i].meshNodeId == meshNodeId) return &anchors_[i];
    }
    return nullptr;
  }

  void removeAnchor(uint32_t meshNodeId) {
    for (size_t i = 0; i < MAX_ANCHORS; ++i) {
      if (anchors_[i].used && anchors_[i].meshNodeId == meshNodeId) {
        anchors_[i] = AnchorSlot{};
        return;
      }
    }
  }

  void removeTag(uint16_t shortAddr) {
    for (size_t i = 0; i < MAX_TAGS; ++i) {
      if (tags_[i].used && tags_[i].shortAddr == shortAddr) {
        tags_[i] = TagSlot{};
        return;
      }
    }
  }

  void expireAnchors() {
    for (size_t i = 0; i < MAX_ANCHORS; ++i) {
      if (!anchors_[i].used) continue;
      if (anchors_[i].dev.isInactive()) {
        anchors_[i].alive = false;
        anchors_[i].failCount++;
      }
    }
  }

  void expireTags() {
    const uint32_t now = millis();
    for (size_t i = 0; i < MAX_TAGS; ++i) {
      if (!tags_[i].used) continue;
      if (now - tags_[i].lastSeenMs > 10000) {
        tags_[i].alive = false;
        tags_[i].failCount++;
      }
    }
  }

  void pruneFailedAnchors() {
    for (size_t i = 0; i < MAX_ANCHORS; ++i) {
      if (anchors_[i].used && anchors_[i].failCount >= MAX_FAIL_COUNT) {
        uint32_t id = anchors_[i].meshNodeId;
        removeAnchor(id);
        --i;
      }
    }
  }

  void pruneFailedTags() {
    for (size_t i = 0; i < MAX_TAGS; ++i) {
      if (tags_[i].used && tags_[i].failCount >= MAX_FAIL_COUNT) {
        uint16_t sa = tags_[i].shortAddr;
        removeTag(sa);
        --i;
      }
    }
  }

  uint32_t edgeAgeMs(const AnchorSlot& n, uint32_t now) const {
    return n.lastUpdateMs == 0 ? now : now - n.lastUpdateMs;
  }

  int chooseOldestAnchor(uint32_t now) {
    int best = -1;
    uint32_t bestAge = 0;

    for (size_t i = 0; i < MAX_ANCHORS; ++i) {
      if (!anchors_[i].used || !anchors_[i].alive) continue;
      uint32_t age = edgeAgeMs(anchors_[i], now);
      if (best < 0 || age > bestAge ||
          (age == bestAge && anchors_[i].meshNodeId < anchors_[best].meshNodeId)) {
        best = (int)i;
        bestAge = age;
      }
    }
    return best;
  }

  void applyPendingModeIfSafe() {
    if (!modeDirty_) return;
    if (runState_ != RunState::FREE) return;
    currentMode_ = pendingMode_;
    modeDirty_ = false;

    StaticJsonDocument<96> doc;
    doc["t"] = "MODE";
    doc["m"] = (currentMode_ == AnchorMode::AA) ? "AA" : "AT";
    String out;
    serializeJson(doc, out);
    mesh.sendBroadcast(out);
  }

  void tryScheduleAA(uint32_t now) {
    int idx = chooseOldestAnchor(now);
    if (idx < 0) return;

    StaticJsonDocument<128> doc;
    doc["t"] = "REQ_AA";
    doc["seq"] = seqCounter_;
    doc["sa"] = SELF_SHORT;
    String out;
    serializeJson(doc, out);

    if (mesh.sendSingle(anchors_[idx].meshNodeId, out)) {
      requestedMeshNodeId_ = anchors_[idx].meshNodeId;
      requestedSeq_ = seqCounter_;
      seqCounter_++;
      requestStartedAtMs_ = now;
      runState_ = RunState::REQUESTING;
    }
  }

  void handleReqAA(uint32_t from, JsonDocument& doc) {
    AnchorSlot* n = findAnchor(from);
    if (!n) {
      uint16_t sa = doc["sa"] | 0;
      n = ensureAnchor(from, sa);
    }
    if (!n) return;

    uint32_t seq = doc["seq"] | 0;

    if (currentMode_ != AnchorMode::AA || runState_ != RunState::FREE) {
      sendRejAA(from, seq);
      return;
    }

    activeAA_ = n;
    activeTag_ = nullptr;
    runState_ = RunState::MATCHED;
    sendAccAA(from, seq);

    // requester initiates
    session_.startResponderAA(&activeAA_->dev, seq);
  }

  void handleAccAA(uint32_t from, JsonDocument& doc) {
    if (runState_ != RunState::REQUESTING) return;
    uint32_t seq = doc["seq"] | 0;
    if (from != requestedMeshNodeId_ || seq != requestedSeq_) return;

    AnchorSlot* n = findAnchor(from);
    if (!n) {
      resetRequesting();
      enterBackoff(millis());
      return;
    }

    activeAA_ = n;
    activeTag_ = nullptr;
    runState_ = RunState::MATCHED;
    session_.startInitiatorAA(&activeAA_->dev, seq);
  }

  void handleRejAA(uint32_t from, JsonDocument& doc) {
    if (runState_ != RunState::REQUESTING) return;
    uint32_t seq = doc["seq"] | 0;
    if (from == requestedMeshNodeId_ && seq == requestedSeq_) {
      resetRequesting();
      enterBackoff(millis());
    }
  }

  void handleDoneAA(uint32_t from) {
    AnchorSlot* n = findAnchor(from);
    if (!n) return;
    n->lastUpdateMs = millis();
    n->alive = true;
    n->dev.noteActivity();

    if (activeAA_ == n) {
      activeAA_ = nullptr;
      runState_ = RunState::FREE;
    }
  }

  void onSessionFinished(uint32_t now) {
    if (activeAA_) {
      activeAA_->lastUpdateMs = now;
      activeAA_->alive = true;
      activeAA_->failCount = 0;
      activeAA_->dev.noteActivity();
      sendDoneAA(activeAA_->meshNodeId);
    }

    if (activeTag_) {
      activeTag_->lastUpdateMs = now;
      activeTag_->alive = true;
      activeTag_->failCount = 0;
      activeTag_->lastRange = session_.lastRange();
      activeTag_->lastRxPower = session_.lastRxPower();
      activeTag_->lastFpPower = session_.lastFpPower();
      activeTag_->lastQuality = session_.lastQuality();
      activeTag_->tagValue1 = session_.lastTagValue1();
      activeTag_->tagValue2 = session_.lastTagValue2();
      uplinkTagResult(*activeTag_);
    }

    activeAA_ = nullptr;
    activeTag_ = nullptr;
    runState_ = RunState::FREE;
  }

  void onSessionFailed(uint32_t now) {
    if (activeAA_) activeAA_->failCount++;
    if (activeTag_) activeTag_->failCount++;
    activeAA_ = nullptr;
    activeTag_ = nullptr;
    runState_ = RunState::FREE;
    enterBackoff(now);
  }

  void uplinkTagResult(const TagSlot& tag) {
    StaticJsonDocument<256> doc;
    doc["t"] = "AT_RESULT";
    doc["anchorShort"] = SELF_SHORT;
    doc["tagShort"] = tag.shortAddr;
    doc["range"] = tag.lastRange;
    doc["rxPower"] = tag.lastRxPower;
    doc["fpPower"] = tag.lastFpPower;
    doc["quality"] = tag.lastQuality;
    doc["tagValue1"] = tag.tagValue1;
    doc["tagValue2"] = tag.tagValue2;
    doc["ts"] = millis();

    String out;
    serializeJson(doc, out);
    mesh.sendSingle(masterNodeId_, out);
  }

  void resetRequesting() {
    requestedMeshNodeId_ = 0;
    requestedSeq_ = 0;
    requestStartedAtMs_ = 0;
    if (runState_ == RunState::REQUESTING) runState_ = RunState::FREE;
  }

  void enterBackoff(uint32_t now) {
    runState_ = RunState::BACKOFF;
    backoffUntilMs_ = now + randRange32(BACKOFF_MIN_MS, BACKOFF_MAX_MS);
  }

  void sendAccAA(uint32_t to, uint32_t seq) {
    StaticJsonDocument<96> doc;
    doc["t"] = "ACC_AA";
    doc["seq"] = seq;
    String out;
    serializeJson(doc, out);
    mesh.sendSingle(to, out);
  }

  void sendRejAA(uint32_t to, uint32_t seq) {
    StaticJsonDocument<96> doc;
    doc["t"] = "REJ_AA";
    doc["seq"] = seq;
    String out;
    serializeJson(doc, out);
    mesh.sendSingle(to, out);
  }

  void sendDoneAA(uint32_t to) {
    StaticJsonDocument<96> doc;
    doc["t"] = "DONE_AA";
    doc["seq"] = seqCounter_++;
    String out;
    serializeJson(doc, out);
    mesh.sendSingle(to, out);
  }
};

AnchorAgent agent;

// ============================================================
// Mesh callbacks
// ============================================================
void receivedCallback(uint32_t from, String &msg) {
  agent.onMeshMessage(from, msg);
}

void newConnectionCallback(uint32_t nodeId) {
  Serial.printf("[MESH] new connection: %u\n", nodeId);
}

void changedConnectionCallback() {
  Serial.println("[MESH] changed connections");
}

Task taskHeartbeat(TASK_SECOND * 3, TASK_FOREVER, []() {
  agent.broadcastHeartbeat();
});

// ============================================================
// External mode control
// ============================================================
void setAnchorModeExternal(AnchorMode mode) {
  agent.setModeExternal(mode);
}

// 예시: 외부 트리거로 tag UWB session 수락
// 실제 구현에서는 특정 UWB preamble / tag discovery / 별도 GPIO/event에 연결 가능
void onTagDetectedUwbRequest(uint16_t tagShort, uint32_t seq) {
  agent.acceptTagUwbSession(tagShort, seq);
}

// ============================================================
// Setup helpers
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

void setupMesh() {
  mesh.setDebugMsgTypes(ERROR | STARTUP);
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);

  userScheduler.addTask(taskHeartbeat);
  taskHeartbeat.enable();
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  randomSeed((uint32_t)esp_random());
  setupDw1000();
  setupMesh();
  agent.begin();

  setAnchorModeExternal(AnchorMode::AT);
  Serial.println("Anchor node started");
}

void loop() {
  mesh.update();
  agent.loop();

  // 테스트용 AA/AT 전환
  static uint32_t lastFlip = 0;
  if (millis() - lastFlip > 20000) {
    lastFlip = millis();
    static bool aa = false;
    aa = !aa;
    setAnchorModeExternal(aa ? AnchorMode::AA : AnchorMode::AT);
  }

  // 테스트용: AT에서 tag 요청 수신 가정
  // 실제 시스템에서는 UWB discovery/event로 바꿔야 함
  static uint32_t fakeTag = 0;
  if (millis() > 5000 && fakeTag == 0) {
    fakeTag = 1;
    onTagDetectedUwbRequest(0x0202, 1001);
  }
}
