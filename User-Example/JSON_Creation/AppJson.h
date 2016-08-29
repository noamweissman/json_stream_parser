#ifndef __JSON_APP_H__
#define __JSON_APP_H__

#ifdef __cplusplus
 extern "C" {
#endif

   
#ifdef PC_DEFINED
  #include "typedefs.h"
#else
  #include "include_types.h"
#endif


#define JS_STATUS_GROUP         0x00000001
#define JS_NETWORK_GROUP        0x00000002
#define JS_CONTROL_GROUP        0x00000004
#define JS_IO_GROUP             0x00000008
#define JS_IN_LABLES_GROUP      0x00000010

#define JS_CFG_ALL_GROUPS       (JS_STATUS_GROUP  |  JS_NETWORK_GROUP | JS_CONTROL_GROUP    |  \
                                 JS_IO_GROUP      | JS_IN_LABLES_GROUP)

                              
#define JS_PROCCESS_CONFIG      0
#define JS_START_CONFIG         0x01
#define JS_END_CONFIG           0x02

#define JS_MAX_GROUPS           32 // we are using a 32 bit map so we can have up to 32 configuration groups
                              
                              
//----------------------------------------------------------------
                  
// JSON single command group sub tags
#define JS_UNIQUE_ID_TAG        "id"

// JSON group tags
#define JS_STATUS_TAG           "status"
#define JS_NETWORK_TAG          "network"
#define JS_CONTROL_TAG          "control"
#define JS_IO_TAG               "io"
#define JS_IN_LABLES_TAG        "in_labels"

// JSON status group sub tags
#define JS_MODEL_TAG            "model_version"
#define JS_HW_VERSION_TAG       "hwr_version"
#define JS_MAC_ADDR_TAG         "mac_address"
#define JS_SN_TAG               "serial_number"
#define JS_TEMP_TAG             "temperature"
#define JS_OP_TIME_TAG          "operating_time"

// JSON network group sub tags
#define JS_DHCP_TAG             "dhcp"
#define JS_IP_ADDR_TAG          "ipaddress"
#define JS_MASK_ADDR_TAG        "subnet"
#define JS_GW_ADDR_TAG          "gateway"
#define JS_TCP_PORT_TAG         "telnet_port"
#define JS_HTTP_PORT_TAG        "http_port"
#define JS_TCP_TIMEOUT_TAG      "timeout"

// JSON control group sub tags
#define JS_POWER_TAG            "power"
#define JS_IR_TAG               "ir"
#define JS_KEYLOCK_TAG          "keylock"
#define JS_BAUD_RATE_TAG        "baud_rate"

// JSON io_route group sub tags
#define JS_ROUTE_TAG            "route"

// JSON io_lables group sub tags
#define JS_IN_X_TAG             "in"

// limiting values used in code
#define JS_LAST_PART            0xFF
#define JS_HDBT_PARTIAL         4
#define JS_CONTROL_PARTIAL      8

  
typedef struct
{
  int UniqueID;
  u8 NumOfGroupsToProccess;
  u8 ProccessedGroupCount;
  u8 CurrentPart;
  u8 NexPart;
  u8 Idx;
  
} JsonUserData_t;


typedef union
{
  u32 Dword;
  u8 Bytes[4];

} DwOrBytes_t;

//----------------------------------------------------------------


extern int GetDeviceConfigData(char *Data, int MaxDataLen, u32 Groups, JsonUserData_t *UserData);


#ifdef __cplusplus
}
#endif

#endif // __JSON_APP_H__
