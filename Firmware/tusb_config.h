#ifndef _TUSB_CONFIG_H_
#define _TUSB_CONFIG_H_

#define CFG_TUSB_MCU               OPT_MCU_RP2040
#define CFG_TUSB_RHPORT0_MODE      OPT_MODE_DEVICE
#define CFG_TUD_ENDPOINT0_SIZE     64
#define CFG_TUD_CDC_EP_BUFSIZE  256
#define CFG_TUD_CDC_TX_BUFSIZE  512
#define CFG_TUD_CDC_RX_BUFSIZE  512

#define CFG_TUD_CDC                1

#endif