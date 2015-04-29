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
#include "pkt_stats.h"

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
    struct timeval time;

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
    pRingBufferEntry->flags = RING_BUFFER_ENTRY_FLAGS_HAS_BINARY |
                              RING_BUFFER_ENTRY_FLAGS_HAS_TIMESTAMP;
    pRingBufferEntry->type = ENTRY_TYPE_POWER_EVENT;
    gettimeofday(&time,NULL);
    pRingBufferEntry->timestamp = time.tv_usec + time.tv_sec * 1000 * 1000;

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

static wifi_error update_stats_to_ring_buf(hal_info *info,
                      u8 *rb_entry, u32 size)
{
    int num_records = 1;
    wifi_ring_buffer_entry *pRingBufferEntry =
        (wifi_ring_buffer_entry *)rb_entry;
    struct timeval time;

    pRingBufferEntry->entry_size = size - sizeof(wifi_ring_buffer_entry);
    pRingBufferEntry->flags = RING_BUFFER_ENTRY_FLAGS_HAS_BINARY |
                              RING_BUFFER_ENTRY_FLAGS_HAS_TIMESTAMP;
    pRingBufferEntry->type = ENTRY_TYPE_PKT;
    gettimeofday(&time,NULL);
    pRingBufferEntry->timestamp = time.tv_usec + time.tv_sec * 1000 * 1000;

    // Write if verbose and handler is set
    if ((info->rb_infos[PKT_STATS_RB_ID].verbose_level >= VERBOSE_DEBUG_PROBLEM)
        && info->on_ring_buffer_data)
        ring_buffer_write(&info->rb_infos[PKT_STATS_RB_ID],
                          (u8*)pRingBufferEntry,
                          size,
                          num_records);
    else
        ALOGV("Verbose level not set \n");


    return WIFI_SUCCESS;
}

static u16 get_rate(u16 mcs_r, u8 short_gi)
{
    u16 tx_rate = 0;
    MCS mcs;
    static u16 rate_lookup[][8] = {{96, 48, 24, 12, 108, 72, 36, 18},
                            {22, 11,  4,  2,  22, 11,  4,  0}};
    static u16 MCS_rate_lookup_ht[][8] = {{ 13,  14,  27,  30,  59,  65,  117,  130},
                                   { 26,  29,  54,  60, 117, 130,  234,  260},
                                   { 39,  43,  81,  90, 176, 195,  351,  390},
                                   { 52,  58, 108, 120, 234, 260,  468,  520},
                                   { 78,  87, 162, 180, 351, 390,  702,  780},
                                   {104, 116, 216, 240, 468, 520,  936, 1040},
                                   {117, 130, 243, 270, 527, 585, 1053, 1170},
                                   {130, 144, 270, 300, 585, 650, 1170, 1300},
                                   {156, 173, 324, 360, 702, 780, 1404, 1560},
                                   {  0,   0, 360, 400, 780, 867, 1560, 1733},
                                   { 26,  29,  54,  60, 117, 130,  234,  260},
                                   { 52,  58, 108, 120, 234, 260,  468,  520},
                                   { 78,  87, 162, 180, 351, 390,  702,  780},
                                   {104, 116, 216, 240, 468, 520,  936, 1040},
                                   {156, 173, 324, 360, 702, 780, 1404, 1560},
                                   {208, 231, 432, 480, 936,1040, 1872, 2080},
                                   {234, 261, 486, 540,1053,1170, 2106, 2340},
                                   {260, 289, 540, 600,1170,1300, 2340, 2600},
                                   {312, 347, 648, 720,1404,1560, 2808, 3120},
                                   {  0,   0, 720, 800,1560,1733, 3120, 3467}};

    mcs.mcs = mcs_r;
    if ((mcs.mcs_s.preamble < 4) && (mcs.mcs_s.rate < 10)) {
        switch(mcs.mcs_s.preamble)
        {
            case 0:
            case 1:
                if(mcs.mcs_s.rate<8) {
                    tx_rate = rate_lookup [mcs.mcs_s.preamble][mcs.mcs_s.rate];
                    if (mcs.mcs_s.nss)
                        tx_rate *=2;
                } else {
                    ALOGE("Unexpected rate value");
                }
            break;
            case 2:
                if(mcs.mcs_s.rate<8) {
                    if (!mcs.mcs_s.nss)
                        tx_rate = MCS_rate_lookup_ht[mcs.mcs_s.rate][2*mcs.mcs_s.bw+short_gi];
                    else
                        tx_rate = MCS_rate_lookup_ht[10+mcs.mcs_s.rate][2*mcs.mcs_s.bw+short_gi];
                } else {
                    ALOGE("Unexpected HT mcs.mcs_s index");
                }
            break;
            case 3:
                if (!mcs.mcs_s.nss)
                    tx_rate = MCS_rate_lookup_ht[mcs.mcs_s.rate][2*mcs.mcs_s.bw+short_gi];
                else
                    tx_rate = MCS_rate_lookup_ht[10+mcs.mcs_s.rate][2*mcs.mcs_s.bw+short_gi];
            break;
            default:
                ALOGE("Unexpected preamble");
        }
    }
    return tx_rate;
}

static u16 get_rx_rate(u16 mcs)
{
    /* TODO: guard interval is not specified currently */
    return get_rate(mcs, 0);
}

static wifi_error parse_rx_stats(hal_info *info, u8 *buf, u16 size)
{
    wifi_error status;
    rb_pkt_stats_t *rx_stats_rcvd = (rb_pkt_stats_t *)buf;
    u8 rb_pkt_entry_buf[RING_BUF_ENTRY_SIZE];
    wifi_ring_buffer_entry *pRingBufferEntry;
    u32 len_ring_buffer_entry = 0;

    len_ring_buffer_entry = sizeof(wifi_ring_buffer_entry)
                            + sizeof(wifi_ring_per_packet_status_entry)
                            + RX_HTT_HDR_STATUS_LEN;

    if (len_ring_buffer_entry > RING_BUF_ENTRY_SIZE) {
        pRingBufferEntry = (wifi_ring_buffer_entry *)malloc(
                len_ring_buffer_entry);
        if (pRingBufferEntry == NULL) {
            ALOGE("%s: Failed to allocate memory", __FUNCTION__);
            return WIFI_ERROR_OUT_OF_MEMORY;
        }
    } else {
        pRingBufferEntry = (wifi_ring_buffer_entry *)rb_pkt_entry_buf;
    }

    wifi_ring_per_packet_status_entry *rb_pkt_stats =
        (wifi_ring_per_packet_status_entry *)(pRingBufferEntry + 1);

    if (size != sizeof(rb_pkt_stats_t)) {
        ALOGE("%s Unexpected rx stats event length: %d", __FUNCTION__, size);
        return WIFI_ERROR_UNKNOWN;
    }

    memset(rb_pkt_stats, 0, sizeof(wifi_ring_per_packet_status_entry));

    /* Peer tx packet and it is an Rx packet for us */
    rb_pkt_stats->flags |= PER_PACKET_ENTRY_FLAGS_DIRECTION_TX;

    if (!rx_stats_rcvd->mpdu_end.tkip_mic_err)
        rb_pkt_stats->flags |= PER_PACKET_ENTRY_FLAGS_TX_SUCCESS;

    rb_pkt_stats->flags |= PER_PACKET_ENTRY_FLAGS_80211_HEADER;

    if (rx_stats_rcvd->mpdu_start.encrypted)
        rb_pkt_stats->flags |= PER_PACKET_ENTRY_FLAGS_PROTECTED;

    rb_pkt_stats->tid = rx_stats_rcvd->mpdu_start.tid;

    if (rx_stats_rcvd->ppdu_start.preamble_type == PREAMBLE_L_SIG_RATE) {
        if (!rx_stats_rcvd->ppdu_start.l_sig_rate_select)
            rb_pkt_stats->MCS |= 1 << 6;
        rb_pkt_stats->MCS |= rx_stats_rcvd->ppdu_start.l_sig_rate % 8;
        /*BW is 0 for legacy cases*/
    } else if (rx_stats_rcvd->ppdu_start.preamble_type ==
               PREAMBLE_VHT_SIG_A_1) {
        rb_pkt_stats->MCS |= 2 << 6;
        rb_pkt_stats->MCS |=
            (rx_stats_rcvd->ppdu_start.ht_sig_vht_sig_a_1 & BITMASK(7)) %8;
        rb_pkt_stats->MCS |=
            ((rx_stats_rcvd->ppdu_start.ht_sig_vht_sig_a_1 >> 7) & 1) << 8;
    } else if (rx_stats_rcvd->ppdu_start.preamble_type ==
               PREAMBLE_VHT_SIG_A_2) {
        rb_pkt_stats->MCS |= 3 << 6;
        rb_pkt_stats->MCS |=
            (rx_stats_rcvd->ppdu_start.ht_sig_vht_sig_a_2 >> 4) & BITMASK(4);
        rb_pkt_stats->MCS |=
            (rx_stats_rcvd->ppdu_start.ht_sig_vht_sig_a_1 & 3) << 8;
    }
    rb_pkt_stats->last_transmit_rate = get_rx_rate(rb_pkt_stats->MCS);

    rb_pkt_stats->rssi = rx_stats_rcvd->ppdu_start.rssi_comb;
    rb_pkt_stats->link_layer_transmit_sequence
        = rx_stats_rcvd->mpdu_start.seq_num;

    rb_pkt_stats->firmware_entry_timestamp
        = rx_stats_rcvd->ppdu_end.wb_timestamp;

    memcpy(&rb_pkt_stats->data[0], &rx_stats_rcvd->rx_hdr_status[0],
        RX_HTT_HDR_STATUS_LEN);

    status = update_stats_to_ring_buf(info, (u8 *)pRingBufferEntry,
                                      len_ring_buffer_entry);

    if (status != WIFI_SUCCESS) {
        ALOGE("Failed to write Rx stats into the ring buffer");
    }

    if ((u8 *)pRingBufferEntry != rb_pkt_entry_buf) {
        ALOGI("Message with more than RING_BUF_ENTRY_SIZE");
        free (pRingBufferEntry);
    }

    return status;
}

static void parse_tx_rate_and_mcs(struct tx_ppdu_start *ppdu_start,
                                wifi_ring_per_packet_status_entry *rb_pkt_stats)
{
    u16 tx_rate = 0, short_gi = 0;
    MCS mcs;

    if (ppdu_start->valid_s0_bw20) {
        short_gi = ppdu_start->s0_bw20.short_gi;
        mcs.mcs_s.rate      = ppdu_start->s0_bw20.rate;
        mcs.mcs_s.nss       = ppdu_start->s0_bw20.nss;
        mcs.mcs_s.preamble  = ppdu_start->s0_bw20.preamble_type;
        mcs.mcs_s.bw        = BW_20_MHZ;
    } else if (ppdu_start->valid_s0_bw40) {
        short_gi = ppdu_start->s0_bw40.short_gi;
        mcs.mcs_s.rate      = ppdu_start->s0_bw40.rate;
        mcs.mcs_s.nss       = ppdu_start->s0_bw40.nss;
        mcs.mcs_s.preamble  = ppdu_start->s0_bw40.preamble_type;
        mcs.mcs_s.bw        = BW_40_MHZ;
    } else if (ppdu_start->valid_s0_bw80) {
        short_gi = ppdu_start->s0_bw80.short_gi;
        mcs.mcs_s.rate      = ppdu_start->s0_bw80.rate;
        mcs.mcs_s.nss       = ppdu_start->s0_bw80.nss;
        mcs.mcs_s.preamble  = ppdu_start->s0_bw80.preamble_type;
        mcs.mcs_s.bw        = BW_80_MHZ;
    } else if (ppdu_start->valid_s0_bw160) {
        short_gi = ppdu_start->s0_bw160.short_gi;
        mcs.mcs_s.rate      = ppdu_start->s0_bw160.rate;
        mcs.mcs_s.nss       = ppdu_start->s0_bw160.nss;
        mcs.mcs_s.preamble  = ppdu_start->s0_bw160.preamble_type;
        mcs.mcs_s.bw        = BW_160_MHZ;
    } else if (ppdu_start->valid_s1_bw20) {
        short_gi = ppdu_start->s1_bw20.short_gi;
        mcs.mcs_s.rate      = ppdu_start->s1_bw20.rate;
        mcs.mcs_s.nss       = ppdu_start->s1_bw20.nss;
        mcs.mcs_s.preamble  = ppdu_start->s1_bw20.preamble_type;
        mcs.mcs_s.bw        = BW_20_MHZ;
    } else if (ppdu_start->valid_s1_bw40) {
        short_gi = ppdu_start->s1_bw40.short_gi;
        mcs.mcs_s.rate      = ppdu_start->s1_bw40.rate;
        mcs.mcs_s.nss       = ppdu_start->s1_bw40.nss;
        mcs.mcs_s.preamble  = ppdu_start->s1_bw40.preamble_type;
        mcs.mcs_s.bw        = BW_40_MHZ;
    } else if (ppdu_start->valid_s1_bw80) {
        short_gi = ppdu_start->s1_bw80.short_gi;
        mcs.mcs_s.rate      = ppdu_start->s1_bw80.rate;
        mcs.mcs_s.nss       = ppdu_start->s1_bw80.nss;
        mcs.mcs_s.preamble  = ppdu_start->s1_bw80.preamble_type;
        mcs.mcs_s.bw        = BW_80_MHZ;
    } else if (ppdu_start->valid_s1_bw160) {
        short_gi = ppdu_start->s1_bw160.short_gi;
        mcs.mcs_s.rate      = ppdu_start->s1_bw160.rate;
        mcs.mcs_s.nss       = ppdu_start->s1_bw160.nss;
        mcs.mcs_s.preamble  = ppdu_start->s1_bw160.preamble_type;
        mcs.mcs_s.bw        = BW_160_MHZ;
    }

    rb_pkt_stats->MCS = mcs.mcs;
    rb_pkt_stats->last_transmit_rate = get_rate(mcs.mcs, short_gi);
}

static wifi_error parse_tx_stats(hal_info *info, void *buf,
                                 u32 buflen, u8 logtype)
{
    wifi_error status;
    wifi_ring_buffer_entry *pRingBufferEntry =
        (wifi_ring_buffer_entry *)info->pkt_stats->tx_stats;

    wifi_ring_per_packet_status_entry *rb_pkt_stats =
        (wifi_ring_per_packet_status_entry *)(pRingBufferEntry + 1);

    ALOGV("Received Tx stats: log_type : %d", logtype);
    switch (logtype)
    {
        case PKTLOG_TYPE_TX_CTRL:
        {
            if (buflen != sizeof (wh_pktlog_txctl)) {
                ALOGE("Unexpected tx_ctrl event length: %d", buflen);
                return WIFI_ERROR_UNKNOWN;
            }

            wh_pktlog_txctl *stats = (wh_pktlog_txctl *)buf;
            struct tx_ppdu_start *ppdu_start =
                (struct tx_ppdu_start *)(&stats->u.ppdu_start);

            if (ppdu_start->frame_control & BIT(DATA_PROTECTED))
                rb_pkt_stats->flags |=
                    PER_PACKET_ENTRY_FLAGS_PROTECTED;
            rb_pkt_stats->link_layer_transmit_sequence
                = ppdu_start->start_seq_num;
            rb_pkt_stats->tid = ppdu_start->qos_ctl & 0xF;
            parse_tx_rate_and_mcs(ppdu_start, rb_pkt_stats);
            info->pkt_stats->tx_stats_events |=  BIT(PKTLOG_TYPE_TX_CTRL);
        }
        break;
        case PKTLOG_TYPE_TX_STAT:
        {
            if (buflen != sizeof(struct tx_ppdu_end)) {
                ALOGE("Unexpected tx_stat event length: %d", buflen);
                return WIFI_ERROR_UNKNOWN;
            }

            /* This should be the first event for tx-stats: So,
             * previous stats are invalid. Flush the old stats and treat
             * this as new packet
             */
            if (info->pkt_stats->tx_stats_events)
                memset(rb_pkt_stats, 0,
                        sizeof(wifi_ring_per_packet_status_entry));

            struct tx_ppdu_end *tx_ppdu_end = (struct tx_ppdu_end*)(buf);

            if (tx_ppdu_end->stat.tx_ok)
                rb_pkt_stats->flags |=
                    PER_PACKET_ENTRY_FLAGS_TX_SUCCESS;
            rb_pkt_stats->transmit_success_timestamp =
                tx_ppdu_end->try_list.try_00.timestamp;
            rb_pkt_stats->rssi = tx_ppdu_end->stat.ack_rssi_ave;
            rb_pkt_stats->num_retries =
                tx_ppdu_end->stat.total_tries;

            info->pkt_stats->tx_stats_events =  BIT(PKTLOG_TYPE_TX_STAT);
        }
        break;
        case PKTLOG_TYPE_RC_UPDATE:
        case PKTLOG_TYPE_TX_MSDU_ID:
        case PKTLOG_TYPE_TX_FRM_HDR:
        case PKTLOG_TYPE_RC_FIND:
        case PKTLOG_TYPE_TX_VIRT_ADDR:
            ALOGV("%s : Unsupported log_type received : %d",
                  __FUNCTION__, logtype);
        break;
        default:
        {
            ALOGV("%s : Unexpected log_type received : %d",
                  __FUNCTION__, logtype);
            return WIFI_ERROR_UNKNOWN;
        }
    }

    if ((info->pkt_stats->tx_stats_events &  BIT(PKTLOG_TYPE_TX_CTRL))&&
        (info->pkt_stats->tx_stats_events &  BIT(PKTLOG_TYPE_TX_STAT))) {
        /* No tx payload as of now, add the length to parameter size(3rd)
         * if there is any payload
         */
        status = update_stats_to_ring_buf(info,
                                          (u8 *)pRingBufferEntry,
                                     sizeof(wifi_ring_buffer_entry) +
                                     sizeof(wifi_ring_per_packet_status_entry));

        /* Flush the local copy after writing the stats to ring buffer
         * for tx-stats.
         */
        info->pkt_stats->tx_stats_events = 0;
        memset(rb_pkt_stats, 0,
                sizeof(wifi_ring_per_packet_status_entry));

        if (status != WIFI_SUCCESS) {
            ALOGE("Failed to write into the ring buffer: %d", logtype);
            return status;
        }
    }

    return WIFI_SUCCESS;
}

static wifi_error parse_stats_record(hal_info *info, u8 *buf, u16 record_type,
                              u16 record_len)
{
    wifi_error status;
    if (record_type == PKTLOG_TYPE_RX_STAT) {
        status = parse_rx_stats(info, buf, record_len);
    } else {
        status = parse_tx_stats(info, buf, record_len, record_type);
    }
    return status;
}

static wifi_error parse_stats(hal_info *info, u8 *data, u32 buflen)
{
    wh_pktlog_hdr_t *pkt_stats_header;
    wifi_error status = WIFI_SUCCESS;

    do {
        if (buflen < sizeof(wh_pktlog_hdr_t)) {
            status = WIFI_ERROR_INVALID_ARGS;
            break;
        }

        pkt_stats_header = (wh_pktlog_hdr_t *)data;

        if (buflen < (sizeof(wh_pktlog_hdr_t) + pkt_stats_header->size)) {
            status = WIFI_ERROR_INVALID_ARGS;
            break;
        }
        status = parse_stats_record(info,
                                    (u8 *)(pkt_stats_header + 1),
                                    pkt_stats_header->log_type,
                                    pkt_stats_header->size);
        if (status != WIFI_SUCCESS) {
            ALOGE("Failed to parse the stats type : %d",
                  pkt_stats_header->log_type);
            return status;
        }
        data += (sizeof(wh_pktlog_hdr_t) + pkt_stats_header->size);
        buflen -= (sizeof(wh_pktlog_hdr_t) + pkt_stats_header->size);
    } while (buflen > 0);

    return status;
}

wifi_error diag_message_handler(hal_info *info, nl_msg *msg)
{
    tAniNlHdr *wnl = (tAniNlHdr *)nlmsg_hdr(msg);
    u8 *buf;
    wifi_error status;

    //ALOGD("event sub type = %x", wnl->wmsg.type);

    if (wnl->wmsg.type == ANI_NL_MSG_LOG_HOST_EVENT_LOG_TYPE) {
        uint32_t diag_host_type;

        buf = (uint8_t *)(wnl + 1);
        diag_host_type = *(uint32_t *)(buf);
        ALOGV("diag type = %d", diag_host_type);

        buf +=  sizeof(uint32_t); //diag_type
        if (diag_host_type == DIAG_TYPE_HOST_EVENTS) {
            host_event_hdr_t *event_hdr =
                          (host_event_hdr_t *)(buf);
            ALOGV("diag event_id = %d length %d",
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
        } else if (diag_host_type == DIAG_TYPE_HOST_LOG_MSGS) {
            drv_msg_t *drv_msg = (drv_msg_t *) (buf);
            ALOGV("diag event_type = %0x length = %d",
                  drv_msg->event_type, drv_msg->length);
            if (drv_msg->event_type == WLAN_PKT_LOG_STATS) {
                if ((info->pkt_stats->prev_seq_no + 1) !=
                        drv_msg->u.pkt_stats_event.msg_seq_no) {
                    ALOGE("Few pkt stats messages missed: rcvd = %d, prev = %d",
                            drv_msg->u.pkt_stats_event.msg_seq_no,
                            info->pkt_stats->prev_seq_no);
                    if (info->pkt_stats->tx_stats_events) {
                        info->pkt_stats->tx_stats_events = 0;
                        memset(&info->pkt_stats->tx_stats, 0,
                                sizeof(wifi_ring_per_packet_status_entry));
                    }
                }

                info->pkt_stats->prev_seq_no =
                    drv_msg->u.pkt_stats_event.msg_seq_no;
                status = parse_stats(info,
                        drv_msg->u.pkt_stats_event.payload,
                        drv_msg->u.pkt_stats_event.payload_len);
                if (status != WIFI_SUCCESS) {
                    ALOGE("%s: Failed to parse Tx-Rx stats", __FUNCTION__);
                    ALOGE("Received msg Seq_num : %d",
                            drv_msg->u.pkt_stats_event.msg_seq_no);
                    hexdump((char *)drv_msg->u.pkt_stats_event.payload,
                            drv_msg->u.pkt_stats_event.payload_len);
                    return status;
                }
            }
        }
    }

//    hexdump((char *)nlmsg_data(nlhdr), nlhdr->nlmsg_len);
    return WIFI_SUCCESS;
}
