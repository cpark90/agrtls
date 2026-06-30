/*
 * mesh_link.h  (ESP32 only -- uses WiFi/ESP-NOW)
 *
 * Control-plane transport for the L2/L3 coordination messages. F-b uses ESP-NOW broadcast:
 * connectionless, no AP/root, single-hop -- which fits local (neighboring-anchor) coordination.
 * Multi-hop ESP-WIFI-MESH is only needed later for L5 result aggregation.
 *
 * Received frames are pushed to a FreeRTOS queue from the ESP-NOW receive callback (WiFi task
 * context, core 0) and drained via poll() in loop() (core 1) -> all module access stays single-
 * threaded in loop().
 *
 * Note: all anchors must share one WiFi channel (set here). UWB (DW1000) is a separate radio.
 */

#ifndef MESH_LINK_H
#define MESH_LINK_H

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <string.h>

#ifndef MESH_RX_QUEUE_LEN
#define MESH_RX_QUEUE_LEN 16
#endif
#ifndef MESH_WIFI_CHANNEL
#define MESH_WIFI_CHANNEL 1
#endif
// Generic transport frame size (independent of the message set). Must hold the largest message any
// variant sends/receives over the shared channel (window TAGINFO is the biggest at ~116 B).
#ifndef MESH_LINK_MAX_FRAME
#define MESH_LINK_MAX_FRAME 128
#endif

class MeshLink {
public:
    struct Frame { uint8_t len; uint8_t data[MESH_LINK_MAX_FRAME]; };

    bool begin(uint8_t channel = MESH_WIFI_CHANNEL) {
        _queue = xQueueCreate(MESH_RX_QUEUE_LEN, sizeof(Frame));
        _self  = this;

        WiFi.mode(WIFI_STA);
        WiFi.disconnect();
        esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);

        if (esp_now_init() != ESP_OK) return false;
        esp_now_register_recv_cb(onRecv);

        esp_now_peer_info_t peer = {};
        memset(peer.peer_addr, 0xFF, 6);   // broadcast
        peer.channel = channel;
        peer.encrypt = false;
        esp_now_add_peer(&peer);
        return true;
    }

    void send(const uint8_t* data, uint8_t len) {
        static const uint8_t bcast[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
        esp_now_send(bcast, data, len);
    }

    // Drain one received frame; returns false if the queue is empty.
    bool poll(Frame& out) {
        return _queue && xQueueReceive(_queue, &out, 0) == pdTRUE;
    }

private:
    static MeshLink*    _self;
    QueueHandle_t       _queue = nullptr;

    // ESP-NOW receive callback (arduino-esp32 2.x signature). Runs in WiFi task context.
    static void onRecv(const uint8_t* /*mac*/, const uint8_t* data, int len) {
        if (!_self || !_self->_queue || len <= 0 || len > (int)MESH_LINK_MAX_FRAME) return;
        Frame f;
        f.len = (uint8_t)len;
        memcpy(f.data, data, len);
        xQueueSend(_self->_queue, &f, 0);   // non-blocking; drop if full
    }
};

// Single-TU use (only the meshagent variant includes this header).
MeshLink* MeshLink::_self = nullptr;

#endif // MESH_LINK_H
