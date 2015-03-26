
#include "wifi_hal.h"

/* API to request RTT measurement */
wifi_error wifi_rtt_range_request(wifi_request_id id, wifi_interface_handle iface,
        unsigned num_rtt_config, wifi_rtt_config rtt_config[], wifi_rtt_event_handler handler) {
    return WIFI_ERROR_NOT_SUPPORTED;
}

/* API to cancel RTT measurements */
wifi_error wifi_rtt_range_cancel(wifi_request_id id,  wifi_interface_handle iface,
        unsigned num_devices, mac_addr addr[]) {
    return WIFI_ERROR_NOT_SUPPORTED;
}

/* API to get RTT capability */
wifi_error wifi_get_rtt_capabilities(wifi_interface_handle iface,
        wifi_rtt_capabilities *capabilities)
{
    return WIFI_ERROR_NOT_SUPPORTED;
}
