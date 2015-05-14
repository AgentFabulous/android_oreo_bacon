/* Copyright (c) 2015, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 *  * Neither the name of The Linux Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */



#include <netlink/genl/genl.h>
#include <netlink/genl/family.h>
#include <netlink/genl/ctrl.h>
#include <linux/rtnetlink.h>
#include "wifiloggercmd.h"
#include "wifilogger_diag.h"

tlv_log* addLoggerTlv(u16 type, u16 length, u8* value, tlv_log *pOutTlv)
{

   pOutTlv->tag = type;
   pOutTlv->length = length;
   memcpy(&pOutTlv->value[0], value, length);

   return((tlv_log *)((u8 *)pOutTlv + sizeof(tlv_log) + length));
}

static int process_wakelock_event(hal_info *info, u8* buf, int length)
{
    wlan_wake_lock_event_t *pWlanWakeLockEvent;
    wake_lock_event *pWakeLockEvent = NULL;
    wifi_power_event *pPowerEvent = NULL;
    wifi_ring_buffer_entry *pRingBufferEntry = NULL;
    int len_wakelock_event = 0, len_power_event = 0, ret = 0;
    int len_ring_buffer_entry = 0, num_records = 0;
    tlv_log *pTlv;

    ALOGD("Received a wake lock event");

    pWlanWakeLockEvent = (wlan_wake_lock_event_t *)(buf);
    ALOGD("wle status = %d reason %d timeout %d name_len %d name %s \n",
          pWlanWakeLockEvent->status, pWlanWakeLockEvent->reason,
          pWlanWakeLockEvent->timeout, pWlanWakeLockEvent->name_len,
          pWlanWakeLockEvent->name);

    len_wakelock_event = sizeof(wake_lock_event) +
                            pWlanWakeLockEvent->name_len + 1;

    pWakeLockEvent = (wake_lock_event *)malloc(len_wakelock_event);
    if (pWakeLockEvent == NULL) {
        ALOGE("%s: Failed to allocate memory", __func__);
        goto cleanup;
    }

    memset(pWakeLockEvent, 0, len_wakelock_event);

    pWakeLockEvent->status = pWlanWakeLockEvent->status;
    pWakeLockEvent->reason = pWlanWakeLockEvent->reason;
    memcpy(pWakeLockEvent->name, pWlanWakeLockEvent->name,
           pWlanWakeLockEvent->name_len);

    len_power_event = sizeof(wifi_power_event) +
                          sizeof(tlv_log) + len_wakelock_event;
    pPowerEvent = (wifi_power_event *)malloc(len_power_event);
    if (pPowerEvent == NULL) {
        ALOGE("%s: Failed to allocate memory", __func__);
        goto cleanup;
    }

    memset(pPowerEvent, 0, len_power_event);
    pPowerEvent->event = WIFI_TAG_WAKE_LOCK_EVENT;

    pTlv = &pPowerEvent->tlvs[0];
    addLoggerTlv(WIFI_TAG_WAKE_LOCK_EVENT, len_wakelock_event,
                 (u8*)pWakeLockEvent, pTlv);
    len_ring_buffer_entry = sizeof(wifi_ring_buffer_entry) + len_power_event;
    pRingBufferEntry = (wifi_ring_buffer_entry *)malloc(
                                     len_ring_buffer_entry);
    if (pRingBufferEntry == NULL) {
        ALOGE("%s: Failed to allocate memory", __func__);
        goto cleanup;
    }
    memset(pRingBufferEntry, 0, len_ring_buffer_entry);

    pRingBufferEntry->entry_size = len_power_event;
    pRingBufferEntry->flags = 0;
    pRingBufferEntry->type = ENTRY_TYPE_POWER_EVENT;
    pRingBufferEntry->timestamp = 0;

    memcpy(pRingBufferEntry + 1, pPowerEvent, len_power_event);
    ALOGI("Ring buffer Length %d \n", len_ring_buffer_entry);

    // Write if verbose and handler is set
    num_records = 1;
    if (info->rb_infos[POWER_EVENTS_RB_ID].verbose_level >= 1 &&
        info->on_ring_buffer_data)
        ring_buffer_write(&info->rb_infos[POWER_EVENTS_RB_ID],
                      (u8*)pRingBufferEntry, len_ring_buffer_entry, num_records);
    else
        ALOGI("Verbose level not set \n");
cleanup:
    if (pWakeLockEvent)
        free(pWakeLockEvent);
    if (pPowerEvent)
        free(pPowerEvent);
    if (pRingBufferEntry)
        free(pRingBufferEntry);
    return ret;
}

int diag_message_handler(hal_info *info, nl_msg *msg)
{
    tAniNlHdr *wnl = (tAniNlHdr *)nlmsg_hdr(msg);
    u8 *buf;

    ALOGD("event sub type = %x", wnl->wmsg.type);

    if (wnl->wmsg.type == ANI_NL_MSG_LOG_HOST_EVENT_LOG_TYPE) {
        uint32_t diag_host_type;

        buf = (uint8_t *)(wnl + 1);
        diag_host_type = *(uint32_t *)(buf);
        ALOGD("diag type = %d", diag_host_type);

        if (diag_host_type == DIAG_TYPE_HOST_EVENTS) {
            buf +=  sizeof(uint32_t); //diag_type
            host_event_hdr_t *event_hdr =
                          (host_event_hdr_t *)(buf);
            ALOGD("diag event_id = %d length %d",
                  event_hdr->event_id, event_hdr->length);
            buf += sizeof(host_event_hdr_t);
            switch (event_hdr->event_id) {
                case EVENT_WLAN_WAKE_LOCK:
                    process_wakelock_event(info, buf, event_hdr->length);
                    break;
                default:
                    ALOGD("Unsupported Event %d \n", event_hdr->event_id);
                    break;
            }
        }
    }

//    hexdump((char *)nlmsg_data(nlhdr), nlhdr->nlmsg_len);
    return NL_OK;
}
