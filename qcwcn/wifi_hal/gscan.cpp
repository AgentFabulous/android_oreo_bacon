/*
 * Copyright (C) 2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "sync.h"
#define LOG_TAG  "WifiHAL"
#include <utils/Log.h>
#include <errno.h>
#include <time.h>

#include "common.h"
#include "cpp_bindings.h"
#include "gscancommand.h"
#include "gscan_event_handler.h"

#define GSCAN_EVENT_WAIT_TIME_SECONDS 4

/* Used to handle gscan command events from driver/firmware. */
GScanCommandEventHandler *GScanStartCmdEventHandler = NULL;
GScanCommandEventHandler *GScanSetBssidHotlistCmdEventHandler = NULL;
GScanCommandEventHandler *GScanSetSignificantChangeCmdEventHandler = NULL;
GScanCommandEventHandler *GScanSetSsidHotlistCmdEventHandler = NULL;
GScanCommandEventHandler *GScanSetPnoListCmdEventHandler = NULL;
GScanCommandEventHandler *GScanPnoSetPasspointListCmdEventHandler = NULL;

/* Implementation of the API functions exposed in gscan.h */
wifi_error wifi_get_valid_channels(wifi_interface_handle handle,
       int band, int max_channels, wifi_channel *channels, int *num_channels)
{
    int requestId, ret = 0;
    GScanCommand *gScanCommand;
    struct nlattr *nlData;
    interface_info *ifaceInfo = getIfaceInfo(handle);
    wifi_handle wifiHandle = getWifiHandle(handle);
    hal_info *info = getHalInfo(wifiHandle);

    if (!(info->supported_feature_set & WIFI_FEATURE_GSCAN)) {
        ALOGE("%s: GSCAN is not supported by driver",
            __func__);
        return WIFI_ERROR_NOT_SUPPORTED;
    }

    if (channels == NULL) {
        ALOGE("%s: NULL channels pointer provided. Exit.",
            __func__);
        return WIFI_ERROR_INVALID_ARGS;
    }

    /* No request id from caller, so generate one and pass it on to the driver.
     * Generate one randomly.
     */
    srand( time(NULL) );
    requestId = rand();

    gScanCommand = new GScanCommand(
                            wifiHandle,
                            requestId,
                            OUI_QCA,
                            QCA_NL80211_VENDOR_SUBCMD_GSCAN_GET_VALID_CHANNELS);
    if (gScanCommand == NULL) {
        ALOGE("%s: Error GScanCommand NULL", __func__);
        return WIFI_ERROR_UNKNOWN;
    }
    /* Create the NL message. */
    ret = gScanCommand->create();
    if (ret < 0)
        goto cleanup;

    /* Set the interface Id of the message. */
    ret = gScanCommand->set_iface_id(ifaceInfo->name);
    if (ret < 0)
        goto cleanup;

    /* Add the vendor specific attributes for the NL command. */
    nlData = gScanCommand->attr_start(NL80211_ATTR_VENDOR_DATA);
    if (!nlData)
        goto cleanup;

    if (gScanCommand->put_u32(
            QCA_WLAN_VENDOR_ATTR_GSCAN_SUBCMD_CONFIG_PARAM_REQUEST_ID,
            requestId) ||
        gScanCommand->put_u32(
        QCA_WLAN_VENDOR_ATTR_GSCAN_GET_VALID_CHANNELS_CONFIG_PARAM_WIFI_BAND,
            band) ||
        gScanCommand->put_u32(
        QCA_WLAN_VENDOR_ATTR_GSCAN_GET_VALID_CHANNELS_CONFIG_PARAM_MAX_CHANNELS,
            max_channels) )
    {
        goto cleanup;
    }
    gScanCommand->attr_end(nlData);
    /* Populate the input received from caller/framework. */
    gScanCommand->setMaxChannels(max_channels);
    gScanCommand->setChannels(channels);
    gScanCommand->setNumChannelsPtr(num_channels);

    /* Send the msg and wait for a response. */
    ret = gScanCommand->requestResponse();
    if (ret) {
        ALOGE("%s: Error %d happened. ", __func__, ret);
    }

cleanup:
    ALOGI("%s: Delete object.", __func__);
    delete gScanCommand;
    return (wifi_error)ret;
}

void get_gscan_capabilities_cb(int status, wifi_gscan_capabilities capa)
{
    ALOGD("%s: Status = %d.", __func__, status);
    ALOGD("%s: Capabilities. max_ap_cache_per_scan:%d, "
            "max_bssid_history_entries:%d, max_hotlist_bssids:%d, "
            "max_hotlist_ssids:%d, max_rssi_sample_size:%d, "
            "max_scan_buckets:%d, "
            "max_scan_cache_size:%d, max_scan_reporting_threshold:%d, "
            "max_significant_wifi_change_aps:%d, "
            "max_number_epno_networks:%d, "
            "max_number_epno_networks_by_ssid:%d, "
            "max_number_of_white_listed_ssid:%d.",
            __func__, capa.max_ap_cache_per_scan,
            capa.max_bssid_history_entries,
            capa.max_hotlist_bssids, capa.max_hotlist_ssids,
            capa.max_rssi_sample_size,
            capa.max_scan_buckets,
            capa.max_scan_cache_size, capa.max_scan_reporting_threshold,
            capa.max_significant_wifi_change_aps,
            capa.max_number_epno_networks,
            capa.max_number_epno_networks_by_ssid,
            capa.max_number_of_white_listed_ssid);
}

wifi_error wifi_get_gscan_capabilities(wifi_interface_handle handle,
                                 wifi_gscan_capabilities *capabilities)
{
    int requestId, ret = 0;
    GScanCommand *gScanCommand;
    struct nlattr *nlData;
    wifi_gscan_capabilities tCapabilities;
    interface_info *ifaceInfo = getIfaceInfo(handle);
    wifi_handle wifiHandle = getWifiHandle(handle);
    hal_info *info = getHalInfo(wifiHandle);

    if (!(info->supported_feature_set & WIFI_FEATURE_GSCAN)) {
        ALOGE("%s: GSCAN is not supported by driver",
            __func__);
        return WIFI_ERROR_NOT_SUPPORTED;
    }


    if (capabilities == NULL) {
        ALOGE("%s: NULL capabilities pointer provided. Exit.",
            __func__);
        return WIFI_ERROR_INVALID_ARGS;
    }

    /* No request id from caller, so generate one and pass it on to the driver.
     * Generate it randomly.
     */
    srand(time(NULL));
    requestId = rand();

    gScanCommand = new GScanCommand(
                            wifiHandle,
                            requestId,
                            OUI_QCA,
                            QCA_NL80211_VENDOR_SUBCMD_GSCAN_GET_CAPABILITIES);
    if (gScanCommand == NULL) {
        ALOGE("%s: Error GScanCommand NULL", __func__);
        return WIFI_ERROR_UNKNOWN;
    }

    GScanCallbackHandler callbackHandler;
    memset(&callbackHandler, 0, sizeof(callbackHandler));
    callbackHandler.get_capabilities = get_gscan_capabilities_cb;

    ret = gScanCommand->setCallbackHandler(callbackHandler);
    if (ret < 0)
        goto cleanup;

    /* Create the NL message. */
    ret = gScanCommand->create();
    if (ret < 0)
        goto cleanup;

    /* Set the interface Id of the message. */
    ret = gScanCommand->set_iface_id(ifaceInfo->name);
    if (ret < 0)
        goto cleanup;

    /* Add the vendor specific attributes for the NL command. */
    nlData = gScanCommand->attr_start(NL80211_ATTR_VENDOR_DATA);
    if (!nlData)
        goto cleanup;

    ret = gScanCommand->put_u32(
            QCA_WLAN_VENDOR_ATTR_GSCAN_SUBCMD_CONFIG_PARAM_REQUEST_ID,
            requestId);
    if (ret < 0)
        goto cleanup;

    gScanCommand->attr_end(nlData);
    ret = gScanCommand->allocRspParams(eGScanGetCapabilitiesRspParams);
    if (ret != 0) {
        ALOGE("%s: Failed to allocate memory fo response struct. Error:%d",
            __func__, ret);
        goto cleanup;
    }

    gScanCommand->waitForRsp(true);
    ret = gScanCommand->requestEvent();
    if (ret != 0) {
        ALOGE("%s: requestEvent Error:%d",__func__, ret);
        goto cleanup;
    }

    gScanCommand->getGetCapabilitiesRspParams(capabilities, (u32 *)&ret);

cleanup:
    gScanCommand->freeRspParams(eGScanGetCapabilitiesRspParams);
    ALOGI("%s: Delete object.", __func__);
    delete gScanCommand;
    return (wifi_error)ret;
}

void start_gscan_cb(int status)
{
    ALOGD("%s: Status = %d.", __func__, status);
}

wifi_error wifi_start_gscan(wifi_request_id id,
                            wifi_interface_handle iface,
                            wifi_scan_cmd_params params,
                            wifi_scan_result_handler handler)
{
    int ret = 0;
    u32 i, j;
    GScanCommand *gScanCommand;
    struct nlattr *nlData;
    interface_info *ifaceInfo = getIfaceInfo(iface);
    wifi_handle wifiHandle = getWifiHandle(iface);
    u32 num_scan_buckets, numChannelSpecs;
    wifi_scan_bucket_spec bucketSpec;
    struct nlattr *nlBuckectSpecList;
    bool previousGScanRunning = false;
    hal_info *info = getHalInfo(wifiHandle);

    if (!(info->supported_feature_set & WIFI_FEATURE_GSCAN)) {
        ALOGE("%s: GSCAN is not supported by driver",
            __func__);
        return WIFI_ERROR_NOT_SUPPORTED;
    }

    /* Wi-Fi HAL doesn't need to check if a similar request to start gscan was
     *  made earlier. If start_gscan() is called while another gscan is already
     *  running, the request will be sent down to driver and firmware. If new
     * request is successfully honored, then Wi-Fi HAL will use the new request
     * id for the GScanStartCmdEventHandler object.
     */

    gScanCommand = new GScanCommand(
                                wifiHandle,
                                id,
                                OUI_QCA,
                                QCA_NL80211_VENDOR_SUBCMD_GSCAN_START);
    if (gScanCommand == NULL) {
        ALOGE("wifi_start_gscan(): Error GScanCommand NULL");
        return WIFI_ERROR_UNKNOWN;
    }

    GScanCallbackHandler callbackHandler;
    memset(&callbackHandler, 0, sizeof(callbackHandler));
    callbackHandler.start = start_gscan_cb;

    ret = gScanCommand->setCallbackHandler(callbackHandler);
    if (ret < 0)
        goto cleanup;

    /* Create the NL message. */
    ret = gScanCommand->create();
    if (ret < 0)
        goto cleanup;

    /* Set the interface Id of the message. */
    ret = gScanCommand->set_iface_id(ifaceInfo->name);
    if (ret < 0)
        goto cleanup;

    /* Add the vendor specific attributes for the NL command. */
    nlData = gScanCommand->attr_start(NL80211_ATTR_VENDOR_DATA);
    if (!nlData)
        goto cleanup;

    num_scan_buckets = (unsigned int)params.num_buckets > MAX_BUCKETS ?
                            MAX_BUCKETS : params.num_buckets;

    if (gScanCommand->put_u32(
            QCA_WLAN_VENDOR_ATTR_GSCAN_SUBCMD_CONFIG_PARAM_REQUEST_ID,
            id) ||
        gScanCommand->put_u32(
            QCA_WLAN_VENDOR_ATTR_GSCAN_SCAN_CMD_PARAMS_BASE_PERIOD,
            params.base_period) ||
        gScanCommand->put_u32(
            QCA_WLAN_VENDOR_ATTR_GSCAN_SCAN_CMD_PARAMS_MAX_AP_PER_SCAN,
            params.max_ap_per_scan) ||
        gScanCommand->put_u8(
            QCA_WLAN_VENDOR_ATTR_GSCAN_SCAN_CMD_PARAMS_REPORT_THRESHOLD_PERCENT,
            params.report_threshold_percent) ||
        gScanCommand->put_u8(
            QCA_WLAN_VENDOR_ATTR_GSCAN_SCAN_CMD_PARAMS_REPORT_THRESHOLD_NUM_SCANS,
            params.report_threshold_num_scans) ||
        gScanCommand->put_u8(
            QCA_WLAN_VENDOR_ATTR_GSCAN_SCAN_CMD_PARAMS_NUM_BUCKETS,
            num_scan_buckets))
    {
        goto cleanup;
    }

    nlBuckectSpecList =
        gScanCommand->attr_start(QCA_WLAN_VENDOR_ATTR_GSCAN_BUCKET_SPEC);
    /* Add NL attributes for scan bucket specs . */
    for (i = 0; i < num_scan_buckets; i++) {
        bucketSpec = params.buckets[i];
        numChannelSpecs = (unsigned int)bucketSpec.num_channels > MAX_CHANNELS ?
                                MAX_CHANNELS : bucketSpec.num_channels;
        struct nlattr *nlBucketSpec = gScanCommand->attr_start(i);
        if (gScanCommand->put_u8(
                QCA_WLAN_VENDOR_ATTR_GSCAN_BUCKET_SPEC_INDEX,
                bucketSpec.bucket) ||
            gScanCommand->put_u8(
                QCA_WLAN_VENDOR_ATTR_GSCAN_BUCKET_SPEC_BAND,
                bucketSpec.band) ||
            gScanCommand->put_u32(
                QCA_WLAN_VENDOR_ATTR_GSCAN_BUCKET_SPEC_PERIOD,
                bucketSpec.period) ||
            gScanCommand->put_u8(
                QCA_WLAN_VENDOR_ATTR_GSCAN_BUCKET_SPEC_REPORT_EVENTS,
                bucketSpec.report_events) ||
            gScanCommand->put_u32(
                QCA_WLAN_VENDOR_ATTR_GSCAN_BUCKET_SPEC_NUM_CHANNEL_SPECS,
                numChannelSpecs) ||
            gScanCommand->put_u32(
                QCA_WLAN_VENDOR_ATTR_GSCAN_BUCKET_SPEC_MAX_PERIOD,
                bucketSpec.max_period) ||
            gScanCommand->put_u32(
                QCA_WLAN_VENDOR_ATTR_GSCAN_BUCKET_SPEC_EXPONENT,
                bucketSpec.exponent) ||
            gScanCommand->put_u32(
                QCA_WLAN_VENDOR_ATTR_GSCAN_BUCKET_SPEC_STEP_COUNT,
                bucketSpec.step_count))
        {
            goto cleanup;
        }

        struct nlattr *nl_channelSpecList =
            gScanCommand->attr_start(QCA_WLAN_VENDOR_ATTR_GSCAN_CHANNEL_SPEC);

        /* Add NL attributes for scan channel specs . */
        for (j = 0; j < numChannelSpecs; j++) {
            struct nlattr *nl_channelSpec = gScanCommand->attr_start(j);
            wifi_scan_channel_spec channel_spec = bucketSpec.channels[j];

            if ( gScanCommand->put_u32(
                    QCA_WLAN_VENDOR_ATTR_GSCAN_CHANNEL_SPEC_CHANNEL,
                    channel_spec.channel) ||
                gScanCommand->put_u32(
                    QCA_WLAN_VENDOR_ATTR_GSCAN_CHANNEL_SPEC_DWELL_TIME,
                    channel_spec.dwellTimeMs) ||
                gScanCommand->put_u8(
                    QCA_WLAN_VENDOR_ATTR_GSCAN_CHANNEL_SPEC_PASSIVE,
                    channel_spec.passive) )
            {
                goto cleanup;
            }

            gScanCommand->attr_end(nl_channelSpec);
        }
        gScanCommand->attr_end(nl_channelSpecList);
        gScanCommand->attr_end(nlBucketSpec);
    }
    gScanCommand->attr_end(nlBuckectSpecList);

    gScanCommand->attr_end(nlData);

    ret = gScanCommand->allocRspParams(eGScanStartRspParams);
    if (ret != 0) {
        ALOGE("wifi_start_gscan(): Failed to allocate memory to the response "
            "struct. Error:%d", ret);
        goto cleanup;
    }

    /* Set the callback handler functions for related events. */
    callbackHandler.on_scan_results_available =
                        handler.on_scan_results_available;
    callbackHandler.on_full_scan_result = handler.on_full_scan_result;
    callbackHandler.on_scan_event = handler.on_scan_event;
    /* Create an object to handle the related events from firmware/driver. */
    if (GScanStartCmdEventHandler == NULL) {
        GScanStartCmdEventHandler = new GScanCommandEventHandler(
                                    wifiHandle,
                                    id,
                                    OUI_QCA,
                                    QCA_NL80211_VENDOR_SUBCMD_GSCAN_START,
                                    callbackHandler);
        if (GScanStartCmdEventHandler == NULL) {
            ALOGE("wifi_start_gscan(): Error GScanStartCmdEventHandler NULL");
            ret = WIFI_ERROR_UNKNOWN;
            goto cleanup;
        }
    } else {
        previousGScanRunning = true;
        ALOGD("%s: "
                "GScan is already running with request id=%d",
                __func__,
                GScanStartCmdEventHandler->get_request_id());
    }

    gScanCommand->waitForRsp(true);
    ret = gScanCommand->requestEvent();
    if (ret != 0) {
        ALOGE("wifi_start_gscan(): requestEvent Error:%d", ret);
        goto cleanup;
    }

    gScanCommand->getStartGScanRspParams((u32 *)&ret);
    if (ret != 0)
    {
        goto cleanup;
    }
    if (GScanStartCmdEventHandler != NULL) {
        GScanStartCmdEventHandler->set_request_id(id);
    }

cleanup:
    gScanCommand->freeRspParams(eGScanStartRspParams);
    ALOGI("wifi_start_gscan(): Delete object.");
    delete gScanCommand;
    /* Delete the command event handler object if ret != 0 */
    if (!previousGScanRunning && ret && GScanStartCmdEventHandler) {
        ALOGI("wifi_start_gscan(): Error ret:%d, delete event handler object.",
            ret);
        delete GScanStartCmdEventHandler;
        GScanStartCmdEventHandler = NULL;
    }
    return (wifi_error)ret;

}

void stop_gscan_cb(int status)
{
    ALOGD("%s: Status = %d.", __func__, status);
}

wifi_error wifi_stop_gscan(wifi_request_id id,
                            wifi_interface_handle iface)
{
    int ret = 0;
    GScanCommand *gScanCommand;
    struct nlattr *nlData;

    interface_info *ifaceInfo = getIfaceInfo(iface);
    wifi_handle wifiHandle = getWifiHandle(iface);
    hal_info *info = getHalInfo(wifiHandle);

    if (!(info->supported_feature_set & WIFI_FEATURE_GSCAN)) {
        ALOGE("%s: GSCAN is not supported by driver",
            __func__);
        return WIFI_ERROR_NOT_SUPPORTED;
    }

    ALOGI("Stopping GScan, halHandle = %p", wifiHandle);

    if (GScanStartCmdEventHandler == NULL) {
        ALOGE("wifi_stop_gscan: GSCAN isn't running or already stopped. "
            "Nothing to do. Exit");
        return WIFI_ERROR_NOT_AVAILABLE;
    }

    gScanCommand = new GScanCommand(
                                wifiHandle,
                                id,
                                OUI_QCA,
                                QCA_NL80211_VENDOR_SUBCMD_GSCAN_STOP);
    if (gScanCommand == NULL) {
        ALOGE("%s: Error GScanCommand NULL", __func__);
        return WIFI_ERROR_UNKNOWN;
    }

    GScanCallbackHandler callbackHandler;
    memset(&callbackHandler, 0, sizeof(callbackHandler));
    callbackHandler.stop = stop_gscan_cb;

    ret = gScanCommand->setCallbackHandler(callbackHandler);
    if (ret < 0)
        goto cleanup;

    /* Create the NL message. */
    ret = gScanCommand->create();
    if (ret < 0)
        goto cleanup;

    /* Set the interface Id of the message. */
    ret = gScanCommand->set_iface_id(ifaceInfo->name);
    if (ret < 0)
        goto cleanup;

    /* Add the vendor specific attributes for the NL command. */
    nlData = gScanCommand->attr_start(NL80211_ATTR_VENDOR_DATA);
    if (!nlData)
        goto cleanup;

    ret = gScanCommand->put_u32(
            QCA_WLAN_VENDOR_ATTR_GSCAN_SUBCMD_CONFIG_PARAM_REQUEST_ID,
            id);
    if (ret < 0)
        goto cleanup;

    gScanCommand->attr_end(nlData);

    ret = gScanCommand->allocRspParams(eGScanStopRspParams);
    if (ret != 0) {
        ALOGE("%s: Failed to allocate memory to the response struct. "
            "Error:%d", __func__, ret);
        goto cleanup;
    }

    gScanCommand->waitForRsp(true);
    ret = gScanCommand->requestEvent();
    if (ret != 0) {
        ALOGE("%s: requestEvent Error:%d",__func__, ret);
        if (ret == ETIMEDOUT)
        {
            /* Delete different GSCAN event handlers for the specified Request ID. */
            if (GScanStartCmdEventHandler) {
                delete GScanStartCmdEventHandler;
                GScanStartCmdEventHandler = NULL;
            }
        }
        goto cleanup;
    }

    gScanCommand->getStopGScanRspParams((u32 *)&ret);
    if (ret != 0)
    {
        goto cleanup;
    }

    /* Delete different GSCAN event handlers for the specified Request ID. */
    if (GScanStartCmdEventHandler) {
        delete GScanStartCmdEventHandler;
        GScanStartCmdEventHandler = NULL;
    }

cleanup:
    gScanCommand->freeRspParams(eGScanStopRspParams);
    ALOGI("%s: Delete object.", __func__);
    delete gScanCommand;
    return (wifi_error)ret;
}

void set_bssid_hotlist_cb(int status)
{
    ALOGD("%s: Status = %d.", __func__, status);
}

/* Set the GSCAN BSSID Hotlist. */
wifi_error wifi_set_bssid_hotlist(wifi_request_id id,
                                    wifi_interface_handle iface,
                                    wifi_bssid_hotlist_params params,
                                    wifi_hotlist_ap_found_handler handler)
{
    int i, numAp, ret = 0;
    GScanCommand *gScanCommand;
    struct nlattr *nlData, *nlApThresholdParamList;
    interface_info *ifaceInfo = getIfaceInfo(iface);
    wifi_handle wifiHandle = getWifiHandle(iface);
    bool previousGScanSetBssidRunning = false;
    hal_info *info = getHalInfo(wifiHandle);

    if (!(info->supported_feature_set & WIFI_FEATURE_GSCAN)) {
        ALOGE("%s: GSCAN is not supported by driver",
            __func__);
        return WIFI_ERROR_NOT_SUPPORTED;
    }

    ALOGD("Setting GScan BSSID Hotlist, halHandle = %p", wifiHandle);

    /* Wi-Fi HAL doesn't need to check if a similar request to set bssid
     * hotlist was made earlier. If set_bssid_hotlist() is called while
     * another one is running, the request will be sent down to driver and
     * firmware. If the new request is successfully honored, then Wi-Fi HAL
     * will use the new request id for the GScanSetBssidHotlistCmdEventHandler
     * object.
     */

    gScanCommand =
        new GScanCommand(
                    wifiHandle,
                    id,
                    OUI_QCA,
                    QCA_NL80211_VENDOR_SUBCMD_GSCAN_SET_BSSID_HOTLIST);
    if (gScanCommand == NULL) {
        ALOGE("%s: Error GScanCommand NULL", __func__);
        return WIFI_ERROR_UNKNOWN;
    }

    GScanCallbackHandler callbackHandler;
    memset(&callbackHandler, 0, sizeof(callbackHandler));
    callbackHandler.set_bssid_hotlist = set_bssid_hotlist_cb;

    ret = gScanCommand->setCallbackHandler(callbackHandler);
    if (ret < 0)
        goto cleanup;

    /* Create the NL message. */
    ret = gScanCommand->create();
    if (ret < 0)
        goto cleanup;

    /* Set the interface Id of the message. */
    ret = gScanCommand->set_iface_id(ifaceInfo->name);
    if (ret < 0)
        goto cleanup;

    /* Add the vendor specific attributes for the NL command. */
    nlData = gScanCommand->attr_start(NL80211_ATTR_VENDOR_DATA);
    if (!nlData)
        goto cleanup;

    numAp = (unsigned int)params.num_bssid > MAX_HOTLIST_APS ?
        MAX_HOTLIST_APS : params.num_bssid;
    if (gScanCommand->put_u32(
            QCA_WLAN_VENDOR_ATTR_GSCAN_SUBCMD_CONFIG_PARAM_REQUEST_ID,
            id) ||
        gScanCommand->put_u32(
            QCA_WLAN_VENDOR_ATTR_GSCAN_BSSID_HOTLIST_PARAMS_LOST_AP_SAMPLE_SIZE,
            params.lost_ap_sample_size) ||
        gScanCommand->put_u32(
            QCA_WLAN_VENDOR_ATTR_GSCAN_BSSID_HOTLIST_PARAMS_NUM_AP,
            numAp))
    {
        goto cleanup;
    }

    /* Add the vendor specific attributes for the NL command. */
    nlApThresholdParamList =
        gScanCommand->attr_start(
                                QCA_WLAN_VENDOR_ATTR_GSCAN_AP_THRESHOLD_PARAM);
    if (!nlApThresholdParamList)
        goto cleanup;

    /* Add nested NL attributes for AP Threshold Param. */
    for (i = 0; i < numAp; i++) {
        ap_threshold_param apThreshold = params.ap[i];
        struct nlattr *nlApThresholdParam = gScanCommand->attr_start(i);
        if (!nlApThresholdParam)
            goto cleanup;
        if (gScanCommand->put_addr(
                QCA_WLAN_VENDOR_ATTR_GSCAN_AP_THRESHOLD_PARAM_BSSID,
                apThreshold.bssid) ||
            gScanCommand->put_s32(
                QCA_WLAN_VENDOR_ATTR_GSCAN_AP_THRESHOLD_PARAM_RSSI_LOW,
                apThreshold.low) ||
            gScanCommand->put_s32(
                QCA_WLAN_VENDOR_ATTR_GSCAN_AP_THRESHOLD_PARAM_RSSI_HIGH,
                apThreshold.high))
        {
            goto cleanup;
        }
        gScanCommand->attr_end(nlApThresholdParam);
    }

    gScanCommand->attr_end(nlApThresholdParamList);

    gScanCommand->attr_end(nlData);

    ret = gScanCommand->allocRspParams(eGScanSetBssidHotlistRspParams);
    if (ret != 0) {
        ALOGE("%s: Failed to allocate memory to the response struct. "
            "Error:%d", __func__, ret);
        goto cleanup;
    }

    callbackHandler.on_hotlist_ap_found = handler.on_hotlist_ap_found;
    callbackHandler.on_hotlist_ap_lost = handler.on_hotlist_ap_lost;
    /* Create an object of the event handler class to take care of the
      * asychronous events on the north-bound.
      */
    if (GScanSetBssidHotlistCmdEventHandler == NULL) {
        GScanSetBssidHotlistCmdEventHandler = new GScanCommandEventHandler(
                            wifiHandle,
                            id,
                            OUI_QCA,
                            QCA_NL80211_VENDOR_SUBCMD_GSCAN_SET_BSSID_HOTLIST,
                            callbackHandler);
        if (GScanSetBssidHotlistCmdEventHandler == NULL) {
            ALOGE("%s: Error instantiating "
                "GScanSetBssidHotlistCmdEventHandler.", __func__);
            ret = WIFI_ERROR_UNKNOWN;
            goto cleanup;
        }
        ALOGD("%s: Handler object was created for HOTLIST_AP_FOUND.", __func__);
    } else {
        previousGScanSetBssidRunning = true;
        ALOGD("%s: "
                "A HOTLIST_AP_FOUND event handler object already exists "
                "with request id=%d",
                __func__,
                GScanSetBssidHotlistCmdEventHandler->get_request_id());
    }

    gScanCommand->waitForRsp(true);
    ret = gScanCommand->requestEvent();
    if (ret != 0) {
        ALOGE("%s: requestEvent Error:%d",__func__, ret);
        goto cleanup;
    }

    gScanCommand->getSetBssidHotlistRspParams((u32 *)&ret);
    if (ret != 0)
    {
        goto cleanup;
    }
    if (GScanSetBssidHotlistCmdEventHandler != NULL) {
        GScanSetBssidHotlistCmdEventHandler->set_request_id(id);
    }

cleanup:
    gScanCommand->freeRspParams(eGScanSetBssidHotlistRspParams);
    ALOGI("%s: Delete object. ", __func__);
    delete gScanCommand;
    /* Delete the command event handler object if ret != 0 */
    if (!previousGScanSetBssidRunning && ret
        && GScanSetBssidHotlistCmdEventHandler) {
        delete GScanSetBssidHotlistCmdEventHandler;
        GScanSetBssidHotlistCmdEventHandler = NULL;
    }
    return (wifi_error)ret;
}

void reset_bssid_hotlist_cb(int status)
{
    ALOGD("%s: Status = %d.", __func__, status);
}

wifi_error wifi_reset_bssid_hotlist(wifi_request_id id,
                            wifi_interface_handle iface)
{
    int ret = 0;
    GScanCommand *gScanCommand;
    struct nlattr *nlData;
    interface_info *ifaceInfo = getIfaceInfo(iface);
    wifi_handle wifiHandle = getWifiHandle(iface);
    hal_info *info = getHalInfo(wifiHandle);

    if (!(info->supported_feature_set & WIFI_FEATURE_GSCAN)) {
        ALOGE("%s: GSCAN is not supported by driver",
            __func__);
        return WIFI_ERROR_NOT_SUPPORTED;
    }

    ALOGE("Resetting GScan BSSID Hotlist, halHandle = %p", wifiHandle);

    if (GScanSetBssidHotlistCmdEventHandler == NULL) {
        ALOGE("wifi_reset_bssid_hotlist: GSCAN bssid_hotlist isn't set. "
            "Nothing to do. Exit");
        return WIFI_ERROR_NOT_AVAILABLE;
    }

    gScanCommand = new GScanCommand(
                        wifiHandle,
                        id,
                        OUI_QCA,
                        QCA_NL80211_VENDOR_SUBCMD_GSCAN_RESET_BSSID_HOTLIST);

    if (gScanCommand == NULL) {
        ALOGE("%s: Error GScanCommand NULL", __func__);
        return WIFI_ERROR_UNKNOWN;
    }

    GScanCallbackHandler callbackHandler;
    memset(&callbackHandler, 0, sizeof(callbackHandler));
    callbackHandler.reset_bssid_hotlist = reset_bssid_hotlist_cb;

    ret = gScanCommand->setCallbackHandler(callbackHandler);
    if (ret < 0)
        goto cleanup;

    /* Create the NL message. */
    ret = gScanCommand->create();
    if (ret < 0)
        goto cleanup;

    /* Set the interface Id of the message. */
    ret = gScanCommand->set_iface_id(ifaceInfo->name);
    if (ret < 0)
        goto cleanup;

    /* Add the vendor specific attributes for the NL command. */
    nlData = gScanCommand->attr_start(NL80211_ATTR_VENDOR_DATA);
    if (!nlData)
        goto cleanup;

    ret = gScanCommand->put_u32(
            QCA_WLAN_VENDOR_ATTR_GSCAN_SUBCMD_CONFIG_PARAM_REQUEST_ID, id);
    if (ret < 0)
        goto cleanup;

    gScanCommand->attr_end(nlData);

    ret = gScanCommand->allocRspParams(eGScanResetBssidHotlistRspParams);
    if (ret != 0) {
        ALOGE("%s: Failed to allocate memory to the response struct. "
            "Error:%d", __func__, ret);
        goto cleanup;
    }

    gScanCommand->waitForRsp(true);
    ret = gScanCommand->requestEvent();
    if (ret != 0) {
        ALOGE("%s: requestEvent Error:%d",__func__, ret);
        if (ret == ETIMEDOUT)
        {
            if (GScanSetBssidHotlistCmdEventHandler) {
                delete GScanSetBssidHotlistCmdEventHandler;
                GScanSetBssidHotlistCmdEventHandler = NULL;
            }
        }
        goto cleanup;
    }

    gScanCommand->getResetBssidHotlistRspParams((u32 *)&ret);
    if (ret != 0)
    {
        goto cleanup;
    }
    if (GScanSetBssidHotlistCmdEventHandler) {
        delete GScanSetBssidHotlistCmdEventHandler;
        GScanSetBssidHotlistCmdEventHandler = NULL;
    }

cleanup:
    gScanCommand->freeRspParams(eGScanResetBssidHotlistRspParams);
    ALOGI("%s: Delete object.", __func__);
    delete gScanCommand;
    return (wifi_error)ret;
}

void set_significant_change_cb(int status)
{
    ALOGD("%s: Status = %d.", __func__, status);
}

/* Set the GSCAN Significant AP Change list. */
wifi_error wifi_set_significant_change_handler(wifi_request_id id,
                                            wifi_interface_handle iface,
                                    wifi_significant_change_params params,
                                    wifi_significant_change_handler handler)
{
    int i, numAp, ret = 0;
    GScanCommand *gScanCommand;
    struct nlattr *nlData, *nlApThresholdParamList;
    interface_info *ifaceInfo = getIfaceInfo(iface);
    wifi_handle wifiHandle = getWifiHandle(iface);
    bool previousGScanSetSigChangeRunning = false;
    hal_info *info = getHalInfo(wifiHandle);

    if (!(info->supported_feature_set & WIFI_FEATURE_GSCAN)) {
        ALOGE("%s: GSCAN is not supported by driver",
            __func__);
        return WIFI_ERROR_NOT_SUPPORTED;
    }

    ALOGE("Setting GScan Significant Change, halHandle = %p", wifiHandle);

    /* Wi-Fi HAL doesn't need to check if a similar request to set significant
     * change list was made earlier. If set_significant_change() is called while
     * another one is running, the request will be sent down to driver and
     * firmware. If the new request is successfully honored, then Wi-Fi HAL
     * will use the new request id for the GScanSetBssidHotlistCmdEventHandler
     * object.
     */

    gScanCommand = new GScanCommand(
                    wifiHandle,
                    id,
                    OUI_QCA,
                    QCA_NL80211_VENDOR_SUBCMD_GSCAN_SET_SIGNIFICANT_CHANGE);
    if (gScanCommand == NULL) {
        ALOGE("%s: Error GScanCommand NULL", __func__);
        return WIFI_ERROR_UNKNOWN;
    }

    GScanCallbackHandler callbackHandler;
    memset(&callbackHandler, 0, sizeof(callbackHandler));
    callbackHandler.set_significant_change = set_significant_change_cb;

    ret = gScanCommand->setCallbackHandler(callbackHandler);
    if (ret < 0)
        goto cleanup;

    /* Create the NL message. */
    ret = gScanCommand->create();
    if (ret < 0)
        goto cleanup;

    /* Set the interface Id of the message. */
    ret = gScanCommand->set_iface_id(ifaceInfo->name);
    if (ret < 0)
        goto cleanup;

    /* Add the vendor specific attributes for the NL command. */
    nlData = gScanCommand->attr_start(NL80211_ATTR_VENDOR_DATA);
    if (!nlData)
        goto cleanup;

    numAp = (unsigned int)params.num_bssid > MAX_SIGNIFICANT_CHANGE_APS ?
        MAX_SIGNIFICANT_CHANGE_APS : params.num_bssid;

    if (gScanCommand->put_u32(
            QCA_WLAN_VENDOR_ATTR_GSCAN_SUBCMD_CONFIG_PARAM_REQUEST_ID,
            id) ||
        gScanCommand->put_u32(
        QCA_WLAN_VENDOR_ATTR_GSCAN_SIGNIFICANT_CHANGE_PARAMS_RSSI_SAMPLE_SIZE,
            params.rssi_sample_size) ||
        gScanCommand->put_u32(
        QCA_WLAN_VENDOR_ATTR_GSCAN_SIGNIFICANT_CHANGE_PARAMS_LOST_AP_SAMPLE_SIZE,
            params.lost_ap_sample_size) ||
        gScanCommand->put_u32(
            QCA_WLAN_VENDOR_ATTR_GSCAN_SIGNIFICANT_CHANGE_PARAMS_MIN_BREACHING,
            params.min_breaching) ||
        gScanCommand->put_u32(
            QCA_WLAN_VENDOR_ATTR_GSCAN_SIGNIFICANT_CHANGE_PARAMS_NUM_AP,
            numAp))
    {
        goto cleanup;
    }

    /* Add the vendor specific attributes for the NL command. */
    nlApThresholdParamList =
        gScanCommand->attr_start(
                                QCA_WLAN_VENDOR_ATTR_GSCAN_AP_THRESHOLD_PARAM);
    if (!nlApThresholdParamList)
        goto cleanup;

    /* Add nested NL attributes for AP Threshold Param list. */
    for (i = 0; i < numAp; i++) {
        ap_threshold_param apThreshold = params.ap[i];
        struct nlattr *nlApThresholdParam = gScanCommand->attr_start(i);
        if (!nlApThresholdParam)
            goto cleanup;
        if ( gScanCommand->put_addr(
                QCA_WLAN_VENDOR_ATTR_GSCAN_AP_THRESHOLD_PARAM_BSSID,
                apThreshold.bssid) ||
            gScanCommand->put_s32(
                QCA_WLAN_VENDOR_ATTR_GSCAN_AP_THRESHOLD_PARAM_RSSI_LOW,
                apThreshold.low) ||
            gScanCommand->put_s32(
                QCA_WLAN_VENDOR_ATTR_GSCAN_AP_THRESHOLD_PARAM_RSSI_HIGH,
                apThreshold.high))
        {
            goto cleanup;
        }
        gScanCommand->attr_end(nlApThresholdParam);
    }

    gScanCommand->attr_end(nlApThresholdParamList);

    gScanCommand->attr_end(nlData);

    ret = gScanCommand->allocRspParams(eGScanSetSignificantChangeRspParams);
    if (ret != 0) {
        ALOGE("%s: Failed to allocate memory to the response struct. "
            "Error:%d", __func__, ret);
        goto cleanup;
    }

    callbackHandler.on_significant_change = handler.on_significant_change;
    /* Create an object of the event handler class to take care of the
      * asychronous events on the north-bound.
      */
    if (GScanSetSignificantChangeCmdEventHandler == NULL) {
        GScanSetSignificantChangeCmdEventHandler =
            new GScanCommandEventHandler(
                     wifiHandle,
                     id,
                     OUI_QCA,
                     QCA_NL80211_VENDOR_SUBCMD_GSCAN_SET_SIGNIFICANT_CHANGE,
                     callbackHandler);
        if (GScanSetSignificantChangeCmdEventHandler == NULL) {
            ALOGE("%s: Error in instantiating, "
                "GScanSetSignificantChangeCmdEventHandler.",
                __func__);
            ret = WIFI_ERROR_UNKNOWN;
            goto cleanup;
        }
        ALOGD("%s: Event handler object was created for SIGNIFICANT_CHANGE.",
            __func__);
    } else {
        previousGScanSetSigChangeRunning = true;
        ALOGD("%s: "
            "A SIGNIFICANT_CHANGE event handler object already exists "
            "with request id=%d",
            __func__,
            GScanSetSignificantChangeCmdEventHandler->get_request_id());
    }

    gScanCommand->waitForRsp(true);
    ret = gScanCommand->requestEvent();
    if (ret != 0) {
        ALOGE("%s: requestEvent Error:%d",__func__, ret);
        goto cleanup;
    }

    gScanCommand->getSetSignificantChangeRspParams((u32 *)&ret);
    if (ret != 0)
    {
        goto cleanup;
    }
    if (GScanSetSignificantChangeCmdEventHandler != NULL) {
        GScanSetSignificantChangeCmdEventHandler->set_request_id(id);
    }

cleanup:
    gScanCommand->freeRspParams(eGScanSetSignificantChangeRspParams);
    ALOGI("%s: Delete object.", __func__);
    /* Delete the command event handler object if ret != 0 */
    if (!previousGScanSetSigChangeRunning && ret
        && GScanSetSignificantChangeCmdEventHandler) {
        delete GScanSetSignificantChangeCmdEventHandler;
        GScanSetSignificantChangeCmdEventHandler = NULL;
    }
    delete gScanCommand;
    return (wifi_error)ret;
}

void reset_significant_change_cb(int status)
{
    ALOGD("%s: Status = %d.", __func__, status);
}

/* Clear the GSCAN Significant AP change list. */
wifi_error wifi_reset_significant_change_handler(wifi_request_id id,
                                            wifi_interface_handle iface)
{
    int ret = 0;
    GScanCommand *gScanCommand;
    struct nlattr *nlData;
    interface_info *ifaceInfo = getIfaceInfo(iface);
    wifi_handle wifiHandle = getWifiHandle(iface);
    hal_info *info = getHalInfo(wifiHandle);

    if (!(info->supported_feature_set & WIFI_FEATURE_GSCAN)) {
        ALOGE("%s: GSCAN is not supported by driver",
            __func__);
        return WIFI_ERROR_NOT_SUPPORTED;
    }

    ALOGD("Resetting GScan Significant Change, halHandle = %p", wifiHandle);

    if (GScanSetSignificantChangeCmdEventHandler == NULL) {
        ALOGE("wifi_reset_significant_change_handler: GSCAN significant_change"
            " isn't set. Nothing to do. Exit");
        return WIFI_ERROR_NOT_AVAILABLE;
    }

    gScanCommand =
        new GScanCommand
                    (
                    wifiHandle,
                    id,
                    OUI_QCA,
                    QCA_NL80211_VENDOR_SUBCMD_GSCAN_RESET_SIGNIFICANT_CHANGE);
    if (gScanCommand == NULL) {
        ALOGE("%s: Error GScanCommand NULL", __func__);
        return WIFI_ERROR_UNKNOWN;
    }

    GScanCallbackHandler callbackHandler;
    memset(&callbackHandler, 0, sizeof(callbackHandler));
    callbackHandler.reset_significant_change = reset_significant_change_cb;

    ret = gScanCommand->setCallbackHandler(callbackHandler);
    if (ret < 0)
        goto cleanup;

    /* Create the NL message. */
    ret = gScanCommand->create();
    if (ret < 0)
        goto cleanup;

    /* Set the interface Id of the message. */
    ret = gScanCommand->set_iface_id(ifaceInfo->name);
    if (ret < 0)
        goto cleanup;

    /* Add the vendor specific attributes for the NL command. */
    nlData = gScanCommand->attr_start(NL80211_ATTR_VENDOR_DATA);
    if (!nlData)
        goto cleanup;

    ret = gScanCommand->put_u32(
                    QCA_WLAN_VENDOR_ATTR_GSCAN_SUBCMD_CONFIG_PARAM_REQUEST_ID,
                    id);
    if (ret < 0)
        goto cleanup;

    gScanCommand->attr_end(nlData);

    ret = gScanCommand->allocRspParams(eGScanResetSignificantChangeRspParams);
    if (ret != 0) {
        ALOGE("%s: Failed to allocate memory to the response struct. "
            "Error:%d", __func__, ret);
        goto cleanup;
    }

    gScanCommand->waitForRsp(true);
    ret = gScanCommand->requestEvent();
    if (ret != 0) {
        ALOGE("%s: requestEvent Error:%d",__func__, ret);
        if (ret == ETIMEDOUT)
        {
            if (GScanSetSignificantChangeCmdEventHandler) {
                delete GScanSetSignificantChangeCmdEventHandler;
                GScanSetSignificantChangeCmdEventHandler = NULL;
            }
        }
        goto cleanup;
    }

    gScanCommand->getResetSignificantChangeRspParams((u32 *)&ret);
    if (ret != 0)
    {
        goto cleanup;
    }
    if (GScanSetSignificantChangeCmdEventHandler) {
        delete GScanSetSignificantChangeCmdEventHandler;
        GScanSetSignificantChangeCmdEventHandler = NULL;
    }

cleanup:
    gScanCommand->freeRspParams(eGScanResetSignificantChangeRspParams);
    ALOGI("%s: Delete object.", __func__);
    delete gScanCommand;
    return (wifi_error)ret;
}

void get_gscan_cached_results_cb(u8 moreData, u32 numResults)
{
    ALOGD("%s: More data = %d.", __func__, moreData);
    ALOGD("%s: Number of cached results = %d.", __func__, numResults);
}

/* Get the GSCAN cached scan results. */
wifi_error wifi_get_cached_gscan_results(wifi_interface_handle iface,
                                            byte flush, int max,
                                            wifi_cached_scan_results *results,
                                            int *num)
{
    int requestId, ret = 0;
    wifi_cached_scan_results *result = results;
    u32 j = 0;
    int i = 0;
    u8 moreData = 0;
    u16 waitTime = GSCAN_EVENT_WAIT_TIME_SECONDS;
    GScanCommand *gScanCommand;
    struct nlattr *nlData;
    wifi_cached_scan_results *cached_results;

    interface_info *ifaceInfo = getIfaceInfo(iface);
    wifi_handle wifiHandle = getWifiHandle(iface);
    hal_info *info = getHalInfo(wifiHandle);

    if (!(info->supported_feature_set & WIFI_FEATURE_GSCAN)) {
        ALOGE("%s: GSCAN is not supported by driver",
            __func__);
        return WIFI_ERROR_NOT_SUPPORTED;
    }

    if (results == NULL) {
        ALOGE("%s: NULL results pointer provided. Exit.",
            __func__);
        return WIFI_ERROR_INVALID_ARGS;
    }

    /* No request id from caller, so generate one and pass it on to the driver. */
    /* Generate it randomly */
    srand(time(NULL));
    requestId = rand();

    ALOGE("Getting GScan Cached Results, halHandle = %p", wifiHandle);

    gScanCommand = new GScanCommand(
                        wifiHandle,
                        requestId,
                        OUI_QCA,
                        QCA_NL80211_VENDOR_SUBCMD_GSCAN_GET_CACHED_RESULTS);
    if (gScanCommand == NULL) {
        ALOGE("%s: Error GScanCommand NULL", __func__);
        return WIFI_ERROR_UNKNOWN;
    }

    GScanCallbackHandler callbackHandler;
    memset(&callbackHandler, 0, sizeof(callbackHandler));
    callbackHandler.get_cached_results = get_gscan_cached_results_cb;

    ret = gScanCommand->setCallbackHandler(callbackHandler);
    if (ret < 0)
        goto cleanup;

    /* Create the NL message. */
    ret = gScanCommand->create();
    if (ret < 0)
        goto cleanup;

    /* Set the interface Id of the message. */
    ret = gScanCommand->set_iface_id(ifaceInfo->name);
    if (ret < 0)
        goto cleanup;

    /* Add the vendor specific attributes for the NL command. */
    nlData = gScanCommand->attr_start(NL80211_ATTR_VENDOR_DATA);
    if (!nlData)
        goto cleanup;

    if (ret < 0)
        goto cleanup;

    if (gScanCommand->put_u32(
         QCA_WLAN_VENDOR_ATTR_GSCAN_SUBCMD_CONFIG_PARAM_REQUEST_ID,
            requestId) ||
        gScanCommand->put_u8(
         QCA_WLAN_VENDOR_ATTR_GSCAN_GET_CACHED_SCAN_RESULTS_CONFIG_PARAM_FLUSH,
            flush) ||
        gScanCommand->put_u32(
         QCA_WLAN_VENDOR_ATTR_GSCAN_GET_CACHED_SCAN_RESULTS_CONFIG_PARAM_MAX,
            max))
    {
        goto cleanup;
    }

    gScanCommand->attr_end(nlData);

    ret = gScanCommand->allocRspParams(eGScanGetCachedResultsRspParams);
    if (ret != 0) {
        ALOGE("%s: Failed to allocate memory for response struct. Error:%d",
            __func__, ret);
        goto cleanup;
    }

    ret = gScanCommand->allocCachedResultsTemp(max, results);
    if (ret != 0) {
        ALOGE("%s: Failed to allocate memory for temp gscan cached list. "
            "Error:%d", __func__, ret);
        goto cleanup;
    }

    /* Clear the destination cached results list before copying results. */
    memset(results, 0, max * sizeof(wifi_cached_scan_results));

    gScanCommand->waitForRsp(true);
    ret = gScanCommand->requestEvent();
    if (ret != 0) {
        ALOGE("%s: requestEvent Error:%d",__func__, ret);
        goto cleanup;
    }

    /* Read more data flag and number of cached results retrieved
     * from driver/firmware.
     * If more data is 0 or numResults >= max, we won't enter the loop.
     * Instead we will proceed with copying & returning the cached results.
     * Otherwise, loop in 4s wait for next results data fragment(s).
     */
    ret = gScanCommand->getGetCachedResultsRspParams(
                                               (u8 *)&moreData,
                                               num);
    ALOGD("wifi_get_cached_gscan_results: max: %d, num:%d", max,
            *num);
    while (!ret && moreData && (*num < max)) {
        int res = gScanCommand->timed_wait(waitTime);
        if (res == ETIMEDOUT) {
            ALOGE("%s: Time out happened.", __func__);
            /*Proceed to cleanup & return whatever data avaiable at this time*/
            goto cleanup;
        }
        ALOGD("%s: Command invoked return value:%d",__func__, res);
        /* Read the moreData and numResults again and possibly exit the loop
         * if no more data or we have reached max num of cached results.
         */
        ret = gScanCommand->getGetCachedResultsRspParams(
                                                   (u8 *)&moreData,
                                                   num);
        ALOGD("wifi_get_cached_gscan_results: max: %d, num:%d", max,
            *num);
    }
    /* No more data, copy the parsed results into the caller's results array */
    ret = gScanCommand->copyCachedScanResults(*num, results);

    if (!ret) {
        int i = 0, j = 0;
        for(i = 0; i < *num; i++)
        {
            ALOGI("HAL:  scan_id  %d \n", results[i].scan_id);
            ALOGI("HAL:  flags  %u \n", results[i].flags);
            ALOGI("HAL:  num_results  %d \n\n", results[i].num_results);
            for(j = 0; j < results[i].num_results; j++)
            {
                ALOGI("HAL:  Wi-Fi Scan Result : %d\n", j+1);
                ALOGI("HAL:  ts  %lld \n", results[i].results[j].ts);
                ALOGI("HAL:  SSID  %s \n", results[i].results[j].ssid);
                ALOGI("HAL:  BSSID: "
                   "%02x:%02x:%02x:%02x:%02x:%02x \n",
                   results[i].results[j].bssid[0], results[i].results[j].bssid[1],
                   results[i].results[j].bssid[2], results[i].results[j].bssid[3],
                   results[i].results[j].bssid[4], results[i].results[j].bssid[5]);
                ALOGI("HAL:  channel %d \n", results[i].results[j].channel);
                ALOGI("HAL:  rssi  %d \n", results[i].results[j].rssi);
                ALOGI("HAL:  rtt  %lld \n", results[i].results[j].rtt);
                ALOGI("HAL:  rtt_sd  %lld \n", results[i].results[j].rtt_sd);
                ALOGI("HAL:  beacon period  %d \n",
                results[i].results[j].beacon_period);
                ALOGI("HAL:  capability  %d \n", results[i].results[j].capability);
                /* For GScan cached results, both ie_length and ie data are
                 * zeros. So no need to print them.
                 */
            }
        }
    }
cleanup:
    gScanCommand->freeRspParams(eGScanGetCachedResultsRspParams);
    ALOGI("%s: Delete object.", __func__);
    delete gScanCommand;
    return (wifi_error)ret;
}

/* Random MAC OUI for PNO */
wifi_error wifi_set_scanning_mac_oui(wifi_interface_handle handle, oui scan_oui)
{
    int ret = 0;
    struct nlattr *nlData;
    WifiVendorCommand *vCommand = NULL;
    interface_info *iinfo = getIfaceInfo(handle);
    wifi_handle wifiHandle = getWifiHandle(handle);

    vCommand = new WifiVendorCommand(wifiHandle, 0,
            OUI_QCA,
            QCA_NL80211_VENDOR_SUBCMD_SCANNING_MAC_OUI);
    if (vCommand == NULL) {
        ALOGE("%s: Error vCommand NULL", __func__);
        return WIFI_ERROR_OUT_OF_MEMORY;
    }

    /* create the message */
    ret = vCommand->create();
    if (ret < 0)
        goto cleanup;

    ret = vCommand->set_iface_id(iinfo->name);
    if (ret < 0)
        goto cleanup;

    /* Add the vendor specific attributes for the NL command. */
    nlData = vCommand->attr_start(NL80211_ATTR_VENDOR_DATA);
    if (!nlData)
        goto cleanup;

    ALOGI("MAC_OUI - %02x:%02x:%02x", scan_oui[0], scan_oui[1], scan_oui[2]);

    /* Add the fixed part of the mac_oui to the nl command */
    ret = vCommand->put_bytes(
            QCA_WLAN_VENDOR_ATTR_SET_SCANNING_MAC_OUI,
            (char *)scan_oui,
            WIFI_SCANNING_MAC_OUI_LENGTH);
    if (ret < 0)
        goto cleanup;

    vCommand->attr_end(nlData);

    ret = vCommand->requestResponse();
    if (ret != 0) {
        ALOGE("%s: requestResponse Error:%d",__func__, ret);
        goto cleanup;
    }

cleanup:
    delete vCommand;
    return (wifi_error)ret;
}


void set_ssid_hotlist_cb(int status)
{
    ALOGD("%s: Status = %d.", __func__, status);
}

/* Set the GSCAN SSID Hotlist. */
wifi_error wifi_set_ssid_hotlist(wifi_request_id id,
                                    wifi_interface_handle iface,
                                    wifi_ssid_hotlist_params params,
                                    wifi_hotlist_ssid_handler handler)
{
    int i, numSsid, ret = 0;
    GScanCommand *gScanCommand;
    struct nlattr *nlData, *nlSsidThresholdParamList;
    interface_info *ifaceInfo = getIfaceInfo(iface);
    wifi_handle wifiHandle = getWifiHandle(iface);
    bool previousGScanSetSsidRunning = false;
    hal_info *info = getHalInfo(wifiHandle);

    if (!(info->supported_feature_set & WIFI_FEATURE_GSCAN)) {
        ALOGE("%s: GSCAN is not supported by driver",
            __func__);
        return WIFI_ERROR_NOT_SUPPORTED;
    }

    ALOGD("Setting GScan SSID Hotlist, halHandle = %p", wifiHandle);

    /* Wi-Fi HAL doesn't need to check if a similar request to set ssid
     * hotlist was made earlier. If set_ssid_hotlist() is called while
     * another one is running, the request will be sent down to driver and
     * firmware. If the new request is successfully honored, then Wi-Fi HAL
     * will use the new request id for the GScanSetSsidHotlistCmdEventHandler
     * object.
     */

    gScanCommand =
        new GScanCommand(
                    wifiHandle,
                    id,
                    OUI_QCA,
                    QCA_NL80211_VENDOR_SUBCMD_GSCAN_SET_SSID_HOTLIST);
    if (gScanCommand == NULL) {
        ALOGE("%s: Error GScanCommand NULL", __func__);
        return WIFI_ERROR_UNKNOWN;
    }

    GScanCallbackHandler callbackHandler;
    memset(&callbackHandler, 0, sizeof(callbackHandler));
    callbackHandler.set_ssid_hotlist = set_ssid_hotlist_cb;

    ret = gScanCommand->setCallbackHandler(callbackHandler);
    if (ret < 0)
        goto cleanup;

    /* Create the NL message. */
    ret = gScanCommand->create();
    if (ret < 0)
        goto cleanup;

    /* Set the interface Id of the message. */
    ret = gScanCommand->set_iface_id(ifaceInfo->name);
    if (ret < 0)
        goto cleanup;

    /* Add the vendor specific attributes for the NL command. */
    nlData = gScanCommand->attr_start(NL80211_ATTR_VENDOR_DATA);
    if (!nlData)
        goto cleanup;

    numSsid = (unsigned int)params.num_ssid > MAX_HOTLIST_SSID ?
        MAX_HOTLIST_SSID : params.num_ssid;
    if (gScanCommand->put_u32(
            QCA_WLAN_VENDOR_ATTR_GSCAN_SUBCMD_CONFIG_PARAM_REQUEST_ID,
            id) ||
        gScanCommand->put_u32(
        QCA_WLAN_VENDOR_ATTR_GSCAN_SSID_HOTLIST_PARAMS_LOST_SSID_SAMPLE_SIZE,
            params.lost_ssid_sample_size) ||
        gScanCommand->put_u32(
            QCA_WLAN_VENDOR_ATTR_GSCAN_SSID_HOTLIST_PARAMS_NUM_SSID,
            numSsid))
    {
        goto cleanup;
    }

    /* Add the vendor specific attributes for the NL command. */
    nlSsidThresholdParamList =
        gScanCommand->attr_start(
                            QCA_WLAN_VENDOR_ATTR_GSCAN_SSID_THRESHOLD_PARAM);
    if (!nlSsidThresholdParamList)
        goto cleanup;

    /* Add nested NL attributes for SSID Threshold Param. */
    for (i = 0; i < numSsid; i++) {
        ssid_threshold_param ssidThreshold = params.ssid[i];
        struct nlattr *nlSsidThresholdParam = gScanCommand->attr_start(i);
        if (!nlSsidThresholdParam)
            goto cleanup;
        if (gScanCommand->put_string(
                QCA_WLAN_VENDOR_ATTR_GSCAN_SSID_THRESHOLD_PARAM_SSID,
                ssidThreshold.ssid) ||
            gScanCommand->put_u8(
                QCA_WLAN_VENDOR_ATTR_GSCAN_SSID_THRESHOLD_PARAM_BAND,
                ssidThreshold.band) ||
            gScanCommand->put_s32(
                QCA_WLAN_VENDOR_ATTR_GSCAN_SSID_THRESHOLD_PARAM_RSSI_LOW,
                ssidThreshold.low) ||
            gScanCommand->put_s32(
                QCA_WLAN_VENDOR_ATTR_GSCAN_SSID_THRESHOLD_PARAM_RSSI_HIGH,
                ssidThreshold.high))
        {
            goto cleanup;
        }
        gScanCommand->attr_end(nlSsidThresholdParam);
    }

    gScanCommand->attr_end(nlSsidThresholdParamList);

    gScanCommand->attr_end(nlData);

    ret = gScanCommand->allocRspParams(eGScanSetSsidHotlistRspParams);
    if (ret != 0) {
        ALOGE("%s: Failed to allocate memory to the response struct. "
            "Error:%d", __func__, ret);
        goto cleanup;
    }

    callbackHandler.on_hotlist_ssid_found = handler.on_hotlist_ssid_found;
    callbackHandler.on_hotlist_ssid_lost = handler.on_hotlist_ssid_lost;
    /* Create an object of the event handler class to take care of the
      * asychronous events on the north-bound.
      */
    if (GScanSetSsidHotlistCmdEventHandler == NULL) {
        GScanSetSsidHotlistCmdEventHandler = new GScanCommandEventHandler(
                            wifiHandle,
                            id,
                            OUI_QCA,
                            QCA_NL80211_VENDOR_SUBCMD_GSCAN_SET_SSID_HOTLIST,
                            callbackHandler);
        if (GScanSetSsidHotlistCmdEventHandler == NULL) {
            ALOGE("%s: Error instantiating "
                "GScanSetSsidHotlistCmdEventHandler.", __func__);
            ret = WIFI_ERROR_UNKNOWN;
            goto cleanup;
        }
        ALOGD("%s: Handler object was created for HOTLIST_AP_FOUND.", __func__);
    } else {
        previousGScanSetSsidRunning = true;
        ALOGD("%s: "
                "A HOTLIST_AP_FOUND event handler object already exists "
                "with request id=%d",
                __func__,
                GScanSetSsidHotlistCmdEventHandler->get_request_id());
    }

    gScanCommand->waitForRsp(true);
    ret = gScanCommand->requestEvent();
    if (ret != 0) {
        ALOGE("%s: requestEvent Error:%d",__func__, ret);
        goto cleanup;
    }

    gScanCommand->getSetSsidHotlistRspParams((u32 *)&ret);
    if (ret != 0)
    {
        goto cleanup;
    }
    if (GScanSetSsidHotlistCmdEventHandler != NULL) {
        GScanSetSsidHotlistCmdEventHandler->set_request_id(id);
    }

cleanup:
    gScanCommand->freeRspParams(eGScanSetSsidHotlistRspParams);
    ALOGI("%s: Delete object. ", __func__);
    delete gScanCommand;
    /* Delete the command event handler object if ret != 0 */
    if (!previousGScanSetSsidRunning && ret
        && GScanSetSsidHotlistCmdEventHandler) {
        delete GScanSetSsidHotlistCmdEventHandler;
        GScanSetSsidHotlistCmdEventHandler = NULL;
    }
    return (wifi_error)ret;
}


void reset_ssid_hotlist_cb(int status)
{
    ALOGD("%s: Status = %d.", __func__, status);
}


wifi_error wifi_reset_ssid_hotlist(wifi_request_id id,
                            wifi_interface_handle iface)
{
    int ret = 0;
    GScanCommand *gScanCommand;
    struct nlattr *nlData;
    interface_info *ifaceInfo = getIfaceInfo(iface);
    wifi_handle wifiHandle = getWifiHandle(iface);
    hal_info *info = getHalInfo(wifiHandle);

    if (!(info->supported_feature_set & WIFI_FEATURE_GSCAN)) {
        ALOGE("%s: GSCAN is not supported by driver",
            __func__);
        return WIFI_ERROR_NOT_SUPPORTED;
    }

    ALOGE("Resetting GScan SSID Hotlist, halHandle = %p", wifiHandle);

    if (GScanSetSsidHotlistCmdEventHandler == NULL) {
        ALOGE("wifi_reset_ssid_hotlist: GSCAN ssid_hotlist isn't set. "
            "Nothing to do. Exit");
        return WIFI_ERROR_NOT_AVAILABLE;
    }

    gScanCommand = new GScanCommand(
                        wifiHandle,
                        id,
                        OUI_QCA,
                        QCA_NL80211_VENDOR_SUBCMD_GSCAN_RESET_SSID_HOTLIST);

    if (gScanCommand == NULL) {
        ALOGE("%s: Error GScanCommand NULL", __func__);
        return WIFI_ERROR_UNKNOWN;
    }

    GScanCallbackHandler callbackHandler;
    memset(&callbackHandler, 0, sizeof(callbackHandler));
    callbackHandler.reset_ssid_hotlist = reset_ssid_hotlist_cb;

    ret = gScanCommand->setCallbackHandler(callbackHandler);
    if (ret < 0)
        goto cleanup;

    /* Create the NL message. */
    ret = gScanCommand->create();
    if (ret < 0)
        goto cleanup;

    /* Set the interface Id of the message. */
    ret = gScanCommand->set_iface_id(ifaceInfo->name);
    if (ret < 0)
        goto cleanup;

    /* Add the vendor specific attributes for the NL command. */
    nlData = gScanCommand->attr_start(NL80211_ATTR_VENDOR_DATA);
    if (!nlData)
        goto cleanup;

    ret = gScanCommand->put_u32(
            QCA_WLAN_VENDOR_ATTR_GSCAN_SUBCMD_CONFIG_PARAM_REQUEST_ID, id);
    if (ret < 0)
        goto cleanup;

    gScanCommand->attr_end(nlData);

    ret = gScanCommand->allocRspParams(eGScanResetSsidHotlistRspParams);
    if (ret != 0) {
        ALOGE("%s: Failed to allocate memory to the response struct. "
            "Error:%d", __func__, ret);
        goto cleanup;
    }

    gScanCommand->waitForRsp(true);
    ret = gScanCommand->requestEvent();
    if (ret != 0) {
        ALOGE("%s: requestEvent Error:%d",__func__, ret);
        if (ret == ETIMEDOUT)
        {
            if (GScanSetSsidHotlistCmdEventHandler) {
                delete GScanSetSsidHotlistCmdEventHandler;
                GScanSetSsidHotlistCmdEventHandler = NULL;
            }
        }
        goto cleanup;
    }

    gScanCommand->getResetSsidHotlistRspParams((u32 *)&ret);
    if (ret != 0)
    {
        goto cleanup;
    }
    if (GScanSetSsidHotlistCmdEventHandler) {
        delete GScanSetSsidHotlistCmdEventHandler;
        GScanSetSsidHotlistCmdEventHandler = NULL;
    }

cleanup:
    gScanCommand->freeRspParams(eGScanResetSsidHotlistRspParams);
    ALOGI("%s: Delete object.", __func__);
    delete gScanCommand;
    return (wifi_error)ret;
}


GScanCommand::GScanCommand(wifi_handle handle, int id, u32 vendor_id,
                                  u32 subcmd)
        : WifiVendorCommand(handle, id, vendor_id, subcmd)
{
    ALOGD("GScanCommand %p constructed", this);
    /* Initialize the member data variables here */
    mStartGScanRspParams = NULL;
    mStopGScanRspParams = NULL;
    mSetBssidHotlistRspParams = NULL;
    mResetBssidHotlistRspParams = NULL;
    mSetSignificantChangeRspParams = NULL;
    mResetSignificantChangeRspParams = NULL;
    mGetCapabilitiesRspParams = NULL;
    mGetCachedResultsRspParams = NULL;
    mGetCachedResultsNumResults = 0;
    mSetSsidHotlistRspParams = NULL;
    mResetSsidHotlistRspParams = NULL;
    mChannels = NULL;
    mMaxChannels = 0;
    mNumChannelsPtr = NULL;
    mWaitforRsp = false;

    mRequestId = id;
    memset(&mHandler, 0,sizeof(mHandler));
}

GScanCommand::~GScanCommand()
{
    ALOGD("GScanCommand %p destructor", this);
    unregisterVendorHandler(mVendor_id, mSubcmd);
}


/* This function implements creation of Vendor command */
int GScanCommand::create() {
    int ret = mMsg.create(NL80211_CMD_VENDOR, 0, 0);
    if (ret < 0) {
        return ret;
    }

    /* Insert the oui in the msg */
    ret = mMsg.put_u32(NL80211_ATTR_VENDOR_ID, mVendor_id);
    if (ret < 0)
        goto out;
    /* Insert the subcmd in the msg */
    ret = mMsg.put_u32(NL80211_ATTR_VENDOR_SUBCMD, mSubcmd);
    if (ret < 0)
        goto out;

     ALOGI("%s: mVendor_id = %d, Subcmd = %d.",
        __func__, mVendor_id, mSubcmd);

out:
    return ret;
}

/* Callback handlers registered for nl message send */
static int error_handler_gscan(struct sockaddr_nl *nla, struct nlmsgerr *err,
                                   void *arg)
{
    struct sockaddr_nl *tmp;
    int *ret = (int *)arg;
    tmp = nla;
    *ret = err->error;
    ALOGE("%s: Error code:%d (%s)", __func__, *ret, strerror(-(*ret)));
    return NL_STOP;
}

/* Callback handlers registered for nl message send */
static int ack_handler_gscan(struct nl_msg *msg, void *arg)
{
    int *ret = (int *)arg;
    struct nl_msg * a;

    ALOGE("%s: called", __func__);
    a = msg;
    *ret = 0;
    return NL_STOP;
}

/* Callback handlers registered for nl message send */
static int finish_handler_gscan(struct nl_msg *msg, void *arg)
{
  int *ret = (int *)arg;
  struct nl_msg * a;

  ALOGE("%s: called", __func__);
  a = msg;
  *ret = 0;
  return NL_SKIP;
}

/*
 * Override base class requestEvent and implement little differently here.
 * This will send the request message.
 * We don't wait for any response back in case of gscan as it is asynchronous,
 * thus no wait for condition.
 */
int GScanCommand::requestEvent()
{
    int res = -1;
    struct nl_cb *cb;

    ALOGD("%s: Entry.", __func__);

    cb = nl_cb_alloc(NL_CB_DEFAULT);
    if (!cb) {
        ALOGE("%s: Callback allocation failed",__func__);
        res = -1;
        goto out;
    }

    /* Send message */
    ALOGE("%s:Handle:%p Socket Value:%p", __func__, mInfo, mInfo->cmd_sock);
    res = nl_send_auto_complete(mInfo->cmd_sock, mMsg.getMessage());
    if (res < 0)
        goto out;
    res = 1;

    nl_cb_err(cb, NL_CB_CUSTOM, error_handler_gscan, &res);
    nl_cb_set(cb, NL_CB_FINISH, NL_CB_CUSTOM, finish_handler_gscan, &res);
    nl_cb_set(cb, NL_CB_ACK, NL_CB_CUSTOM, ack_handler_gscan, &res);

    /* Err is populated as part of finish_handler. */
    while (res > 0){
         nl_recvmsgs(mInfo->cmd_sock, cb);
    }

    ALOGD("%s: Msg sent, res=%d, mWaitForRsp=%d", __func__, res, mWaitforRsp);
    /* Only wait for the asynchronous event if HDD returns success, res=0 */
    if (!res && (mWaitforRsp == true)) {
        struct timespec abstime;
        abstime.tv_sec = 4;
        abstime.tv_nsec = 0;
        res = mCondition.wait(abstime);
        if (res == ETIMEDOUT)
        {
            ALOGE("%s: Time out happened.", __func__);
        }
        ALOGD("%s: Command invoked return value:%d, mWaitForRsp=%d",
            __func__, res, mWaitforRsp);
    }
out:
    /* Cleanup the mMsg */
    mMsg.destroy();
    return res;
}

int GScanCommand::requestResponse()
{
    ALOGD("%s: request a response", __func__);
    return WifiCommand::requestResponse(mMsg);
}

int GScanCommand::handleResponse(WifiEvent &reply) {
    ALOGI("Received a GScan response message from Driver");
    u32 status;
    int i = 0;
    WifiVendorCommand::handleResponse(reply);

    switch(mSubcmd)
    {
        case QCA_NL80211_VENDOR_SUBCMD_GSCAN_GET_VALID_CHANNELS:
            {
                struct nlattr *tb_vendor[
                    QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_MAX + 1];
                nla_parse(tb_vendor, QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_MAX,
                            (struct nlattr *)mVendorData,mDataLen, NULL);

                if (tb_vendor[
                    QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_NUM_CHANNELS]) {
                    u32 val;
                    val = nla_get_u32(
                        tb_vendor[
                        QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_NUM_CHANNELS]);

                    ALOGD("%s: Num channels : %d", __func__, val);
                    val = val > (unsigned int)mMaxChannels ?
                          (unsigned int)mMaxChannels : val;
                    *mNumChannelsPtr = val;

                    /* Extract the list of channels. */
                    if (*mNumChannelsPtr > 0 &&
                        tb_vendor[
                        QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CHANNELS]) {
                        nla_memcpy(mChannels,
                            tb_vendor[
                            QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CHANNELS],
                            sizeof(wifi_channel) * (*mNumChannelsPtr));
                    }

                    ALOGD("%s: Get valid channels response received.",
                        __func__);
                    ALOGD("%s: Num channels : %d",
                        __func__, *mNumChannelsPtr);
                    ALOGD("%s: List of valid channels is: ", __func__);
                    for(i = 0; i < *mNumChannelsPtr; i++)
                    {
                        ALOGD("%u", *(mChannels + i));
                    }
                }
            }
            break;
        default :
            ALOGE("%s: Wrong GScan subcmd response received %d",
                __func__, mSubcmd);
    }
    return NL_SKIP;
}

/* Called to parse and extract cached results. */
int GScanCommand:: gscan_get_cached_results(u32 num_results,
                                          wifi_cached_scan_results *cached_results,
                                          u32 starting_index,
                                          struct nlattr **tb_vendor)
{
    u32 i = starting_index, j = 0;
    struct nlattr *scanResultsInfo, *wifiScanResultsInfo;
    int rem = 0;
    u32 len = 0;
    ALOGE("starting counter: %d", i);

    for (scanResultsInfo = (struct nlattr *) nla_data(tb_vendor[
               QCA_WLAN_VENDOR_ATTR_GSCAN_CACHED_RESULTS_LIST]),
               rem = nla_len(tb_vendor[
               QCA_WLAN_VENDOR_ATTR_GSCAN_CACHED_RESULTS_LIST
               ]);
           nla_ok(scanResultsInfo, rem) && i < mGetCachedResultsRspParams->max;
           scanResultsInfo = nla_next(scanResultsInfo, &(rem)))
       {
           struct nlattr *tb2[QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_MAX + 1];
           nla_parse(tb2, QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_MAX,
           (struct nlattr *) nla_data(scanResultsInfo),
                   nla_len(scanResultsInfo), NULL);

           if (!
               tb2[
                   QCA_WLAN_VENDOR_ATTR_GSCAN_CACHED_RESULTS_SCAN_ID
                   ])
           {
               ALOGE("gscan_get_cached_results: GSCAN_CACHED_RESULTS_SCAN_ID"
                   " not found");
               return WIFI_ERROR_INVALID_ARGS;
           }
           cached_results[i].scan_id =
               nla_get_u32(
               tb2[
                   QCA_WLAN_VENDOR_ATTR_GSCAN_CACHED_RESULTS_SCAN_ID
                   ]);

           if (!
               tb2[
                   QCA_WLAN_VENDOR_ATTR_GSCAN_CACHED_RESULTS_FLAGS
                   ])
           {
               ALOGE("gscan_get_cached_results: GSCAN_CACHED_RESULTS_FLAGS "
                   "not found");
               return WIFI_ERROR_INVALID_ARGS;
           }
           cached_results[i].flags =
               nla_get_u32(
               tb2[QCA_WLAN_VENDOR_ATTR_GSCAN_CACHED_RESULTS_FLAGS]);

           if (!
               tb2[
                   QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_NUM_RESULTS_AVAILABLE
                   ])
           {
               ALOGE("gscan_get_cached_results: RESULTS_NUM_RESULTS_AVAILABLE "
                   "not found");
               return WIFI_ERROR_INVALID_ARGS;
           }
           cached_results[i].num_results =
               nla_get_u32(
               tb2[QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_NUM_RESULTS_AVAILABLE]);

           if (mGetCachedResultsRspParams->lastProcessedScanId !=
                                        cached_results[i].scan_id) {
                ALOGD("parsing: *lastProcessedScanId [%d] !="
                    " cached_results[i].scan_id:%d, j:%d ",
                    mGetCachedResultsRspParams->lastProcessedScanId,
                    cached_results[i].scan_id, j);
               mGetCachedResultsRspParams->lastProcessedScanId =
                cached_results[i].scan_id;
               mGetCachedResultsRspParams->wifiScanResultsStartingIndex =
                cached_results[i].num_results;
           } else {
               j = mGetCachedResultsRspParams->wifiScanResultsStartingIndex;
               mGetCachedResultsRspParams->wifiScanResultsStartingIndex +=
                cached_results[i].num_results;
               cached_results[i].num_results =
                mGetCachedResultsRspParams->wifiScanResultsStartingIndex;
                ALOGD("parsing: *lastProcessedScanId [%d] == "
                    "cached_results[i].scan_id:%d, j:%d ",
                    mGetCachedResultsRspParams->lastProcessedScanId,
                    cached_results[i].scan_id, j);
           }

           if (!cached_results[i].results) {
               ALOGE("gscan_get_cached_results:NULL cached_results[%d].results"
                   ". Abort.", i);
               return WIFI_ERROR_OUT_OF_MEMORY;
           }

           ALOGE("gscan_get_cached_results: scan_id %d ",
            cached_results[i].scan_id);
           ALOGE("gscan_get_cached_results: flags  %u ",
            cached_results[i].flags);
           ALOGE("gscan_get_cached_results: num_results %d ",
            cached_results[i].num_results);

           for (wifiScanResultsInfo = (struct nlattr *) nla_data(tb2[
                QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_LIST]),
                rem = nla_len(tb2[
                QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_LIST
                ]);
                nla_ok(wifiScanResultsInfo, rem);
                wifiScanResultsInfo = nla_next(wifiScanResultsInfo, &(rem)))
           {
                struct nlattr *tb3[QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_MAX + 1];
                nla_parse(tb3, QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_MAX,
                        (struct nlattr *) nla_data(wifiScanResultsInfo),
                        nla_len(wifiScanResultsInfo), NULL);
                if (j < MAX_AP_CACHE_PER_SCAN) {
                    if (!
                        tb3[
                           QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_RESULT_TIME_STAMP
                           ])
                    {
                        ALOGE("gscan_get_cached_results: "
                            "RESULTS_SCAN_RESULT_TIME_STAMP not found");
                        return WIFI_ERROR_INVALID_ARGS;
                    }
                    cached_results[i].results[j].ts =
                        nla_get_u64(
                        tb3[
                            QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_RESULT_TIME_STAMP
                            ]);
                    if (!
                        tb3[
                            QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_RESULT_SSID
                            ])
                    {
                        ALOGE("gscan_get_cached_results: "
                            "RESULTS_SCAN_RESULT_SSID not found");
                        return WIFI_ERROR_INVALID_ARGS;
                    }
                    len = nla_len(tb3[
                            QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_RESULT_SSID]);
                    len =
                        sizeof(cached_results[i].results[j].ssid) <= len ?
                        sizeof(cached_results[i].results[j].ssid) : len;
                    memcpy((void *)&cached_results[i].results[j].ssid,
                        nla_data(
                        tb3[
                        QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_RESULT_SSID]),
                        len);
                    if (!
                        tb3[
                            QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_RESULT_BSSID
                            ])
                    {
                        ALOGE("gscan_get_cached_results: "
                            "RESULTS_SCAN_RESULT_BSSID not found");
                        return WIFI_ERROR_INVALID_ARGS;
                    }
                    len = nla_len(
                        tb3[
                        QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_RESULT_BSSID]);
                    len =
                        sizeof(cached_results[i].results[j].bssid) <= len ?
                        sizeof(cached_results[i].results[j].bssid) : len;
                    memcpy(&cached_results[i].results[j].bssid,
                        nla_data(
                        tb3[
                        QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_RESULT_BSSID]),
                        len);
                    if (!
                        tb3[
                            QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_RESULT_CHANNEL
                            ])
                    {
                        ALOGE("gscan_get_cached_results: "
                            "RESULTS_SCAN_RESULT_CHANNEL not found");
                        return WIFI_ERROR_INVALID_ARGS;
                    }
                    cached_results[i].results[j].channel =
                        nla_get_u32(
                        tb3[QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_RESULT_CHANNEL]);
                    if (!
                        tb3[
                            QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_RESULT_RSSI
                            ])
                    {
                        ALOGE("gscan_get_cached_results: "
                            "RESULTS_SCAN_RESULT_RSSI not found");
                        return WIFI_ERROR_INVALID_ARGS;
                    }
                    cached_results[i].results[j].rssi =
                        get_s32(
                        tb3[QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_RESULT_RSSI]);
                    if (!
                        tb3[
                            QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_RESULT_RTT
                            ])
                    {
                        ALOGE("gscan_get_cached_results: "
                            "RESULTS_SCAN_RESULT_RTT not found");
                        return WIFI_ERROR_INVALID_ARGS;
                    }
                    cached_results[i].results[j].rtt =
                        nla_get_u32(
                        tb3[QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_RESULT_RTT]);
                    if (!
                        tb3[
                            QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_RESULT_RTT_SD
                        ])
                    {
                        ALOGE("gscan_get_cached_results: "
                            "RESULTS_SCAN_RESULT_RTT_SD not found");
                        return WIFI_ERROR_INVALID_ARGS;
                    }
                    cached_results[i].results[j].rtt_sd =
                        nla_get_u32(
                        tb3[QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_RESULT_RTT_SD]);

                    ALOGD("gscan_get_cached_results: ts  %lld ",
                        cached_results[i].results[j].ts);
                    ALOGD("gscan_get_cached_results: SSID  %s ",
                        cached_results[i].results[j].ssid);
                    ALOGD("gscan_get_cached_results: "
                        "BSSID: %02x:%02x:%02x:%02x:%02x:%02x \n",
                        cached_results[i].results[j].bssid[0],
                        cached_results[i].results[j].bssid[1],
                        cached_results[i].results[j].bssid[2],
                        cached_results[i].results[j].bssid[3],
                        cached_results[i].results[j].bssid[4],
                        cached_results[i].results[j].bssid[5]);
                    ALOGD("gscan_get_cached_results: channel %d ",
                        cached_results[i].results[j].channel);
                    ALOGD("gscan_get_cached_results: rssi  %d ",
                        cached_results[i].results[j].rssi);
                    ALOGD("gscan_get_cached_results: rtt  %lld ",
                        cached_results[i].results[j].rtt);
                    ALOGD("gscan_get_cached_results: rtt_sd  %lld ",
                        cached_results[i].results[j].rtt_sd);
                } else {
                    /* We already parsed and stored up to max wifi_scan_results
                     * specified by the caller. Now, continue to loop over NL
                     * entries in order to properly update NL parsing pointer
                     * so it points to the next scan_id results.
                     */
                    ALOGD("gscan_get_cached_results: loop index:%d > max num"
                        " of wifi_scan_results:%d for gscan cached results"
                        " bucket:%d. Dummy loop",
                        j, MAX_AP_CACHE_PER_SCAN, i);
                }
                /* Increment loop index for next record */
                j++;
           }
           /* Increment loop index for next record */
           i++;
       }
   return WIFI_SUCCESS;
}

void set_pno_list_cb(int status)
{
    ALOGD("%s: Status = %d.", __func__, status);
}

/* Set the GSCAN BSSID Hotlist. */
wifi_error wifi_set_epno_list(wifi_request_id id,
                                wifi_interface_handle iface,
                                int num_networks,
                                wifi_epno_network *networks,
                                wifi_epno_handler handler)
{
    int i, ret = 0;
    GScanCommand *gScanCommand;
    struct nlattr *nlData, *nlPnoParamList;
    interface_info *ifaceInfo = getIfaceInfo(iface);
    wifi_handle wifiHandle = getWifiHandle(iface);
    bool previousGScanSetEpnoListRunning = false;
    hal_info *info = getHalInfo(wifiHandle);

    if (!(info->supported_feature_set & WIFI_FEATURE_HAL_EPNO)) {
        ALOGE("%s: Enhanced PNO is not supported by the driver",
            __func__);
        return WIFI_ERROR_NOT_SUPPORTED;
    }

    ALOGE("Setting GScan EPNO List, halHandle = %p", wifiHandle);

    /* Wi-Fi HAL doesn't need to check if a similar request to set ePNO
     * list was made earlier. If wifi_set_epno_list() is called while
     * another one is running, the request will be sent down to driver and
     * firmware. If the new request is successfully honored, then Wi-Fi HAL
     * will use the new request id for the GScanSetPnoListCmdEventHandler
     * object.
     */

    gScanCommand =
        new GScanCommand(
                    wifiHandle,
                    id,
                    OUI_QCA,
                    QCA_NL80211_VENDOR_SUBCMD_PNO_SET_LIST);
    if (gScanCommand == NULL) {
        ALOGE("%s: Error GScanCommand NULL", __func__);
        return WIFI_ERROR_UNKNOWN;
    }

    GScanCallbackHandler callbackHandler;
    memset(&callbackHandler, 0, sizeof(callbackHandler));
    callbackHandler.set_epno_list = set_pno_list_cb;

    ret = gScanCommand->setCallbackHandler(callbackHandler);
    if (ret < 0) {
        ALOGE("%s: Failed to set callback handler. Error:%d", __func__, ret);
        goto cleanup;
    }

    /* Create the NL message. */
    ret = gScanCommand->create();
    if (ret < 0) {
        ALOGE("%s: Failed to create the NL msg. Error:%d", __func__, ret);
        goto cleanup;
    }

    /* Set the interface Id of the message. */
    ret = gScanCommand->set_iface_id(ifaceInfo->name);
    if (ret < 0) {
        ALOGE("%s: Failed to set iface id. Error:%d", __func__, ret);
        goto cleanup;
    }

    /* Add the vendor specific attributes for the NL command. */
    nlData = gScanCommand->attr_start(NL80211_ATTR_VENDOR_DATA);
    if (!nlData) {
        ALOGE("%s: Failed to add attribute NL80211_ATTR_VENDOR_DATA. Error:%d",
            __func__, ret);
        goto cleanup;
    }

    num_networks = (unsigned int)num_networks > MAX_PNO_SSID ?
        MAX_PNO_SSID : num_networks;
    if (gScanCommand->put_u32(
            QCA_WLAN_VENDOR_ATTR_GSCAN_SUBCMD_CONFIG_PARAM_REQUEST_ID,
            id) ||
        gScanCommand->put_u32(
            QCA_WLAN_VENDOR_ATTR_PNO_SET_LIST_PARAM_NUM_NETWORKS,
            num_networks))
    {
        ALOGE("%s: Failed to add vendor atributes. Error:%d", __func__, ret);
        goto cleanup;
    }

    /* Add the vendor specific attributes for the NL command. */
    nlPnoParamList =
        gScanCommand->attr_start(
                QCA_WLAN_VENDOR_ATTR_PNO_SET_LIST_PARAM_EPNO_NETWORKS_LIST);
    if (!nlPnoParamList) {
        ALOGE("%s: Failed to add attr. PNO_SET_LIST_PARAM_EPNO_NETWORKS_LIST. "
            "Error:%d", __func__, ret);
        goto cleanup;
    }

    /* Add nested NL attributes for ePno List. */
    for (i = 0; i < num_networks; i++) {
        wifi_epno_network pnoNetwork = networks[i];
        struct nlattr *nlPnoNetwork = gScanCommand->attr_start(i);
        if (!nlPnoNetwork) {
            ALOGE("%s: Failed attr_start for nlPnoNetwork. Error:%d",
                __func__, ret);
            goto cleanup;
        }
        if (gScanCommand->put_string(
                QCA_WLAN_VENDOR_ATTR_PNO_SET_LIST_PARAM_EPNO_NETWORK_SSID,
                pnoNetwork.ssid) ||
                gScanCommand->put_s8(
           QCA_WLAN_VENDOR_ATTR_PNO_SET_LIST_PARAM_EPNO_NETWORK_RSSI_THRESHOLD,
                pnoNetwork.rssi_threshold) ||
            gScanCommand->put_u8(
                QCA_WLAN_VENDOR_ATTR_PNO_SET_LIST_PARAM_EPNO_NETWORK_FLAGS,
                pnoNetwork.flags) ||
            gScanCommand->put_u8(
                QCA_WLAN_VENDOR_ATTR_PNO_SET_LIST_PARAM_EPNO_NETWORK_AUTH_BIT,
                pnoNetwork.auth_bit_field))
        {
            ALOGE("%s: Failed to add PNO_SET_LIST_PARAM_EPNO_NETWORK_*. "
                "Error:%d", __func__, ret);
            goto cleanup;
        }
        gScanCommand->attr_end(nlPnoNetwork);
    }

    gScanCommand->attr_end(nlPnoParamList);

    gScanCommand->attr_end(nlData);

    callbackHandler.on_pno_network_found = handler.on_network_found;

    /* Create an object of the event handler class to take care of the
      * asychronous events on the north-bound.
      */
    if (GScanSetPnoListCmdEventHandler == NULL) {
        GScanSetPnoListCmdEventHandler = new GScanCommandEventHandler(
                            wifiHandle,
                            id,
                            OUI_QCA,
                            QCA_NL80211_VENDOR_SUBCMD_PNO_SET_LIST,
                            callbackHandler);
        if (GScanSetPnoListCmdEventHandler == NULL) {
            ALOGE("%s: Error instantiating "
                "GScanSetPnoListCmdEventHandler.", __func__);
            ret = WIFI_ERROR_UNKNOWN;
            goto cleanup;
        }
        ALOGD("%s: Handler object was created for PNO_NETWORK_FOUND.",
            __func__);
    } else {
        previousGScanSetEpnoListRunning = true;
        ALOGD("%s: "
                "A PNO_NETWORK_FOUND event handler object already exists"
                " with request id=%d",
                __func__,
                GScanSetPnoListCmdEventHandler->get_request_id());
    }

    gScanCommand->waitForRsp(false);
    ret = gScanCommand->requestEvent();
    if (ret != 0) {
        ALOGE("%s: requestEvent Error:%d",__func__, ret);
        goto cleanup;
    }

    if (GScanSetPnoListCmdEventHandler != NULL) {
        GScanSetPnoListCmdEventHandler->set_request_id(id);
    }

cleanup:
    ALOGI("%s: Delete object. ", __func__);
    delete gScanCommand;
    /* Delete the command event handler object if ret != 0 */
    if (!previousGScanSetEpnoListRunning && ret
        && GScanSetPnoListCmdEventHandler) {
        delete GScanSetPnoListCmdEventHandler;
        GScanSetPnoListCmdEventHandler = NULL;
    }
    return (wifi_error)ret;
}

void set_passpoint_list_cb(int status)
{
    ALOGD("%s: Status = %d.", __func__, status);
}

/* Set the ePNO Passpoint List. */
wifi_error wifi_set_passpoint_list(wifi_request_id id,
                                    wifi_interface_handle iface, int num,
                                    wifi_passpoint_network *networks,
                                    wifi_passpoint_event_handler handler)
{
    int i, numAp, ret = 0;
    GScanCommand *gScanCommand;
    struct nlattr *nlData, *nlPasspointNetworksParamList;
    interface_info *ifaceInfo = getIfaceInfo(iface);
    wifi_handle wifiHandle = getWifiHandle(iface);
    bool previousGScanPnoSetPasspointListRunning = false;
    hal_info *info = getHalInfo(wifiHandle);

    if (!(info->supported_feature_set & WIFI_FEATURE_HAL_EPNO)) {
        ALOGE("%s: Enhanced PNO is not supported by the driver",
            __func__);
        return WIFI_ERROR_NOT_SUPPORTED;
    }

    ALOGD("Setting ePNO Passpoint List, halHandle = %p", wifiHandle);

    /* Wi-Fi HAL doesn't need to check if a similar request to set ePNO
     * passpoint list was made earlier. If wifi_set_passpoint_list() is called
     * while another one is running, the request will be sent down to driver and
     * firmware. If the new request is successfully honored, then Wi-Fi HAL
     * will use the new request id for the
     * GScanPnoSetPasspointListCmdEventHandler object.
     */
    gScanCommand =
        new GScanCommand(
                    wifiHandle,
                    id,
                    OUI_QCA,
                    QCA_NL80211_VENDOR_SUBCMD_PNO_SET_PASSPOINT_LIST);
    if (gScanCommand == NULL) {
        ALOGE("%s: Error GScanCommand NULL", __func__);
        return WIFI_ERROR_UNKNOWN;
    }

    GScanCallbackHandler callbackHandler;
    memset(&callbackHandler, 0, sizeof(callbackHandler));
    callbackHandler.set_passpoint_list = set_passpoint_list_cb;

    ret = gScanCommand->setCallbackHandler(callbackHandler);
    if (ret < 0) {
        ALOGE("%s: Failed to set callback handler. Error:%d", __func__, ret);
        goto cleanup;
    }

    /* Create the NL message. */
    ret = gScanCommand->create();
    if (ret < 0) {
        ALOGE("%s: Failed to create the NL msg. Error:%d", __func__, ret);
        goto cleanup;
    }

    /* Set the interface Id of the message. */
    ret = gScanCommand->set_iface_id(ifaceInfo->name);
    if (ret < 0) {
        ALOGE("%s: Failed to set iface id. Error:%d", __func__, ret);
        goto cleanup;
    }

    /* Add the vendor specific attributes for the NL command. */
    nlData = gScanCommand->attr_start(NL80211_ATTR_VENDOR_DATA);
    if (!nlData) {
        ALOGE("%s: Failed to add attribute NL80211_ATTR_VENDOR_DATA. Error:%d",
            __func__, ret);
        goto cleanup;
    }

    if (gScanCommand->put_u32(
            QCA_WLAN_VENDOR_ATTR_GSCAN_SUBCMD_CONFIG_PARAM_REQUEST_ID,
            id) ||
        gScanCommand->put_u32(
            QCA_WLAN_VENDOR_ATTR_PNO_PASSPOINT_LIST_PARAM_NUM,
            num))
    {
        ALOGE("%s: Failed to add vendor atributes. Error:%d", __func__, ret);
        goto cleanup;
    }

    /* Add the vendor specific attributes for the NL command. */
    nlPasspointNetworksParamList =
        gScanCommand->attr_start(
            QCA_WLAN_VENDOR_ATTR_PNO_PASSPOINT_LIST_PARAM_NETWORK_ARRAY);
    if (!nlPasspointNetworksParamList) {
        ALOGE("%s: Failed attr_start for PASSPOINT_LIST_PARAM_NETWORK_ARRAY. "
            "Error:%d", __func__, ret);
        goto cleanup;
    }

    /* Add nested NL attributes for Passpoint List param. */
    for (i = 0; i < num; i++) {
        wifi_passpoint_network passpointNetwork = networks[i];
        struct nlattr *nlPasspointNetworkParam = gScanCommand->attr_start(i);
        if (!nlPasspointNetworkParam) {
            ALOGE("%s: Failed attr_start for nlPasspointNetworkParam. "
                "Error:%d", __func__, ret);
            goto cleanup;
        }
        if (gScanCommand->put_u32(
                QCA_WLAN_VENDOR_ATTR_PNO_PASSPOINT_NETWORK_PARAM_ID,
                passpointNetwork.id) ||
            gScanCommand->put_string(
                QCA_WLAN_VENDOR_ATTR_PNO_PASSPOINT_NETWORK_PARAM_REALM,
                passpointNetwork.realm) ||
            gScanCommand->put_bytes(
         QCA_WLAN_VENDOR_ATTR_PNO_PASSPOINT_NETWORK_PARAM_ROAM_CNSRTM_ID,
                (char*)passpointNetwork.roamingConsortiumIds,
                16 * sizeof(int64_t)) ||
            gScanCommand->put_bytes(
            QCA_WLAN_VENDOR_ATTR_PNO_PASSPOINT_NETWORK_PARAM_ROAM_PLMN,
                (char*)passpointNetwork.plmn, 3 * sizeof(u8)))
        {
            ALOGE("%s: Failed to add PNO_PASSPOINT_NETWORK_PARAM_ROAM_* attr. "
                "Error:%d", __func__, ret);
            goto cleanup;
        }
        gScanCommand->attr_end(nlPasspointNetworkParam);
    }

    gScanCommand->attr_end(nlPasspointNetworksParamList);

    gScanCommand->attr_end(nlData);

    callbackHandler.on_passpoint_network_found =
                        handler.on_passpoint_network_found;
    /* Create an object of the event handler class to take care of the
      * asychronous events on the north-bound.
      */
    if (GScanPnoSetPasspointListCmdEventHandler == NULL) {
        GScanPnoSetPasspointListCmdEventHandler = new GScanCommandEventHandler(
                        wifiHandle,
                        id,
                        OUI_QCA,
                        QCA_NL80211_VENDOR_SUBCMD_PNO_SET_PASSPOINT_LIST,
                        callbackHandler);
        if (GScanPnoSetPasspointListCmdEventHandler == NULL) {
            ALOGE("%s: Error instantiating "
                "GScanPnoSetPasspointListCmdEventHandler.", __func__);
            ret = WIFI_ERROR_UNKNOWN;
            goto cleanup;
        }
        ALOGD("%s: Handler object was created for PNO_PASSPOINT_"
            "NETWORK_FOUND.", __func__);
    } else {
        previousGScanPnoSetPasspointListRunning = true;
        ALOGD("%s: "
                "A PNO_PASSPOINT_NETWORK_FOUND event handler object "
                "already exists with request id=%d",
                __func__,
                GScanPnoSetPasspointListCmdEventHandler->get_request_id());
    }

    gScanCommand->waitForRsp(false);
    ret = gScanCommand->requestEvent();
    if (ret != 0) {
        ALOGE("%s: requestEvent Error:%d",__func__, ret);
        goto cleanup;
    }

    if (GScanPnoSetPasspointListCmdEventHandler != NULL) {
        GScanPnoSetPasspointListCmdEventHandler->set_request_id(id);
    }

cleanup:
    ALOGI("%s: Delete object. ", __func__);
    delete gScanCommand;
    /* Delete the command event handler object if ret != 0 */
    if (!previousGScanPnoSetPasspointListRunning && ret
        && GScanPnoSetPasspointListCmdEventHandler) {
        delete GScanPnoSetPasspointListCmdEventHandler;
        GScanPnoSetPasspointListCmdEventHandler = NULL;
    }
    return (wifi_error)ret;
}

void reset_passpoint_list_cb(int status)
{
    ALOGD("%s: Status = %d.", __func__, status);
}

wifi_error wifi_reset_passpoint_list(wifi_request_id id,
                            wifi_interface_handle iface)
{
    int ret = 0;
    GScanCommand *gScanCommand;
    struct nlattr *nlData;
    interface_info *ifaceInfo = getIfaceInfo(iface);
    wifi_handle wifiHandle = getWifiHandle(iface);
    hal_info *info = getHalInfo(wifiHandle);

    if (!(info->supported_feature_set & WIFI_FEATURE_HAL_EPNO)) {
        ALOGE("%s: Enhanced PNO is not supported by the driver",
            __func__);
        return WIFI_ERROR_NOT_SUPPORTED;
    }

    ALOGE("Resetting ePNO Passpoint List, halHandle = %p", wifiHandle);

    if (GScanPnoSetPasspointListCmdEventHandler == NULL) {
        ALOGE("wifi_reset_passpoint_list: ePNO passpoint_list isn't set. "
            "Nothing to do. Exit");
        return WIFI_ERROR_NOT_AVAILABLE;
    }

    gScanCommand = new GScanCommand(
                    wifiHandle,
                    id,
                    OUI_QCA,
                    QCA_NL80211_VENDOR_SUBCMD_PNO_RESET_PASSPOINT_LIST);

    if (gScanCommand == NULL) {
        ALOGE("%s: Error GScanCommand NULL", __func__);
        return WIFI_ERROR_UNKNOWN;
    }

    GScanCallbackHandler callbackHandler;
    memset(&callbackHandler, 0, sizeof(callbackHandler));
    callbackHandler.reset_passpoint_list = reset_passpoint_list_cb;

    ret = gScanCommand->setCallbackHandler(callbackHandler);
    if (ret < 0) {
        ALOGE("%s: Failed to set callback handler. Error:%d", __func__, ret);
        goto cleanup;
    }

    /* Create the NL message. */
    ret = gScanCommand->create();
    if (ret < 0) {
        ALOGE("%s: Failed to create the NL msg. Error:%d", __func__, ret);
        goto cleanup;
    }

    /* Set the interface Id of the message. */
    ret = gScanCommand->set_iface_id(ifaceInfo->name);
    if (ret < 0) {
        ALOGE("%s: Failed to set iface id. Error:%d", __func__, ret);
        goto cleanup;
    }

    /* Add the vendor specific attributes for the NL command. */
    nlData = gScanCommand->attr_start(NL80211_ATTR_VENDOR_DATA);
    if (!nlData) {
        ALOGE("%s: Failed to add attribute NL80211_ATTR_VENDOR_DATA. Error:%d",
            __func__, ret);
        goto cleanup;
    }

    ret = gScanCommand->put_u32(
            QCA_WLAN_VENDOR_ATTR_GSCAN_SUBCMD_CONFIG_PARAM_REQUEST_ID, id);
    if (ret < 0) {
        ALOGE("%s: Failed to add vendor data attributes. Error:%d",
            __func__, ret);
        goto cleanup;
    }

    gScanCommand->attr_end(nlData);

    gScanCommand->waitForRsp(false);
    ret = gScanCommand->requestEvent();
    if (ret != 0) {
        ALOGE("%s: requestEvent Error:%d",__func__, ret);
        if (ret == ETIMEDOUT)
        {
            if (GScanPnoSetPasspointListCmdEventHandler) {
                delete GScanPnoSetPasspointListCmdEventHandler;
                GScanPnoSetPasspointListCmdEventHandler = NULL;
            }
        }
        goto cleanup;
    }

    if (GScanPnoSetPasspointListCmdEventHandler) {
        delete GScanPnoSetPasspointListCmdEventHandler;
        GScanPnoSetPasspointListCmdEventHandler = NULL;
    }

cleanup:
    ALOGI("%s: Delete object.", __func__);
    delete gScanCommand;
    return (wifi_error)ret;
}

/* This function will be the main handler for incoming (from driver)  GSscan_SUBCMD.
 *  Calls the appropriate callback handler after parsing the vendor data.
 */
int GScanCommand::handleEvent(WifiEvent &event)
{
    ALOGI("Got a GSCAN Event message from the Driver.");
    unsigned i = 0;
    u32 status;
    int ret = WIFI_SUCCESS;
    WifiVendorCommand::handleEvent(event);

    struct nlattr *tbVendor[
        QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_MAX + 1];
    nla_parse(tbVendor, QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_MAX,
            (struct nlattr *)mVendorData,
            mDataLen, NULL);

    switch(mSubcmd)
    {
        case QCA_NL80211_VENDOR_SUBCMD_GSCAN_START:
        {
            if (mStartGScanRspParams){
                mStartGScanRspParams->status =
                    nla_get_u32(tbVendor[QCA_WLAN_VENDOR_ATTR_GSCAN_STATUS]);
                if (mHandler.start)
                    (*mHandler.start)(mStartGScanRspParams->status);
            }
            waitForRsp(false);
        }
        break;

        case QCA_NL80211_VENDOR_SUBCMD_GSCAN_STOP:
        {
            if (mStopGScanRspParams){
                mStopGScanRspParams->status =
                    nla_get_u32(tbVendor[QCA_WLAN_VENDOR_ATTR_GSCAN_STATUS]);
                if (mHandler.stop)
                    (*mHandler.stop)(mStopGScanRspParams->status);
            }
            waitForRsp(false);
        }
        break;

        case QCA_NL80211_VENDOR_SUBCMD_GSCAN_SET_BSSID_HOTLIST:
        {
            if (mSetBssidHotlistRspParams){
                mSetBssidHotlistRspParams->status =
                    nla_get_u32(tbVendor[QCA_WLAN_VENDOR_ATTR_GSCAN_STATUS]);
                if (mHandler.set_bssid_hotlist)
                    (*mHandler.set_bssid_hotlist)
                            (mSetBssidHotlistRspParams->status);
            }
            waitForRsp(false);
        }
        break;

        case QCA_NL80211_VENDOR_SUBCMD_GSCAN_RESET_BSSID_HOTLIST:
        {
            if (mResetBssidHotlistRspParams){
                mResetBssidHotlistRspParams->status =
                    nla_get_u32(tbVendor[QCA_WLAN_VENDOR_ATTR_GSCAN_STATUS]);
                if (mHandler.reset_bssid_hotlist)
                    (*mHandler.reset_bssid_hotlist)
                            (mResetBssidHotlistRspParams->status);
            }
            waitForRsp(false);
        }
        break;

        case QCA_NL80211_VENDOR_SUBCMD_GSCAN_SET_SIGNIFICANT_CHANGE:
        {
            if (mSetSignificantChangeRspParams){
                mSetSignificantChangeRspParams->status =
                    nla_get_u32(tbVendor[QCA_WLAN_VENDOR_ATTR_GSCAN_STATUS]);
                if (mHandler.set_significant_change)
                    (*mHandler.set_significant_change)
                            (mSetSignificantChangeRspParams->status);
            }
            waitForRsp(false);
        }
        break;

        case QCA_NL80211_VENDOR_SUBCMD_GSCAN_RESET_SIGNIFICANT_CHANGE:
        {
            if (mResetSignificantChangeRspParams){
                mResetSignificantChangeRspParams->status =
                    nla_get_u32(tbVendor[QCA_WLAN_VENDOR_ATTR_GSCAN_STATUS]);
                if (mHandler.reset_significant_change)
                    (*mHandler.reset_significant_change)
                            (mResetSignificantChangeRspParams->status);
            }
            waitForRsp(false);
        }
        break;

        case QCA_NL80211_VENDOR_SUBCMD_GSCAN_SET_SSID_HOTLIST:
        {
            if (mSetSsidHotlistRspParams){
                mSetSsidHotlistRspParams->status =
                    nla_get_u32(tbVendor[QCA_WLAN_VENDOR_ATTR_GSCAN_STATUS]);
                if (mHandler.set_ssid_hotlist)
                    (*mHandler.set_ssid_hotlist)
                            (mSetSsidHotlistRspParams->status);
            }
            waitForRsp(false);
        }
        break;

        case QCA_NL80211_VENDOR_SUBCMD_GSCAN_RESET_SSID_HOTLIST:
        {
            if (mResetSsidHotlistRspParams){
                mResetSsidHotlistRspParams->status =
                    nla_get_u32(tbVendor[QCA_WLAN_VENDOR_ATTR_GSCAN_STATUS]);
                if (mHandler.reset_ssid_hotlist)
                    (*mHandler.reset_ssid_hotlist)
                            (mResetSsidHotlistRspParams->status);
            }
            waitForRsp(false);
        }
        break;

        case QCA_NL80211_VENDOR_SUBCMD_GSCAN_GET_CAPABILITIES:
        {
            if (!mGetCapabilitiesRspParams){
                ALOGE("%s: mGetCapabilitiesRspParams ptr is NULL. Exit. ",
                    __func__);
                break;
            }

            if (!tbVendor[QCA_WLAN_VENDOR_ATTR_GSCAN_STATUS]) {
                ALOGE("%s: QCA_WLAN_VENDOR_ATTR_GSCAN_STATUS "
                    "not found", __func__);
                ret = WIFI_ERROR_INVALID_ARGS;
                break;
            }
            mGetCapabilitiesRspParams->status =
                nla_get_u32(tbVendor[QCA_WLAN_VENDOR_ATTR_GSCAN_STATUS]);

            if (!tbVendor[
            QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CAPABILITIES_MAX_SCAN_CACHE_SIZE
                    ]) {
                ALOGE("%s: QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_"
                    "CAPABILITIES_MAX_SCAN_CACHE_SIZE not found", __func__);
                ret = WIFI_ERROR_INVALID_ARGS;
                break;
            }
            mGetCapabilitiesRspParams->capabilities.max_scan_cache_size =
                nla_get_u32(tbVendor[
                QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CAPABILITIES_MAX_SCAN_CACHE_SIZE]);

            if (!tbVendor[
            QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CAPABILITIES_MAX_SCAN_BUCKETS
                    ]) {
                ALOGE("%s: QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CAPABILITIES_MAX"
                    "_SCAN_BUCKETS not found", __func__);
                ret = WIFI_ERROR_INVALID_ARGS;
                break;
            }
            mGetCapabilitiesRspParams->capabilities.max_scan_buckets =
                nla_get_u32(tbVendor[
                QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CAPABILITIES_MAX_SCAN_BUCKETS]
                                );

            if (!tbVendor[
        QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CAPABILITIES_MAX_AP_CACHE_PER_SCAN
                    ]) {
                ALOGE("%s: QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CAPABILITIES_MAX"
                    "_AP_CACHE_PER_SCAN not found", __func__);
                ret = WIFI_ERROR_INVALID_ARGS;
                break;
            }
            mGetCapabilitiesRspParams->capabilities.max_ap_cache_per_scan =
                    nla_get_u32(tbVendor[
                QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CAPABILITIES_MAX_AP_CACHE_PER_SCAN]);

            if (!tbVendor[
            QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CAPABILITIES_MAX_RSSI_SAMPLE_SIZE
                    ]) {
                ALOGE("%s: QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CAPABILITIES_MAX"
                    "_RSSI_SAMPLE_SIZE not found", __func__);
                ret = WIFI_ERROR_INVALID_ARGS;
                break;
            }
            mGetCapabilitiesRspParams->capabilities.max_rssi_sample_size =
                    nla_get_u32(tbVendor[
                    QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CAPABILITIES_MAX_RSSI_SAMPLE_SIZE]);

            if (!tbVendor[
            QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CAPABILITIES_MAX_SCAN_REPORTING_THRESHOLD
                    ]) {
                ALOGE("%s: QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CAPABILITIES_"
                    "MAX_SCAN_REPORTING_THRESHOLD not found", __func__);
                ret = WIFI_ERROR_INVALID_ARGS;
                break;
            }
            mGetCapabilitiesRspParams->capabilities.max_scan_reporting_threshold =
                    nla_get_u32(tbVendor[
            QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CAPABILITIES_MAX_SCAN_REPORTING_THRESHOLD
            ]);

            if (!tbVendor[
            QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CAPABILITIES_MAX_HOTLIST_BSSIDS
                    ]) {
                ALOGE("%s: QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CAPABILITIES_"
                    "MAX_HOTLIST_BSSIDS not found", __func__);
                ret = WIFI_ERROR_INVALID_ARGS;
                break;
            }
            mGetCapabilitiesRspParams->capabilities.max_hotlist_bssids =
                    nla_get_u32(tbVendor[
                    QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CAPABILITIES_MAX_HOTLIST_BSSIDS]);

            if (!tbVendor[
            QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CAPABILITIES_MAX_SIGNIFICANT_WIFI_CHANGE_APS
                    ]) {
                ALOGE("%s: QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CAPABILITIES_MAX"
                    "_SIGNIFICANT_WIFI_CHANGE_APS not found", __func__);
                ret = WIFI_ERROR_INVALID_ARGS;
                break;
            }
            mGetCapabilitiesRspParams->capabilities.max_significant_wifi_change_aps =
                    nla_get_u32(tbVendor[
            QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CAPABILITIES_MAX_SIGNIFICANT_WIFI_CHANGE_APS]);

            if (!tbVendor[
            QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CAPABILITIES_MAX_BSSID_HISTORY_ENTRIES
                    ]) {
                ALOGE("%s: QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CAPABILITIES_MAX"
                    "_BSSID_HISTORY_ENTRIES not found", __func__);
                ret = WIFI_ERROR_INVALID_ARGS;
                break;
            }
            mGetCapabilitiesRspParams->capabilities.max_bssid_history_entries =
                    nla_get_u32(tbVendor[
            QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CAPABILITIES_MAX_BSSID_HISTORY_ENTRIES
            ]);

            if (!tbVendor[
            QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CAPABILITIES_MAX_HOTLIST_SSIDS
                    ]) {
                ALOGE("%s: QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CAPABILITIES"
                    "_MAX_HOTLIST_SSIDS not found. Set to 0.", __func__);
            } else {
                mGetCapabilitiesRspParams->capabilities.max_hotlist_ssids =
                        nla_get_u32(tbVendor[
                QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CAPABILITIES_MAX_HOTLIST_SSIDS
                ]);
            }

            if (!tbVendor[
            QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CAPABILITIES_MAX_NUM_EPNO_NETS
                    ]) {
                ALOGE("%s: QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CAPABILITIES_MAX"
                    "_NUM_EPNO_NETS not found. Set to 0.", __func__);
                mGetCapabilitiesRspParams->capabilities.\
                    max_number_epno_networks = 0;
            } else {
                mGetCapabilitiesRspParams->capabilities.max_number_epno_networks
                    = nla_get_u32(tbVendor[
                QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CAPABILITIES_MAX_NUM_EPNO_NETS
                ]);
            }

            if (!tbVendor[
            QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CAPABILITIES_MAX_NUM_EPNO_NETS_BY_SSID
                    ]) {
                ALOGE("%s: QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CAPABILITIES_MAX"
                    "_NUM_EPNO_NETS_BY_SSID not found. Set to 0.", __func__);
                mGetCapabilitiesRspParams->capabilities.\
                    max_number_epno_networks_by_ssid = 0;
            } else {
                mGetCapabilitiesRspParams->capabilities.max_number_epno_networks_by_ssid
                    = nla_get_u32(tbVendor[
                QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CAPABILITIES_MAX_NUM_EPNO_NETS_BY_SSID
                ]);
            }

            if (!tbVendor[
            QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CAPABILITIES_MAX_NUM_WHITELISTED_SSID
                    ]) {
                ALOGE("%s: QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CAPABILITIES_MAX"
                    "_NUM_WHITELISTED_SSID not found. Set to 0.", __func__);
                mGetCapabilitiesRspParams->capabilities.\
                    max_number_of_white_listed_ssid = 0;
            } else {
                mGetCapabilitiesRspParams->capabilities.max_number_of_white_listed_ssid
                    = nla_get_u32(tbVendor[
                QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_CAPABILITIES_MAX_NUM_WHITELISTED_SSID
                ]);
            }

            /* Call the call back handler func. */
            if (mHandler.get_capabilities)
                (*mHandler.get_capabilities)
                        (mGetCapabilitiesRspParams->status,
                        mGetCapabilitiesRspParams->capabilities);
            waitForRsp(false);
        }
        break;

        case QCA_NL80211_VENDOR_SUBCMD_GSCAN_GET_CACHED_RESULTS:
        {
            wifi_request_id id;
            u32 numResults = 0;
            u32 startingIndex;
            int firstScanIdInPatch = -1;

            if (!tbVendor[
                QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_REQUEST_ID]) {
                ALOGE("%s: GSCAN_RESULTS_REQUEST_ID not"
                    "found", __func__);
                break;
            }
            id = nla_get_u32(
                    tbVendor[QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_REQUEST_ID]
                    );
            ALOGE("%s: Event has Req. ID:%d, ours:%d",
                __func__, id, mRequestId);
            /* If this is not for us, just ignore it. */
            if (id != mRequestId) {
                ALOGE("%s: Event has Req. ID:%d <> ours:%d",
                    __func__, id, mRequestId);
                break;
            }
            if (!tbVendor[
                QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_NUM_RESULTS_AVAILABLE]) {
                ALOGE("%s: GSCAN_RESULTS_NUM_RESULTS_AVAILABLE not"
                    "found", __func__);
                break;
            }
            numResults = nla_get_u32(tbVendor[
                QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_NUM_RESULTS_AVAILABLE]);
            ALOGE("%s: number of results:%d", __func__,
                numResults);

            if (!mGetCachedResultsRspParams) {
                ALOGE("%s: mGetCachedResultsRspParams is NULL, exit.",
                    __func__);
                break;
            }

            if (!tbVendor[QCA_WLAN_VENDOR_ATTR_GSCAN_CACHED_RESULTS_SCAN_ID]) {
                ALOGE("GSCAN_CACHED_RESULTS_SCAN_ID not found");
                return WIFI_ERROR_INVALID_ARGS;
            }

            /* Get the first Scan-Id in this chuck of cached results. */
            firstScanIdInPatch = nla_get_u32(tbVendor[
                    QCA_WLAN_VENDOR_ATTR_GSCAN_CACHED_RESULTS_SCAN_ID]);

            if (firstScanIdInPatch ==
                mGetCachedResultsRspParams->lastProcessedScanId) {
                ALOGD("firstScanIdInPatch == lastProcessedScanId = %d",
                    firstScanIdInPatch);
            /* The first scan id in this new patch is the same as last scan
             * id in previous patch. Hence we should update the numResults so it only
             * reflects new unique scan ids.
             */
                numResults--;
            }

            mGetCachedResultsNumResults += numResults;

            /* To support fragmentation from firmware, monitor the
             * MORE_DATA flag and cache results until MORE_DATA = 0.
             */
            if (!tbVendor[
                QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_RESULT_MORE_DATA]) {
                ALOGE("%s: GSCAN_RESULTS_NUM_RESULTS_MORE_DATA "
                    "not found", __func__);
                ret = WIFI_ERROR_INVALID_ARGS;
                break;
            } else {
                mGetCachedResultsRspParams->more_data = nla_get_u8(
                    tbVendor[
                QCA_WLAN_VENDOR_ATTR_GSCAN_RESULTS_SCAN_RESULT_MORE_DATA]);
                ALOGE("%s: More data = %d. \n", __func__,
                                mGetCachedResultsRspParams->more_data);
            }

            mGetCachedResultsRspParams->num_cached_results =
                                        mGetCachedResultsNumResults;

            if (numResults || (firstScanIdInPatch ==
                mGetCachedResultsRspParams->lastProcessedScanId)) {
                ALOGD("%s: Extract cached results received.\n", __func__);
                if (firstScanIdInPatch !=
                    mGetCachedResultsRspParams->lastProcessedScanId) {
                    startingIndex =
                        mGetCachedResultsNumResults - numResults;
                } else {
                    startingIndex =
                        mGetCachedResultsNumResults - 1;
                }
                ALOGD("%s: starting_index:%d", __func__, startingIndex);
                ALOGD("lastProcessedScanId: %d, wifiScanResultsStartingIndex:%d. ",
                mGetCachedResultsRspParams->lastProcessedScanId,
                mGetCachedResultsRspParams->wifiScanResultsStartingIndex);
                ret = gscan_get_cached_results(numResults,
                                        mGetCachedResultsRspParams->cached_results,
                                        startingIndex,
                                        tbVendor);
                /* If a parsing error occurred, exit and proceed for cleanup. */
                if (ret)
                    break;
            }
            /* Send the results if no more result data fragments are expected. */
            if (mHandler.get_cached_results) {
                (*mHandler.get_cached_results)
                    (mGetCachedResultsRspParams->more_data,
                    mGetCachedResultsRspParams->num_cached_results);
            }
            waitForRsp(false);
        }
        break;

        default:
            /* Error case should not happen print log */
            ALOGE("%s: Wrong GScan subcmd received %d", __func__, mSubcmd);
    }

    /* A parsing error occurred, do the cleanup of gscan result lists. */
    if (ret) {
        switch(mSubcmd)
        {
            case QCA_NL80211_VENDOR_SUBCMD_GSCAN_GET_CACHED_RESULTS:
            {
                freeRspParams(eGScanGetCachedResultsRspParams);
            }
            break;

            case QCA_NL80211_VENDOR_SUBCMD_GSCAN_GET_CAPABILITIES:
            break;

            default:
                ALOGE("%s: Wrong GScan subcmd received %d", __func__, mSubcmd);
        }
    }

    return NL_SKIP;
}

int GScanCommand::setCallbackHandler(GScanCallbackHandler nHandler)
{
    int res = 0;
    mHandler = nHandler;
    res = registerVendorHandler(mVendor_id, mSubcmd);
    if (res != 0) {
        /* Error case: should not happen, so print a log when it does. */
        ALOGE("%s: Unable to register Vendor Handler Vendor Id=0x%x subcmd=%u",
              __func__, mVendor_id, mSubcmd);
    }
    return res;
}

int GScanCommand::allocCachedResultsTemp(int max,
                                     wifi_cached_scan_results *cached_results)
{
    wifi_cached_scan_results *tempCachedResults = NULL;

    /* Alloc memory for "max" number of cached results. */
    mGetCachedResultsRspParams->cached_results =
        (wifi_cached_scan_results*)
        malloc(max * sizeof(wifi_cached_scan_results));
    if (!mGetCachedResultsRspParams->cached_results) {
        ALOGE("%s: Failed to allocate memory for "
            "mGetCachedResultsRspParams->cached_results.", __func__);
        return WIFI_ERROR_OUT_OF_MEMORY;
    }
    memset(mGetCachedResultsRspParams->cached_results, 0,
           max * sizeof(wifi_cached_scan_results));

    mGetCachedResultsRspParams->max = max;

    return WIFI_SUCCESS;
}

/*
 * Allocates memory for the subCmd response struct and initializes status = -1
 */
int GScanCommand::allocRspParams(eGScanRspRarams cmd)
{
    int ret = 0;
    switch(cmd)
    {
        case eGScanStartRspParams:
            mStartGScanRspParams = (GScanStartRspParams *)
                malloc(sizeof(GScanStartRspParams));
            if (!mStartGScanRspParams)
                ret = -1;
            else
                mStartGScanRspParams->status = -1;
        break;
        case eGScanStopRspParams:
            mStopGScanRspParams = (GScanStopRspParams *)
                malloc(sizeof(GScanStopRspParams));
            if (!mStopGScanRspParams)
                ret = -1;
            else
                mStopGScanRspParams->status = -1;
        break;
        case eGScanSetBssidHotlistRspParams:
            mSetBssidHotlistRspParams = (GScanSetBssidHotlistRspParams *)
                malloc(sizeof(GScanSetBssidHotlistRspParams));
            if (!mSetBssidHotlistRspParams)
                ret = -1;
            else
                mSetBssidHotlistRspParams->status = -1;
        break;
        case eGScanResetBssidHotlistRspParams:
            mResetBssidHotlistRspParams = (GScanResetBssidHotlistRspParams *)
                malloc(sizeof(GScanResetBssidHotlistRspParams));
            if (!mResetBssidHotlistRspParams)
                ret = -1;
            else
                mResetBssidHotlistRspParams->status = -1;
        break;
        case eGScanSetSignificantChangeRspParams:
            mSetSignificantChangeRspParams =
                (GScanSetSignificantChangeRspParams *)
                malloc(sizeof(GScanSetSignificantChangeRspParams));
            if (!mSetSignificantChangeRspParams)
                ret = -1;
            else
                mSetSignificantChangeRspParams->status = -1;
        break;
        case eGScanResetSignificantChangeRspParams:
            mResetSignificantChangeRspParams =
                (GScanResetSignificantChangeRspParams *)
                malloc(sizeof(GScanResetSignificantChangeRspParams));
            if (!mResetSignificantChangeRspParams)
                ret = -1;
            else
                mResetSignificantChangeRspParams->status = -1;
        break;
        case eGScanGetCapabilitiesRspParams:
            mGetCapabilitiesRspParams = (GScanGetCapabilitiesRspParams *)
                malloc(sizeof(GScanGetCapabilitiesRspParams));
            if (!mGetCapabilitiesRspParams)
                ret = -1;
            else  {
                memset(&mGetCapabilitiesRspParams->capabilities, 0,
                    sizeof(wifi_gscan_capabilities));
                mGetCapabilitiesRspParams->status = -1;
            }
        break;
        case eGScanGetCachedResultsRspParams:
            mGetCachedResultsRspParams = (GScanGetCachedResultsRspParams *)
                malloc(sizeof(GScanGetCachedResultsRspParams));
            if (!mGetCachedResultsRspParams)
                ret = -1;
            else {
                mGetCachedResultsRspParams->num_cached_results = 0;
                mGetCachedResultsRspParams->more_data = false;
                mGetCachedResultsRspParams->lastProcessedScanId = -1;
                mGetCachedResultsRspParams->wifiScanResultsStartingIndex = -1;
                mGetCachedResultsRspParams->max = 0;
                mGetCachedResultsRspParams->cached_results = NULL;
            }
        break;
        case eGScanSetSsidHotlistRspParams:
            mSetSsidHotlistRspParams = (GScanSetSsidHotlistRspParams *)
                malloc(sizeof(GScanSetSsidHotlistRspParams));
            if (!mSetSsidHotlistRspParams)
                ret = -1;
            else
                mSetSsidHotlistRspParams->status = -1;
        break;
        case eGScanResetSsidHotlistRspParams:
            mResetSsidHotlistRspParams = (GScanResetSsidHotlistRspParams *)
                malloc(sizeof(GScanResetSsidHotlistRspParams));
            if (!mResetSsidHotlistRspParams)
                ret = -1;
            else
                mResetSsidHotlistRspParams->status = -1;
        break;
        default:
            ALOGD("%s: Wrong request for alloc.", __func__);
            ret = -1;
    }
    return ret;
}

void GScanCommand::freeRspParams(eGScanRspRarams cmd)
{
    u32 i = 0;
    wifi_cached_scan_results *cached_results = NULL;

    switch(cmd)
    {
        case eGScanStartRspParams:
            if (mStartGScanRspParams) {
                free(mStartGScanRspParams);
                mStartGScanRspParams = NULL;
            }
        break;
        case eGScanStopRspParams:
            if (mStopGScanRspParams) {
                free(mStopGScanRspParams);
                mStopGScanRspParams = NULL;
            }
        break;
        case eGScanSetBssidHotlistRspParams:
            if (mSetBssidHotlistRspParams) {
                free(mSetBssidHotlistRspParams);
                mSetBssidHotlistRspParams = NULL;
            }
        break;
        case eGScanResetBssidHotlistRspParams:
            if (mResetBssidHotlistRspParams) {
                free(mResetBssidHotlistRspParams);
                mResetBssidHotlistRspParams = NULL;
            }
        break;
        case eGScanSetSignificantChangeRspParams:
            if (mSetSignificantChangeRspParams) {
                free(mSetSignificantChangeRspParams);
                mSetSignificantChangeRspParams = NULL;
            }
        break;
        case eGScanResetSignificantChangeRspParams:
            if (mResetSignificantChangeRspParams) {
                free(mResetSignificantChangeRspParams);
                mResetSignificantChangeRspParams = NULL;
            }
        break;
        case eGScanGetCapabilitiesRspParams:
            if (mGetCapabilitiesRspParams) {
                free(mGetCapabilitiesRspParams);
                mGetCapabilitiesRspParams = NULL;
            }
        break;
        case eGScanGetCachedResultsRspParams:
            if (mGetCachedResultsRspParams) {
                if (mGetCachedResultsRspParams->cached_results) {
                    free(mGetCachedResultsRspParams->cached_results);
                    mGetCachedResultsRspParams->cached_results = NULL;
                }
                free(mGetCachedResultsRspParams);
                mGetCachedResultsRspParams = NULL;
            }
        break;
        case eGScanSetSsidHotlistRspParams:
            if (mSetSsidHotlistRspParams) {
                free(mSetSsidHotlistRspParams);
                mSetSsidHotlistRspParams = NULL;
            }
        break;
        case eGScanResetSsidHotlistRspParams:
            if (mResetSsidHotlistRspParams) {
                free(mResetSsidHotlistRspParams);
                mResetSsidHotlistRspParams = NULL;
            }
        break;
        default:
            ALOGD("%s: Wrong request for free.", __func__);
    }
}

wifi_error GScanCommand::getGetCachedResultsRspParams(u8 *moreData,
                                                      int *numResults)
{
    if (!mGetCachedResultsRspParams) {
        ALOGD("%s: mGetCachedResultsRspParams is NULL. Exit", __func__);
        return WIFI_ERROR_INVALID_ARGS;
    }
    /* Populate more data flag and number of parsed cached results. */
    *moreData = mGetCachedResultsRspParams->more_data;
    *numResults = mGetCachedResultsRspParams->num_cached_results;
    return WIFI_SUCCESS;
}

wifi_error GScanCommand::copyCachedScanResults(
                                      int numResults,
                                      wifi_cached_scan_results *cached_results)
{
    wifi_error ret = WIFI_SUCCESS;
    int i;
    wifi_cached_scan_results *cachedResultRsp;

    ALOGD("copyCachedScanResults: Enter");
    if (mGetCachedResultsRspParams && cached_results)
    {
        for (i = 0; i < numResults; i++) {
            cachedResultRsp = &mGetCachedResultsRspParams->cached_results[i];
            cached_results[i].scan_id = cachedResultRsp->scan_id;
            cached_results[i].flags = cachedResultRsp->flags;
            cached_results[i].num_results = cachedResultRsp->num_results;

            if (!cached_results[i].num_results) {
                ALOGD("Error: cached_results[i].num_results=0", i);
                continue;
            }

            ALOGD("copyCachedScanResults: "
                "cached_results[%d].num_results : %d",
                i, cached_results[i].num_results);

            memcpy(cached_results[i].results,
                cachedResultRsp->results,
                cached_results[i].num_results * sizeof(wifi_scan_result));
        }
    } else {
        ALOGD("%s: mGetCachedResultsRspParams is NULL", __func__);
        ret = WIFI_ERROR_INVALID_ARGS;
    }
    return ret;
}

void GScanCommand::getGetCapabilitiesRspParams(
                                        wifi_gscan_capabilities *capabilities,
                                        u32 *status)
{
    if (mGetCapabilitiesRspParams && capabilities)
    {
        *status = mGetCapabilitiesRspParams->status;
        memcpy(capabilities,
            &mGetCapabilitiesRspParams->capabilities,
            sizeof(wifi_gscan_capabilities));
    } else {
        ALOGD("%s: mGetCapabilitiesRspParams is NULL", __func__);
    }
}

void GScanCommand::getStartGScanRspParams(u32 *status)
{
    if (mStartGScanRspParams)
    {
        *status = mStartGScanRspParams->status;
    } else {
        ALOGD("%s: mStartGScanRspParams is NULL", __func__);
    }
}

void GScanCommand::getStopGScanRspParams(u32 *status)
{
    if (mStopGScanRspParams)
    {
        *status = mStopGScanRspParams->status;
    } else {
        ALOGD("%s: mStopGScanRspParams is NULL", __func__);
    }
}

void GScanCommand::getSetBssidHotlistRspParams(u32 *status)
{
    if (mSetBssidHotlistRspParams)
    {
        *status = mSetBssidHotlistRspParams->status;
    } else {
        ALOGD("%s: mSetBssidHotlistRspParams is NULL", __func__);
    }
}

void GScanCommand::getResetBssidHotlistRspParams(u32 *status)
{
    if (mResetBssidHotlistRspParams)
    {
        *status = mResetBssidHotlistRspParams->status;
    } else {
        ALOGD("%s: mResetBssidHotlistRspParams is NULL", __func__);
    }
}

void GScanCommand::getSetSignificantChangeRspParams(u32 *status)
{
    if (mSetSignificantChangeRspParams)
    {
        *status = mSetSignificantChangeRspParams->status;
    } else {
        ALOGD("%s: mSetSignificantChangeRspParams is NULL", __func__);
    }
}

void GScanCommand::getResetSignificantChangeRspParams(u32 *status)
{
    if (mResetSignificantChangeRspParams)
    {
        *status = mResetSignificantChangeRspParams->status;
    } else {
        ALOGD("%s: mResetSignificantChangeRspParams is NULL", __func__);
    }
}

void GScanCommand::getSetSsidHotlistRspParams(u32 *status)
{
    if (mSetSsidHotlistRspParams)
    {
        *status = mSetSsidHotlistRspParams->status;
    } else {
        ALOGD("%s: mSetSsidHotlistRspParams is NULL", __func__);
    }
}

void GScanCommand::getResetSsidHotlistRspParams(u32 *status)
{
    if (mResetSsidHotlistRspParams)
    {
        *status = mResetSsidHotlistRspParams->status;
    } else {
        ALOGD("%s: mResetSsidHotlistRspParams is NULL", __func__);
    }
}

int GScanCommand::timed_wait(u16 wait_time)
{
    struct timespec absTime;
    int res;
    absTime.tv_sec = wait_time;
    absTime.tv_nsec = 0;
    return mCondition.wait(absTime);
}

void GScanCommand::waitForRsp(bool wait)
{
    mWaitforRsp = wait;
}

void GScanCommand::setMaxChannels(int max_channels) {
    mMaxChannels = max_channels;
}

void GScanCommand::setChannels(int *channels) {
    mChannels = channels;
}

void GScanCommand::setNumChannelsPtr(int *num_channels) {
    mNumChannelsPtr = num_channels;
}

wifi_error wifi_set_ssid_white_list(wifi_request_id id,
                                    wifi_interface_handle iface,
                                    int num_networks,
                                    wifi_ssid *ssids)
{
    int ret = 0, i;
    GScanCommand *roamCommand;
    struct nlattr *nlData, *nlSsids;
    interface_info *ifaceInfo = getIfaceInfo(iface);
    wifi_handle wifiHandle = getWifiHandle(iface);
    hal_info *info = getHalInfo(wifiHandle);

    ALOGI("White list ssid : set");
    ALOGI("Number of SSIDs : %d", num_networks);
    for (i = 0; i < num_networks; i++) {
        ALOGI("ssid %d : %s", i, ssids[i].ssid);
    }

    if (!(info->supported_feature_set & WIFI_FEATURE_GSCAN)) {
        ALOGE("%s: GSCAN is not supported by driver",
            __func__);
        return WIFI_ERROR_NOT_SUPPORTED;
    }

    roamCommand = new GScanCommand(
                                wifiHandle,
                                id,
                                OUI_QCA,
                                QCA_NL80211_VENDOR_SUBCMD_ROAM);
    if (roamCommand == NULL) {
        ALOGE("wifi_set_ssid_white_list(): Error roamCommand NULL");
        return WIFI_ERROR_UNKNOWN;
    }

    /* Create the NL message. */
    ret = roamCommand->create();
    if (ret < 0)
        goto cleanup;

    /* Set the interface Id of the message. */
    ret = roamCommand->set_iface_id(ifaceInfo->name);
    if (ret < 0)
        goto cleanup;

    /* Add the vendor specific attributes for the NL command. */
    nlData = roamCommand->attr_start(NL80211_ATTR_VENDOR_DATA);
    if (!nlData)
        goto cleanup;

    if (roamCommand->put_u32(QCA_WLAN_VENDOR_ATTR_ROAMING_SUBCMD,
            QCA_WLAN_VENDOR_ATTR_ROAM_SUBCMD_SSID_WHITE_LIST) ||
        roamCommand->put_u32(
            QCA_WLAN_VENDOR_ATTR_ROAMING_REQ_ID,
            id) ||
        roamCommand->put_u32(
            QCA_WLAN_VENDOR_ATTR_ROAMING_PARAM_WHITE_LIST_SSID_NUM_NETWORKS,
            num_networks)) {
        goto cleanup;
    }

    nlSsids =
      roamCommand->attr_start(
            QCA_WLAN_VENDOR_ATTR_ROAMING_PARAM_WHITE_LIST_SSID_LIST);
    for (i = 0; i < num_networks; i++) {
        struct nlattr *nl_ssid = roamCommand->attr_start(i);

        if ( roamCommand->put_string(
                    QCA_WLAN_VENDOR_ATTR_ROAMING_PARAM_WHITE_LIST_SSID,
                    ssids[i].ssid)) {
            goto cleanup;
        }

        roamCommand->attr_end(nl_ssid);
    }
    roamCommand->attr_end(nlSsids);

    roamCommand->attr_end(nlData);

    ret = roamCommand->requestEvent();
    if (ret != 0) {
        ALOGE("wifi_set_ssid_white_list(): requestEvent Error:%d", ret);
    }

cleanup:
    delete roamCommand;
    return (wifi_error)ret;

}

wifi_error wifi_set_gscan_roam_params(wifi_request_id id,
                                      wifi_interface_handle iface,
                                      wifi_roam_params * params)
{
    int ret = 0;
    GScanCommand *roamCommand;
    struct nlattr *nlData;
    interface_info *ifaceInfo = getIfaceInfo(iface);
    wifi_handle wifiHandle = getWifiHandle(iface);
    hal_info *info = getHalInfo(wifiHandle);

    ALOGI("set gscan roam params:");
    if(params) {
        ALOGI("A_band_boost_threshold   %d", params->A_band_boost_threshold);
        ALOGI("A_band_penalty_threshol  %d", params->A_band_penalty_threshold);
        ALOGI("A_band_boost_factor      %u", params->A_band_boost_factor);
        ALOGI("A_band_penalty_factor    %u", params->A_band_penalty_factor);
        ALOGI("A_band_max_boost         %u", params->A_band_max_boost);
        ALOGI("lazy_roam_histeresys     %u", params->lazy_roam_hysteresis);
        ALOGI("alert_roam_rssi_trigger  %d", params->alert_roam_rssi_trigger);
    } else {
        ALOGE("wifi_roam_params is NULL");
        return WIFI_ERROR_INVALID_ARGS;
    }

    if (!(info->supported_feature_set & WIFI_FEATURE_GSCAN)) {
        ALOGE("%s: GSCAN is not supported by driver",
            __func__);
        return WIFI_ERROR_NOT_SUPPORTED;
    }

    roamCommand = new GScanCommand(wifiHandle,
                                   id,
                                   OUI_QCA,
                                   QCA_NL80211_VENDOR_SUBCMD_ROAM);
    if (roamCommand == NULL) {
        ALOGE("wifi_set_gscan_roam_params(): Error roamCommand NULL");
        return WIFI_ERROR_UNKNOWN;
    }

    /* Create the NL message. */
    ret = roamCommand->create();
    if (ret < 0)
        goto cleanup;

    /* Set the interface Id of the message. */
    ret = roamCommand->set_iface_id(ifaceInfo->name);
    if (ret < 0)
        goto cleanup;

    /* Add the vendor specific attributes for the NL command. */
    nlData = roamCommand->attr_start(NL80211_ATTR_VENDOR_DATA);
    if (!nlData)
        goto cleanup;

    if (roamCommand->put_u32(QCA_WLAN_VENDOR_ATTR_ROAMING_SUBCMD,
            QCA_WLAN_VENDOR_ATTR_ROAM_SUBCMD_SET_GSCAN_ROAM_PARAMS) ||
        roamCommand->put_u32(
            QCA_WLAN_VENDOR_ATTR_ROAMING_REQ_ID,
            id) ||
        roamCommand->put_s32(
            QCA_WLAN_VENDOR_ATTR_ROAMING_PARAM_A_BAND_BOOST_THRESHOLD,
            params->A_band_boost_threshold) ||
        roamCommand->put_s32(
            QCA_WLAN_VENDOR_ATTR_ROAMING_PARAM_A_BAND_PENALTY_THRESHOLD,
            params->A_band_penalty_threshold) ||
        roamCommand->put_u32(
            QCA_WLAN_VENDOR_ATTR_ROAMING_PARAM_A_BAND_BOOST_FACTOR,
            params->A_band_boost_factor) ||
        roamCommand->put_u32(
            QCA_WLAN_VENDOR_ATTR_ROAMING_PARAM_A_BAND_PENALTY_FACTOR,
            params->A_band_penalty_factor) ||
        roamCommand->put_u32(
            QCA_WLAN_VENDOR_ATTR_ROAMING_PARAM_A_BAND_MAX_BOOST,
            params->A_band_max_boost) ||
        roamCommand->put_u32(
            QCA_WLAN_VENDOR_ATTR_ROAMING_PARAM_LAZY_ROAM_HISTERESYS,
            params->lazy_roam_hysteresis) ||
        roamCommand->put_s32(
            QCA_WLAN_VENDOR_ATTR_ROAMING_PARAM_ALERT_ROAM_RSSI_TRIGGER,
            params->alert_roam_rssi_trigger)) {
        goto cleanup;
    }

    roamCommand->attr_end(nlData);

    ret = roamCommand->requestEvent();
    if (ret != 0) {
        ALOGE("wifi_set_gscan_roam_params(): requestEvent Error:%d", ret);
    }

cleanup:
    delete roamCommand;
    return (wifi_error)ret;

}

wifi_error wifi_enable_lazy_roam(wifi_request_id id,
                                 wifi_interface_handle iface,
                                 int enable)
{
    int ret = 0;
    GScanCommand *roamCommand;
    struct nlattr *nlData;
    interface_info *ifaceInfo = getIfaceInfo(iface);
    wifi_handle wifiHandle = getWifiHandle(iface);
    hal_info *info = getHalInfo(wifiHandle);

    ALOGI("set lazy roam: %s", enable?"ENABLE":"DISABLE");

    if (!(info->supported_feature_set & WIFI_FEATURE_GSCAN)) {
        ALOGE("%s: GSCAN is not supported by driver",
            __func__);
        return WIFI_ERROR_NOT_SUPPORTED;
    }

    roamCommand =
         new GScanCommand(wifiHandle,
                          id,
                          OUI_QCA,
                          QCA_NL80211_VENDOR_SUBCMD_ROAM);
    if (roamCommand == NULL) {
        ALOGE("%s: Error roamCommand NULL", __func__);
        return WIFI_ERROR_UNKNOWN;
    }

    /* Create the NL message. */
    ret = roamCommand->create();
    if (ret < 0)
        goto cleanup;

    /* Set the interface Id of the message. */
    ret = roamCommand->set_iface_id(ifaceInfo->name);
    if (ret < 0)
        goto cleanup;

    /* Add the vendor specific attributes for the NL command. */
    nlData = roamCommand->attr_start(NL80211_ATTR_VENDOR_DATA);
    if (!nlData)
        goto cleanup;

    if (roamCommand->put_u32(QCA_WLAN_VENDOR_ATTR_ROAMING_SUBCMD,
            QCA_WLAN_VENDOR_ATTR_ROAM_SUBCMD_SET_LAZY_ROAM) ||
        roamCommand->put_u32(
            QCA_WLAN_VENDOR_ATTR_ROAMING_REQ_ID,
            id) ||
        roamCommand->put_u32(
            QCA_WLAN_VENDOR_ATTR_ROAMING_PARAM_SET_LAZY_ROAM_ENABLE,
            enable)) {
        goto cleanup;
    }

    roamCommand->attr_end(nlData);

    ret = roamCommand->requestEvent();
    if (ret != 0) {
        ALOGE("%s: Error roamCommand NULL ret = %d", __func__, ret);
    }

cleanup:
    delete roamCommand;
    return (wifi_error)ret;

}

wifi_error wifi_set_bssid_preference(wifi_request_id id,
                                     wifi_interface_handle iface,
                                     int num_bssid,
                                     wifi_bssid_preference *prefs)
{
    int ret = 0, i;
    GScanCommand *roamCommand;
    struct nlattr *nlData, *nlBssids;
    interface_info *ifaceInfo = getIfaceInfo(iface);
    wifi_handle wifiHandle = getWifiHandle(iface);
    hal_info *info = getHalInfo(wifiHandle);

    ALOGI("Set BSSID preferences");
    ALOGI("Number of BSSIDs: %d", num_bssid);
    if(prefs && num_bssid) {
        for (i = 0; i < num_bssid; i++) {
            ALOGI("BSSID: %d : %02x:%02x:%02x:%02x:%02x:%02x", i,
                    prefs[i].bssid[0], prefs[i].bssid[1],
                    prefs[i].bssid[2], prefs[i].bssid[3],
                    prefs[i].bssid[4], prefs[i].bssid[5]);
            ALOGI("alert_roam_rssi_trigger : %d", prefs[i].rssi_modifier);
        }
    } else {
        ALOGE("wifi_bssid_preference is NULL");
        return WIFI_ERROR_INVALID_ARGS;
    }

    if (!(info->supported_feature_set & WIFI_FEATURE_GSCAN)) {
        ALOGE("%s: GSCAN is not supported by driver",
            __func__);
        return WIFI_ERROR_NOT_SUPPORTED;
    }

    roamCommand =
         new GScanCommand(wifiHandle,
                          id,
                          OUI_QCA,
                          QCA_NL80211_VENDOR_SUBCMD_ROAM);
    if (roamCommand == NULL) {
        ALOGE("%s: Error roamCommand NULL", __func__);
        return WIFI_ERROR_UNKNOWN;
    }

    /* Create the NL message. */
    ret = roamCommand->create();
    if (ret < 0)
        goto cleanup;

    /* Set the interface Id of the message. */
    ret = roamCommand->set_iface_id(ifaceInfo->name);
    if (ret < 0)
        goto cleanup;

    /* Add the vendor specific attributes for the NL command. */
    nlData = roamCommand->attr_start(NL80211_ATTR_VENDOR_DATA);
    if (!nlData)
        goto cleanup;

    if (roamCommand->put_u32(QCA_WLAN_VENDOR_ATTR_ROAMING_SUBCMD,
            QCA_WLAN_VENDOR_ATTR_ROAM_SUBCMD_SET_BSSID_PREFS) ||
        roamCommand->put_u32(
            QCA_WLAN_VENDOR_ATTR_ROAMING_REQ_ID,
            id) ||
        roamCommand->put_u32(
            QCA_WLAN_VENDOR_ATTR_ROAMING_PARAM_SET_LAZY_ROAM_NUM_BSSID,
            num_bssid)) {
        goto cleanup;
    }

    nlBssids = roamCommand->attr_start(
            QCA_WLAN_VENDOR_ATTR_ROAMING_PARAM_SET_BSSID_PREFS);
    for (i = 0; i < num_bssid; i++) {
        struct nlattr *nl_ssid = roamCommand->attr_start(i);

        if (roamCommand->put_addr(
                QCA_WLAN_VENDOR_ATTR_ROAMING_PARAM_SET_LAZY_ROAM_BSSID,
                (u8 *)prefs[i].bssid) ||
            roamCommand->put_s32(
                QCA_WLAN_VENDOR_ATTR_ROAMING_PARAM_SET_LAZY_ROAM_RSSI_MODIFIER,
                prefs[i].rssi_modifier)) {
            goto cleanup;
        }

        roamCommand->attr_end(nl_ssid);
    }
    roamCommand->attr_end(nlBssids);

    roamCommand->attr_end(nlData);

    ret = roamCommand->requestEvent();
    if (ret != 0) {
        ALOGE("%s: Error roamCommand NULL %d",__func__, ret);
    }

cleanup:
    delete roamCommand;
    return (wifi_error)ret;

}

wifi_error wifi_set_bssid_blacklist(wifi_request_id id,
                                    wifi_interface_handle iface,
                                    wifi_bssid_params params)
{
    int ret = 0, i;
    GScanCommand *roamCommand;
    struct nlattr *nlData, *nlBssids;
    interface_info *ifaceInfo = getIfaceInfo(iface);
    wifi_handle wifiHandle = getWifiHandle(iface);
    hal_info *info = getHalInfo(wifiHandle);

    ALOGI("Set BSSID  blacks list Params");
    for (i = 0; i < params.num_bssid; i++) {
        ALOGI("BSSID: %d : %02x:%02x:%02x:%02x:%02x:%02x", i,
                params.bssids[i][0], params.bssids[i][1],
                params.bssids[i][2], params.bssids[i][3],
                params.bssids[i][4], params.bssids[i][5]);
    }

    if (!(info->supported_feature_set & WIFI_FEATURE_GSCAN)) {
        ALOGE("%s: GSCAN is not supported by driver",
            __func__);
        return WIFI_ERROR_NOT_SUPPORTED;
    }

    roamCommand =
         new GScanCommand(wifiHandle,
                          id,
                          OUI_QCA,
                          QCA_NL80211_VENDOR_SUBCMD_ROAM);
    if (roamCommand == NULL) {
        ALOGE("%s: Error roamCommand NULL", __func__);
        return WIFI_ERROR_UNKNOWN;
    }

    /* Create the NL message. */
    ret = roamCommand->create();
    if (ret < 0)
        goto cleanup;

    /* Set the interface Id of the message. */
    ret = roamCommand->set_iface_id(ifaceInfo->name);
    if (ret < 0)
        goto cleanup;

    /* Add the vendor specific attributes for the NL command. */
    nlData = roamCommand->attr_start(NL80211_ATTR_VENDOR_DATA);
    if (!nlData)
        goto cleanup;

    if (roamCommand->put_u32(QCA_WLAN_VENDOR_ATTR_ROAMING_SUBCMD,
            QCA_WLAN_VENDOR_ATTR_ROAM_SUBCMD_SET_BLACKLIST_BSSID) ||
        roamCommand->put_u32(
            QCA_WLAN_VENDOR_ATTR_ROAMING_REQ_ID,
            id) ||
        roamCommand->put_u32(
            QCA_WLAN_VENDOR_ATTR_ROAMING_PARAM_SET_BSSID_PARAMS_NUM_BSSID,
            params.num_bssid)) {
        goto cleanup;
    }

    nlBssids = roamCommand->attr_start(
            QCA_WLAN_VENDOR_ATTR_ROAMING_PARAM_SET_BSSID_PARAMS);
    for (i = 0; i < params.num_bssid; i++) {
        struct nlattr *nl_ssid = roamCommand->attr_start(i);

        if (roamCommand->put_addr(
                QCA_WLAN_VENDOR_ATTR_ROAMING_PARAM_SET_BSSID_PARAMS_BSSID,
                (u8 *)params.bssids[i])) {
            goto cleanup;
        }

        roamCommand->attr_end(nl_ssid);
    }
    roamCommand->attr_end(nlBssids);

    roamCommand->attr_end(nlData);

    ret = roamCommand->requestEvent();
    if (ret != 0) {
        ALOGE("%s: Error roamCommand NULL %d",__func__, ret);
    }

cleanup:
    delete roamCommand;
    return (wifi_error)ret;

}
