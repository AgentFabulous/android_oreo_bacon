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

#include "sync.h"

#include "wifi_hal.h"
#include "common.h"
#include "cpp_bindings.h"
#include <utils/Log.h>
#include "wifiloggercmd.h"

#define WIFI_MEMORY_DUMP_WAIT_TIME_SECONDS    4
#define WIFI_MEMORY_DUMP_MAX_SIZE             300000

//Singleton Static Instance
WifiLoggerCommand* WifiLoggerCommand::mWifiLoggerCommandInstance  = NULL;


//Implementation of the functions exposed in wifi_logger.h

/* Function to intiate logging */
wifi_error wifi_start_logging(wifi_interface_handle iface,
                              u32 verbose_level, u32 flags,
                              u32 max_interval_sec, u32 min_data_size,
                              u8 *buffer_name,
                              wifi_ring_buffer_data_handler handler)
{
    int requestId, ret = 0;
    WifiLoggerCommand *wifiLoggerCommand;
    struct nlattr *nlData;
    interface_info *ifaceInfo = getIfaceInfo(iface);
    wifi_handle wifiHandle = getWifiHandle(iface);
    hal_info *info = getHalInfo(wifiHandle);

    /*
     * No request id from caller, so generate one and pass it on to the driver.
     * Generate one randomly.
     */
    srand( time(NULL) );
    requestId = rand();

    /*
     * TBD Include support to send VENDOR_CMD for PKT LOG.
     * Create a Ring Buffer
     */
    wifiLoggerCommand = new WifiLoggerCommand(
                            wifiHandle,
                            requestId,
                            OUI_QCA,
                            QCA_NL80211_VENDOR_SUBCMD_WIFI_LOGGER_START);
    ALOGI("%s: Sending Start Logging Request. \n", __FUNCTION__);

    if (wifiLoggerCommand == NULL) {
        ALOGE("%s: Error WifiLoggerCommand NULL", __func__);
        return WIFI_ERROR_UNKNOWN;
    }
    /* Create the NL message. */
    ret = wifiLoggerCommand->create();

    if (ret < 0)
        goto cleanup;

    /* Set the interface Id of the message. */
    ret = wifiLoggerCommand->set_iface_id(ifaceInfo->name);

    if (ret < 0)
        goto cleanup;

    /* Add the vendor specific attributes for the NL command. */
    nlData = wifiLoggerCommand->attr_start(NL80211_ATTR_VENDOR_DATA);

    if (!nlData)
        goto cleanup;

    wifiLoggerCommand->attr_end(nlData);

    /* Send the msg and wait for a response. */
    ret = wifiLoggerCommand->requestResponse();
    if (ret) {
        ALOGE("%s: Error %d happened. ", __func__, ret);
    }

cleanup:
    ALOGI("%s: Delete object.", __func__);
    delete wifiLoggerCommand;
    return (wifi_error)ret;

}

/*  Function to get each ring related info */
wifi_error wifi_get_ring_buffers_status(wifi_request_id id,
                                        wifi_interface_handle iface,
                                        u32 *num_buffers,
                                        wifi_ring_buffer_status **status)
{
    int requestId, ret = 0;
    WifiLoggerCommand *wifiLoggerCommand;
    struct nlattr *nlData;
    interface_info *ifaceInfo = getIfaceInfo(iface);
    wifi_handle wifiHandle = getWifiHandle(iface);
    hal_info *info = getHalInfo(wifiHandle);

    /* No request id from caller, so generate one and pass it on to the driver.
     * Generate one randomly.
     */
    srand( time(NULL) );
    requestId = rand();
    /*
     * TBD need to implement the total rings supported
     * along with the ring buffer info.
     */
cleanup:
    return (wifi_error)ret;
}

/*  Function to get the supported feature set for logging.*/
wifi_error wifi_get_logger_supported_feature_set(wifi_request_id id,
                                                 wifi_interface_handle iface,
                                                 u32 *support)
{

    int requestId, ret = 0;
    WifiLoggerCommand *wifiLoggerCommand;
    struct nlattr *nlData;
    interface_info *ifaceInfo = getIfaceInfo(iface);
    wifi_handle wifiHandle = getWifiHandle(iface);
    hal_info *info = getHalInfo(wifiHandle);

    /* No request id from caller, so generate one and pass it on to the driver.
     * Generate one randomly.
     */
    srand( time(NULL) );
    requestId = rand();
    /*
     * TBD need to implement feature set supported in wifi_hal
     * for the logger app.
     */
cleanup:
    return (wifi_error)ret;
}

/*  Function to get the data in each ring for the given ring ID.*/
wifi_error wifi_get_ring_data(wifi_request_id id,
                              wifi_interface_handle iface,
                              wifi_ring_buffer_id ring_id)
{

    int requestId, ret = 0;
    WifiLoggerCommand *wifiLoggerCommand;
    struct nlattr *nlData;
    interface_info *ifaceInfo = getIfaceInfo(iface);
    wifi_handle wifiHandle = getWifiHandle(iface);
    hal_info *info = getHalInfo(wifiHandle);

    /* No request id from caller, so generate one and pass it on to the driver.
     * Generate one randomly.
     */
    srand( time(NULL) );
    requestId = rand();
    /*
     * TBD need to implement get ring data to invoke the CB handler
     * for the given ring_id.
     */

cleanup:
    return (wifi_error)ret;
}

void WifiLoggerCommand::setVersionInfo(char **buffer, int *buffer_size) {
    mVersion = buffer;
    mVersionLen = buffer_size;
}

/*  Function to send enable request to the wifi driver.*/
wifi_error wifi_get_firmware_version(wifi_request_id id,
                              wifi_interface_handle iface,
                              char **buffer, int *buffer_size)
{
    int requestId, ret = 0;
    WifiLoggerCommand *wifiLoggerCommand;
    struct nlattr *nlData;
    interface_info *ifaceInfo = getIfaceInfo(iface);
    wifi_handle wifiHandle = getWifiHandle(iface);
    hal_info *info = getHalInfo(wifiHandle);

    /* No request id from caller, so generate one and pass it on to the driver.
     * Generate one randomly.
     */
    srand( time(NULL) );
    requestId = rand();

    wifiLoggerCommand = new WifiLoggerCommand(
                                wifiHandle,
                                requestId,
                                OUI_QCA,
                                QCA_NL80211_VENDOR_SUBCMD_GET_WIFI_INFO);
    ALOGI("%s: Sending Get Wifi Info Request. \n", __FUNCTION__);
    if (wifiLoggerCommand == NULL) {
        ALOGE("%s: Error WifiLoggerCommand NULL", __func__);
        return WIFI_ERROR_UNKNOWN;
    }
    /* Create the NL message. */
    ret = wifiLoggerCommand->create();

    if (ret < 0)
        goto cleanup;

    /* Set the interface Id of the message. */
    ret = wifiLoggerCommand->set_iface_id(ifaceInfo->name);

    if (ret < 0)
        goto cleanup;

    /* Add the vendor specific attributes for the NL command. */
    nlData = wifiLoggerCommand->attr_start(NL80211_ATTR_VENDOR_DATA);

    if (!nlData)
        goto cleanup;

    if (wifiLoggerCommand->put_u32(
                QCA_WLAN_VENDOR_ATTR_WIFI_INFO_FIRMWARE_VERSION, requestId) )
    {
        goto cleanup;
    }
    wifiLoggerCommand->attr_end(nlData);

    wifiLoggerCommand->setVersionInfo(buffer, buffer_size);

    /* Send the msg and wait for a response. */
    ret = wifiLoggerCommand->requestResponse();
    if (ret) {
        ALOGE("%s: Error %d happened. ", __func__, ret);
    }
cleanup:
    ALOGI("%s: Delete object.", __func__);
    delete wifiLoggerCommand;
    return (wifi_error)ret;

}

/*  Function to get wlan driver version.*/
wifi_error wifi_get_driver_version(wifi_request_id id,
                                   wifi_interface_handle iface,
                                   char **buffer, int *buffer_size)
{

    int requestId, ret = 0;
    WifiLoggerCommand *wifiLoggerCommand;
    struct nlattr *nlData;
    interface_info *ifaceInfo = getIfaceInfo(iface);
    wifi_handle wifiHandle = getWifiHandle(iface);
    hal_info *info = getHalInfo(wifiHandle);

    /* No request id from caller, so generate one and pass it on to the driver.
     * Generate one randomly.
     */
    srand( time(NULL) );
    requestId = rand();

    wifiLoggerCommand = new WifiLoggerCommand(
                            wifiHandle,
                            requestId,
                            OUI_QCA,
                            QCA_NL80211_VENDOR_SUBCMD_GET_WIFI_INFO);
    ALOGI("%s: Sending Get Wifi Info Request. \n", __FUNCTION__);
    if (wifiLoggerCommand == NULL) {
        ALOGE("%s: Error WifiLoggerCommand NULL", __func__);
        return WIFI_ERROR_UNKNOWN;
    }
    /* Create the NL message. */
    ret = wifiLoggerCommand->create();

    if (ret < 0)
        goto cleanup;

    /* Set the interface Id of the message. */
    ret = wifiLoggerCommand->set_iface_id(ifaceInfo->name);

    if (ret < 0)
        goto cleanup;

    /* Add the vendor specific attributes for the NL command. */
    nlData = wifiLoggerCommand->attr_start(NL80211_ATTR_VENDOR_DATA);

    if (!nlData)
        goto cleanup;

    if (wifiLoggerCommand->put_u32(
            QCA_WLAN_VENDOR_ATTR_WIFI_INFO_DRIVER_VERSION, requestId))
    {
        goto cleanup;
    }
    wifiLoggerCommand->attr_end(nlData);

    wifiLoggerCommand->setVersionInfo(buffer, buffer_size);

    /* Send the msg and wait for a response. */
    ret = wifiLoggerCommand->requestResponse();
    if (ret) {
        ALOGE("%s: Error %d happened. ", __func__, ret);
    }
cleanup:
    ALOGI("%s: Delete object.", __func__);
    delete wifiLoggerCommand;
    return (wifi_error)ret;
}


/* Function to get the Firmware memory dump. */
wifi_error wifi_get_firmware_memory_dump(wifi_request_id id,
                                wifi_interface_handle iface,
                                wifi_firmware_memory_dump_handler handler)
{
    int requestId, ret = 0;
    WifiLoggerCommand *wifiLoggerCommand;
    struct nlattr *nlData;
    interface_info *ifaceInfo = getIfaceInfo(iface);
    wifi_handle wifiHandle = getWifiHandle(iface);
    hal_info *info = getHalInfo(wifiHandle);

    /* No request id from caller, so generate one and pass it on to the driver.
     * Generate one randomly.
     */
    srand( time(NULL) );
    requestId = rand();

    wifiLoggerCommand = new WifiLoggerCommand(
                            wifiHandle,
                            requestId,
                            OUI_QCA,
                            QCA_NL80211_VENDOR_SUBCMD_WIFI_LOGGER_MEMORY_DUMP);
    ALOGI("%s: Sending Memory Dump Request. \n", __FUNCTION__);
    if (wifiLoggerCommand == NULL) {
        ALOGE("%s: Error WifiLoggerCommand NULL", __func__);
        return WIFI_ERROR_UNKNOWN;
    }
    /* Create the NL message. */
    ret = wifiLoggerCommand->create();

    if (ret < 0)
        goto cleanup;

    /* Set the interface Id of the message. */
    ret = wifiLoggerCommand->set_iface_id(ifaceInfo->name);

    if (ret < 0)
        goto cleanup;

    /* Add the vendor specific attributes for the NL command. */
    nlData = wifiLoggerCommand->attr_start(NL80211_ATTR_VENDOR_DATA);

    if (!nlData)
        goto cleanup;

    wifiLoggerCommand->attr_end(nlData);

    /* Send the msg and wait for a response. */
    ret = wifiLoggerCommand->requestResponse();
    if (ret) {
        ALOGE("%s: Error %d happened. ", __func__, ret);
    }

    wifiLoggerCommand->mMemoryDumBuffer =(u8*)malloc(WIFI_MEMORY_DUMP_MAX_SIZE);

    if (wifiLoggerCommand->mMemoryDumBuffer == NULL) {
        ALOGE("%s: Failed to allocate memory", __func__);
        goto cleanup;
    }

    /* Send the msg and wait for a response. */
    ret = wifiLoggerCommand->requestResponse();
    if (ret) {
        ALOGE("%s: Error %d happened. ", __func__, ret);
        goto cleanup;
    }

cleanup:
    if (wifiLoggerCommand->mMemoryDumBuffer != NULL) {
        free(wifiLoggerCommand->mMemoryDumBuffer);
        wifiLoggerCommand->mMemoryDumBuffer = NULL;
    }
    ALOGI("%s: Delete object.", __func__);
    delete wifiLoggerCommand;
    return (wifi_error)ret;
}


WifiLoggerCommand::WifiLoggerCommand(wifi_handle handle, int id, u32 vendor_id, u32 subcmd)
        : WifiVendorCommand(handle, id, vendor_id, subcmd)
{
    ALOGV("WifiLoggerCommand %p constructed", this);
    mVersion = NULL;
    mVersionLen = NULL;
}

WifiLoggerCommand::~WifiLoggerCommand()
{
    ALOGD("WifiLoggerCommand %p destructor", this);
    unregisterVendorHandler(mVendor_id, mSubcmd);
}

WifiLoggerCommand* WifiLoggerCommand::instance(wifi_handle handle)
{
    if (handle == NULL) {
        ALOGE("Interface Handle is invalid");
        return NULL;
    }
    if (mWifiLoggerCommandInstance == NULL) {
        mWifiLoggerCommandInstance = new WifiLoggerCommand(handle, 0,
                OUI_QCA,
                QCA_NL80211_VENDOR_SUBCMD_WIFI_LOGGER_START);
        ALOGV("WifiLoggerCommand %p created", mWifiLoggerCommandInstance);
        return mWifiLoggerCommandInstance;
    }
    else
    {
        if (handle != getWifiHandle(mWifiLoggerCommandInstance->mInfo))
        {
            ALOGE("Handle different");
            return NULL;
        }
    }
    ALOGV("WifiLoggerCommand %p created already", mWifiLoggerCommandInstance);
    return mWifiLoggerCommandInstance;
}


/* This function implements creation of Vendor command */
int WifiLoggerCommand::create() {
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
static int error_handler_wifi_logger(struct sockaddr_nl *nla, struct nlmsgerr *err,
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
static int ack_handler_wifi_logger(struct nl_msg *msg, void *arg)
{
    int *ret = (int *)arg;
    struct nl_msg * a;

    ALOGE("%s: called", __func__);
    a = msg;
    *ret = 0;
    return NL_STOP;
}

/* Callback handlers registered for nl message send */
static int finish_handler_wifi_logger(struct nl_msg *msg, void *arg)
{
  int *ret = (int *)arg;
  struct nl_msg * a;

  ALOGE("%s: called", __func__);
  a = msg;
  *ret = 0;
  return NL_SKIP;
}

int WifiLoggerCommand::requestEvent()
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

    nl_cb_err(cb, NL_CB_CUSTOM, error_handler_wifi_logger, &res);
    nl_cb_set(cb, NL_CB_FINISH, NL_CB_CUSTOM, finish_handler_wifi_logger, &res);
    nl_cb_set(cb, NL_CB_ACK, NL_CB_CUSTOM, ack_handler_wifi_logger, &res);

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

int WifiLoggerCommand::requestResponse()
{
    ALOGD("%s: request a response", __func__);

    return WifiCommand::requestResponse(mMsg);
}

int WifiLoggerCommand::handleResponse(WifiEvent &reply) {
    ALOGD("Received a WifiLogger response message from Driver");
    u32 status;
    int ret = WIFI_SUCCESS;
    int i = 0;
    u32 len = 0, version;
    char version_type[20];
    WifiVendorCommand::handleResponse(reply);

    switch(mSubcmd)
    {
        case QCA_NL80211_VENDOR_SUBCMD_GET_WIFI_INFO:
        {
            struct nlattr *tb_vendor[QCA_WLAN_VENDOR_ATTR_WIFI_INFO_GET_MAX + 1];

            nla_parse(tb_vendor, QCA_WLAN_VENDOR_ATTR_WIFI_INFO_GET_MAX,
                            (struct nlattr *)mVendorData, mDataLen, NULL);

            if (tb_vendor[QCA_WLAN_VENDOR_ATTR_WIFI_INFO_DRIVER_VERSION]) {
                len = nla_len(tb_vendor[
                        QCA_WLAN_VENDOR_ATTR_WIFI_INFO_DRIVER_VERSION]);
                memcpy(version_type, "Driver", strlen("Driver"));
                version = QCA_WLAN_VENDOR_ATTR_WIFI_INFO_DRIVER_VERSION;
            } else if (
                tb_vendor[QCA_WLAN_VENDOR_ATTR_WIFI_INFO_FIRMWARE_VERSION]) {
                len = nla_len(
                        tb_vendor[
                        QCA_WLAN_VENDOR_ATTR_WIFI_INFO_FIRMWARE_VERSION]);
                memcpy(version_type, "Firmware", strlen("Firmware"));
                version = QCA_WLAN_VENDOR_ATTR_WIFI_INFO_FIRMWARE_VERSION;
            }
            if (len) {
                *mVersion = (char *)malloc(len*(sizeof(char)) + 1);
                if (!(*mVersion)) {
                    ALOGE("%s: Failed to allocate memory for Version.",
                    __func__);
                    return WIFI_ERROR_OUT_OF_MEMORY;
                }
                memset(*mVersion, 0, (len*(sizeof(char))) + 1);
                *mVersionLen = len;
                memcpy(*mVersion, nla_data(tb_vendor[version]), len);
                ALOGD("%s: WLAN version len : %d", __func__, len);
                ALOGD("%s: WLAN %s version : %s ", __func__,
                      version_type, *mVersion);
            }
        }
        break;

        case QCA_NL80211_VENDOR_SUBCMD_WIFI_LOGGER_MEMORY_DUMP:
        {
            /*
             * TBD malloc buffer.
             * Device open, initiate a read from proc
             * interface.
             * Attributes need to determine the size
             */
        }
        break;
        default :
            ALOGE("%s: Wrong Wifi Logger subcmd response received %d",
                __func__, mSubcmd);
    }

    return NL_SKIP;
}

/* This function will be the main handler for incoming (from driver)
 * WIFI_LOGGER_SUBCMD.
 * Calls the appropriate callback handler after parsing the vendor data.
 */
int WifiLoggerCommand::handleEvent(WifiEvent &event)
{
    ALOGI("Got a WifiLogger Event message from the Driver.");
    unsigned i = 0;
    u32 status;
    int ret = WIFI_SUCCESS;
    WifiVendorCommand::handleEvent(event);
    /* TBD Handle Event */
    return NL_SKIP;
}

int WifiLoggerCommand::setCallbackHandler(WifiLoggerCallbackHandler nHandler)
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

void WifiLoggerCommand::unregisterHandler(u32 subCmd)
{
    unregisterVendorHandler(mVendor_id, subCmd);
}

int WifiLoggerCommand::timed_wait(u16 wait_time)
{
    struct timespec absTime;
    int res;
    absTime.tv_sec = wait_time;
    absTime.tv_nsec = 0;
    return mCondition.wait(absTime);
}

void WifiLoggerCommand::waitForRsp(bool wait)
{
    mWaitforRsp = wait;
}

