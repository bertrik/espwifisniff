#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

#include "Arduino.h"
#include "ESP8266WiFi.h"

extern "C" {
#include <user_interface.h>
}
// maximum number of unique mac addresses to remember
#define MAX_MAC 200

static int unique_num = 0;

typedef struct {
    uint8_t mac[6];
    unsigned long last_seen;
} mac_t;

static mac_t unique_mac[MAX_MAC];

// prints a mac address
static void print_mac(const uint8_t * mac)
{
    char text[32];
    sprintf(text, "%02X:%02X:%02X:%02X:%02X:%02X",
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    Serial.print(text);
}

// returns true if a new mac was added to the table of unique mac addresses
static bool add_mac(unsigned long now, const uint8_t * mac)
{
    // try to find it
    int i;
    for (i = 0; i < unique_num; i++) {
        if (memcmp(mac, unique_mac[i].mac, 6) == 0) {
            // found it, update timestamp
            unique_mac[i].last_seen = now;
            return false;
        }
    }

    // not found, add it if there's still room
    if (unique_num < MAX_MAC) {
        memcpy(&unique_mac[unique_num], mac, 6);
        unique_mac[unique_num].last_seen = now;
        unique_num++;
        return true;
    } else {
        // could not fit it
        return false;
    }
}

// expires old mac entries
static void expire_mac(unsigned long now, unsigned long expiry)
{
    char text[32];
    int i;
    for (i = 0; i < unique_num; i++) {
        if ((now - unique_mac[i].last_seen) > expiry) {
            // expired, overwrite it with the last one
            if (--unique_num > i) {
                sprintf(text, "%10d: ", now);
                Serial.print(text);
                print_mac(unique_mac[i].mac);
                sprintf(text, " expired: %d\n", unique_num);
                Serial.print(text);
                memcpy(&unique_mac[i], &unique_mac[unique_num], sizeof(mac_t));
            }
        }
    }
}

// callback for promiscuous mode
static void promisc_cb(uint8_t * buf, uint16_t len)
{
    uint8_t addr1[6];
    uint8_t addr2[6];
    uint8_t addr3[6];
    char text[32];

    if (len >= 34) {
        // skip header
        buf += 12;

        // read FC and DUR
        uint16_t fc = buf[0] + buf[1] << 8;
        buf += 2;
        uint16_t dur = buf[0] + buf[1] << 8;
        buf += 2;

        // get address1, address2, address3
        memcpy(addr1, buf, 6);
        buf += 6;
        memcpy(addr2, buf, 6);
        buf += 6;
        memcpy(addr3, buf, 6);
        buf += 6;

        // check for packet with FromDS=0, packets from station
        if (((fc >> 9) & 1) == 0) {
            unsigned long now = millis();
            expire_mac(now, 60000);
            if (add_mac(now, addr2)) {
                sprintf(text, "%10d: ", now);
                Serial.print(text);
                print_mac(addr2);
                sprintf(text, " added: %d\n", unique_num);
                Serial.print(text);
            }
        }
    }
}

// enables promiscuous mode on a specific channel
static void enable_promisc(int channel)
{
    // Set up promiscuous callback
    wifi_set_channel(channel);
    wifi_promiscuous_enable(0);
    wifi_set_promiscuous_rx_cb(promisc_cb);
    wifi_promiscuous_enable(1);
}

void setup(void)
{
    Serial.begin(115200);
    Serial.println("ESP SNIFFER!\n");

    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

    enable_promisc(6);
}

void loop(void)
{
}
