#include "uf2.h"

#if USE_HID

typedef struct {
    PacketBuffer pbuf;
    uint16_t size;
    uint8_t serial;
    uint8_t ep;
    union {
        uint8_t buf[FLASH_ROW_SIZE + 64];
        uint32_t buf32[(FLASH_ROW_SIZE + 64) / 4];
        uint16_t buf16[(FLASH_ROW_SIZE + 64) / 2];
    };
} HID_InBuffer;

#if USE_HID
static HID_InBuffer hidbufData;
#endif

void process_hid() {
#if USE_HID
    hidbufData.ep = USB_EP_HID;
#endif
}

#endif