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
#define MAX_MAC 64

static int unique_num = 0;
static uint8_t unique_mac[MAX_MAC][6];

// returns true if a new mac was added to the table of unique mac addresses
static bool add_mac(const uint8_t * mac)
{
    // try to find it
    int i;
    for (i = 0; i < unique_num; i++) {
        if (memcmp(mac, unique_mac[i], 6) == 0) {
            // not unique
            return false;
        }
    }

    // not found, add it if there's still room
    if (unique_num < MAX_MAC) {
        memcpy(&unique_mac[unique_num], mac, 6);
        unique_num++;
        return true;
    } else {
        // could not fit it
        return false;
    }
}

// prints a mac address
static void print_mac(const uint8_t * mac)
{
    char text[32];
    sprintf(text, "%02X:%02X:%02X:%02X:%02X:%02X",
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    Serial.println(text);
}

// callback for promiscuous mode
static void promisc_cb(uint8_t * buf, uint16_t len)
{
    uint8_t addr1[6];
    uint8_t addr2[6];
    uint8_t addr3[6];

    if (len == 128) {
        // skip header
        buf += 12;
        // skip FC and DUR
        buf += 4;
        // get address1, address2, address3
        memcpy(addr1, buf, 6);
        buf += 6;
        memcpy(addr2, buf, 6);
        buf += 6;
        memcpy(addr3, buf, 6);
        buf += 6;
        if (add_mac(addr2)) {
            print_mac(addr2);
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
