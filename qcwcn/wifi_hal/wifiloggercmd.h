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

#ifndef __WIFI_HAL_WIFILOGGER_COMMAND_H__
#define __WIFH_HAL_WIFILOGGER_COMMAND_H__

#include "common.h"
#include "cpp_bindings.h"
#include "qca-vendor.h"
#include "wifi_logger.h"
#include "vendor_definitions.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */


typedef struct {
  void (*on_ring_buffer_data) (wifi_request_id id,
                               wifi_ring_buffer_id ring_id,
                               char *buffer, int buffer_size,
                               wifi_ring_buffer_status *status);
} WifiLoggerCallbackHandler;


class WifiLoggerCommand : public WifiVendorCommand
{
private:
    static WifiLoggerCommand *mWifiLoggerCommandInstance;
    WifiLoggerCallbackHandler mHandler;
    char                      **mVersion;
    int                       *mVersionLen;
    bool                      mWaitforRsp;
public:
    u8      *mTailMemoryDumBuffer;
    u8      *mMemoryDumBuffer;
    u8      mNumMemoryDumBufferRecv;
    u32     mMemoryDumBufferLen;
    bool    mMoreData;

    WifiLoggerCommand(wifi_handle handle, int id, u32 vendor_id, u32 subcmd);

    static WifiLoggerCommand* instance(wifi_handle handle);
    virtual ~WifiLoggerCommand();

    // This function implements creation of WifiLogger specific Request
    // based on  the request type
    virtual int create();
    virtual int requestEvent();
    virtual int requestResponse();
    virtual int handleResponse(WifiEvent &reply);
    virtual int handleEvent(WifiEvent &event);
    int setCallbackHandler(WifiLoggerCallbackHandler nHandler);
    virtual void unregisterHandler(u32 subCmd);

    /* Takes wait time in seconds. */
    virtual int timed_wait(u16 wait_time);
    virtual void waitForRsp(bool wait);
    virtual void setVersionInfo(char **buffer, int *buffer_size);
};
#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* __WIFH_HAL_WIFILOGGER_COMMAND_H__ */
