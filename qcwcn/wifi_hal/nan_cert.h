/*
 * Copyright (c) 2016, The Linux Foundation. All rights reserved.

 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.

 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __NAN_CERT_H__
#define __NAN_CERT_H__


#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

typedef struct {
    /* NAN master rank being advertised by DE */
    u64 master_rank;
    /* NAN master preference being advertised by DE */
    u8 master_pref;
    /* random value being advertised by DE */
    u8 random_factor;
    /* hop_count from anchor master */
    u8 hop_count;
    u32 beacon_transmit_time;
    /* NDP channel Frequency */
    u32 ndp_channel_freq;
} NanStaParameter;

typedef struct PACKED
{
    /*
      Valid bit values:
      0 - Invalidates any current and previous configuration of nan availability
      1 - Current configuration is valid
    */
    u32 valid:1;
    /*
      2g_band_availability bit values
      0 - 2G band all channel and all slots not available
      1 - 2G band all channel and all slots available
    */
    u32 band_availability_2g:1;
    /*
      5g_band_availability bit values
      0 - 5G band all channel and all slots not available
      1 - 5G band all channel and all slots available
    */
    u32 band_availability_5g:1;
    u32 reserved:29;
} NanAvailabilityDebug;

/*
   Function to get the sta_parameter expected by Sigma
   as per CAPI spec.
*/
wifi_error nan_get_sta_parameter(transaction_id id,
                                 wifi_interface_handle iface,
                                 NanStaParameter* msg);

wifi_error nan_availability_config(transaction_id id,
                                   wifi_interface_handle iface,
                                   NanAvailabilityDebug msg);
#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* __NAN_CERT_H__ */

