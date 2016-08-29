
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>


#include "AppJson.h"


//----------------------------------------------------------------------------

static int GetDeviceGroupData(char *Data, int MaxDataLen, u32 JS_DataTypeGroups, u8 StartEnd, JsonUserData_t *UserData);

static int GetStatusGroupData(char *Data, int MaxDataLen, JsonUserData_t *UserData);
static int GetNetworkGroupData(char *Data, int MaxDataLen, JsonUserData_t *UserData);
static int GetControlGroupData(char *Data, int MaxDataLen, JsonUserData_t *UserData);
static int GetIoGroupData(char *Data, int MaxDataLen, JsonUserData_t *UserData);
static int GetInLablesGroupData(char *Data, int MaxDataLen, JsonUserData_t *UserData);

//
// dummy functions just to show it works. In a real application the following
// function will be defined somewere else and called here in
//
#define MAC_ADDR_SIZE 6

static u8 Route[] = {1,2,3,4,5,6,7,8};
static char *InputName[] =
{
  "Input_1","Input_2","Input_3","Input_4","Input_5","Input_6","Input_7","Input_8"
};

#define PARAM_OFF  "Off"
#define PARAM_ON   "On"


static char *GetVersionString(void) { return "1.0.0"; }
static char *GetHardwareVersion(void) { return "1.0.0"; }
static void GetMACadd(u8 *Addr) { Addr[0]=0xAA; Addr[1]=0xBB; Addr[2]=0xEE; Addr[3]=0x11; Addr[4]=0x22; Addr[5]=0x33; }
static char *GetDeviceSerialNumber(void) { return "10120000101"; }
static u8 GetHighestTempValue(void) { return 43; }

static u8 GetTcpIpMode(void) { return 1; }
static u32 GetIpAddDword(void) { return 0xC0A8141E; }
static u32 GetIpMaskDword(void) { return 0xFFFFFF00; }
static u32 GetIpGwDword(void) { return 0xC0A81401; }
static u16 GetHttpSrvPort(void) { return 80; }
static u16 GetTCP_IP_TimeOutValue(void) { return 300; }

static u8 GetPowState(void) { return 1; }
static u8 GetIR_State(void) { return 0; }
static u8 GetKeyState(void) { return 0; }
static u32 GetMatrixInterfaceBaudRate(void) { return 115200; }

static u8 GetMaxChannelPerDevice(void) { return 8; }
static u8 GetRoutPort(u8 idx) { return Route[idx]; }
static char *GetInputName(u8 idx) { return InputName[idx]; };
//
// dummy functions just to show it works


//----------------------------------------------------------------------------
//****************************************************************************
//----------------------------------------------------------------------------

// This function is called from SSI or when ever a JSON configuration file is
// downloaded or created.
//
// This function is designed to be called multiple times until RespLen == 0
// I embedd code we normally are limited in RAM and resorces and cannot allocate
// large memory buffers so this file is created in parts.
//
// char *Data : pointer to data buffer were the created data is written to
// int MaxDataLen : data buffer size, tested against overflow
// u32 Groups :  a bit map of up to 32 objects or "groups" in the JSON file. A
//               user can select to create a partial or a full file
// JsonUserData_t *UserData : a stracture defined and initialized in the user
//                            code see example of creating a file in ????
//
int GetDeviceConfigData(char *Data, int MaxDataLen, u32 Groups, JsonUserData_t *UserData)
{
	u16 RespLen = 0;
  bool Continue;
	int i;
  u8 JS_StartEnd = JS_PROCCESS_CONFIG;

  
  do
  {
    Continue = FALSE;   
        
    if(UserData->ProccessedGroupCount == 0)
    { 
      if(UserData->CurrentPart == 0)
      {
        // set the start/end flag as this is the first time we call it      
        JS_StartEnd = JS_START_CONFIG;
      
        // count how many groups we are going to proccess, in this case
        // it is all defined groups
        for(i=0; i<JS_MAX_GROUPS; i++)
        {
          UserData->NumOfGroupsToProccess += ((((Groups >> i) & 1) == 0)? 0 : 1);
        }
        
        UserData->NexPart = JS_LAST_PART;
        
        // find the first group to process
        do
        {
          if(((Groups >> UserData->ProccessedGroupCount) & 1) != 1)
          {      
            UserData->ProccessedGroupCount++;
          }
          else
          {
            break; 
          }  
        
        } while(UserData->ProccessedGroupCount < JS_MAX_GROUPS);
        
        //--------------------------------------------------------------------
        
        // check if we found any ?.. if not this is an error and 
        // should not happen ??
        if(UserData->ProccessedGroupCount >= JS_MAX_GROUPS)
        {
          // this should never happen, this is just a safty test
          return 0;
        }        
      }      
      
      // start processing the first defined group, JS_STATUS_GROUP... it is equal to 0x01
      // UserData->ProccessedGroupCount is equals to 0x00... after we end processing JS_STATUS_GROUP
      // we will advance UserData->ProccessedGroupCount by one and check if ( 0x01 << UserData->ProccessedGroupCount )
      // is one of the groups we need to process.      
      RespLen = GetDeviceGroupData(Data, MaxDataLen, (1 << UserData->ProccessedGroupCount), JS_StartEnd, UserData);
      
      if(UserData->NexPart == JS_LAST_PART)
      {
        // processed first group, decrement from total
        UserData->NumOfGroupsToProccess--;
                   
        // initialize CurrentPart to start from zero.
        UserData->CurrentPart = 0;
        
        // advance ProccessedGroupCount by one as we finished to proccess the first group
        UserData->ProccessedGroupCount++;
      }
      else
      {
        UserData->CurrentPart = UserData->NexPart;
      }
    }
    else if(UserData->ProccessedGroupCount >= 1)
    {
      JS_StartEnd = JS_PROCCESS_CONFIG;
      
      do
      {
        // this if checks if the bit coresponds to a group we need to process.
        // else it will cycle to the next bit
        if(((Groups >> UserData->ProccessedGroupCount) & 1) == 1)
        {
          // every time we call GetDeviceGroupData we set the NexPart t JS_LAST_PART
          // if the function returns with NexPart not equal JS_LAST_PART it means we have 
          // another part to proccess from the same group
          UserData->NexPart = JS_LAST_PART;
          RespLen = GetDeviceGroupData(Data, MaxDataLen, (1 << UserData->ProccessedGroupCount), JS_StartEnd, UserData);
            
          if(UserData->NexPart == JS_LAST_PART)
          {
            UserData->NumOfGroupsToProccess--;
            UserData->ProccessedGroupCount++;
            UserData->CurrentPart = 0;
          }
          else
          {            
            UserData->CurrentPart = UserData->NexPart;
            
            break;
          }  
        }
        else
        {
          UserData->ProccessedGroupCount++;
        }
          
      } while((RespLen == 0) && (UserData->ProccessedGroupCount < JS_MAX_GROUPS));
    }
    
  } while(Continue == TRUE);
 
 
  return RespLen;  
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

int GetDeviceGroupData(char *Data, int MaxDataLen, u32 JS_DataTypeGroups, u8 JS_StartEnd, JsonUserData_t *UserData)
{
  int DataLen = 0;
  
  
  if(JS_StartEnd & JS_START_CONFIG)
  {
    DataLen += snprintf(&Data[DataLen], MaxDataLen, "{\n");
    
    DataLen += snprintf(&Data[DataLen], MaxDataLen, "\t\"%s\":%d,\n", JS_UNIQUE_ID_TAG, UserData->UniqueID);
  }

  //----------------------------------------------------
  //----------------------------------------------------
  
  if(JS_DataTypeGroups & JS_STATUS_GROUP)
  {
    DataLen += GetStatusGroupData(&Data[DataLen], MaxDataLen, UserData);
  }
  //----------------------------------------------------
  else if(JS_DataTypeGroups & JS_NETWORK_GROUP)
  {
    DataLen += GetNetworkGroupData(&Data[DataLen], MaxDataLen, UserData);
  }
  //----------------------------------------------------   
  else if(JS_DataTypeGroups & JS_CONTROL_GROUP)
  {
    DataLen += GetControlGroupData(&Data[DataLen], MaxDataLen, UserData);
  }
  //----------------------------------------------------   
  else if(JS_DataTypeGroups & JS_IO_GROUP)
  {
    DataLen += GetIoGroupData(&Data[DataLen], MaxDataLen, UserData);
  }
  //----------------------------------------------------        
  else if(JS_DataTypeGroups & JS_IN_LABLES_GROUP)
  {
    DataLen += GetInLablesGroupData(&Data[DataLen], MaxDataLen, UserData);
  }

  //----------------------------------------------------
  //----------------------------------------------------

  //
  // This function should be called once for every group
  // If there is more then group we will seperate them with ','
  if(UserData->NexPart == JS_LAST_PART)
  {
    DataLen += snprintf(&Data[DataLen], MaxDataLen, "\t}%s", (((UserData->NumOfGroupsToProccess - 1) > 0)? ",\n" : "\n"));
  }

  //----------------------------------------------------
  
  if((UserData->NexPart == JS_LAST_PART) && (UserData->NumOfGroupsToProccess - 1) == 0)
  {
    DataLen += snprintf(&Data[DataLen], MaxDataLen, "}");
  }
  
  return DataLen;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static int GetStatusGroupData(char *Data, int MaxDataLen, JsonUserData_t *UserData)
{
  int DataLen = 0;
  u8 Temp;  
  u8 Arr[MAC_ADDR_SIZE] = {0};

  // update next part before continuing. If we will have  more
  // parts it will be updated later
  UserData->NexPart = JS_LAST_PART;
  
  if(UserData->CurrentPart == 0)
  {
    DataLen += snprintf(&Data[DataLen], MaxDataLen, "\t\"%s\":\n\t{\n", JS_STATUS_TAG);  
    
    DataLen += snprintf(&Data[DataLen], MaxDataLen, "\t\t\"%s\":\"%s\",\n", JS_MODEL_TAG, GetVersionString());
    DataLen += snprintf(&Data[DataLen], MaxDataLen, "\t\t\"%s\":\"%s\",\n", JS_HW_VERSION_TAG, GetHardwareVersion());
    GetMACadd(Arr);
    DataLen += snprintf(&Data[DataLen], MaxDataLen, "\t\t\"%s\":\"%02X:%02X:%02X:%02X:%02X:%02X\",\n", JS_MAC_ADDR_TAG, Arr[0], Arr[1], Arr[2], Arr[3], Arr[4], Arr[5]);
    
    UserData->NexPart = (UserData->CurrentPart + 1);
  }
  else if(UserData->CurrentPart == 1)
  {    
    DataLen += snprintf(&Data[DataLen], MaxDataLen, "\t\t\"%s\":\"%s\",\n", JS_SN_TAG, GetDeviceSerialNumber());
    
    Temp = GetHighestTempValue();
    DataLen += snprintf(&Data[DataLen], MaxDataLen, "\t\t\"%s\":\"%d°C -- %d°F\"\n", JS_TEMP_TAG, Temp, ((int)(Temp * 9 / 5 + 32.5)));
  }
    
  return DataLen;
}

//----------------------------------------------------------------------------

static int GetNetworkGroupData(char *Data, int MaxDataLen, JsonUserData_t *UserData)
{
  int DataLen = 0;
  DwOrBytes_t addr;

  if(UserData->CurrentPart == 0)
  {
    DataLen += snprintf(&Data[DataLen], MaxDataLen, "\t\"%s\":\n\t{\n", JS_NETWORK_TAG);

    DataLen += snprintf(&Data[DataLen], MaxDataLen, "\t\t\"%s\":\"%s\",\n", JS_DHCP_TAG, (GetTcpIpMode()==0)? PARAM_OFF : PARAM_ON);
  
    addr.Dword = GetIpAddDword();
	  DataLen += snprintf(&Data[DataLen], MaxDataLen, "\t\t\"%s\":\"%d.%d.%d.%d\",\n", JS_IP_ADDR_TAG, addr.Bytes[0], addr.Bytes[1], addr.Bytes[2], addr.Bytes[3]);
  
  	addr.Dword = GetIpMaskDword();
	  DataLen += snprintf(&Data[DataLen], MaxDataLen, "\t\t\"%s\":\"%d.%d.%d.%d\",\n", JS_MASK_ADDR_TAG, addr.Bytes[0], addr.Bytes[1], addr.Bytes[2], addr.Bytes[3]);
 
  	addr.Dword = GetIpGwDword();
	  DataLen += snprintf(&Data[DataLen], MaxDataLen, "\t\t\"%s\":\"%d.%d.%d.%d\",\n", JS_GW_ADDR_TAG, addr.Bytes[0], addr.Bytes[1], addr.Bytes[2], addr.Bytes[3]);

    UserData->NexPart = (UserData->CurrentPart + 1);
  }
  else if(UserData->CurrentPart == 1)
  {
    DataLen += snprintf(&Data[DataLen], MaxDataLen, "\t\t\"%s\":%d,\n", JS_HTTP_PORT_TAG, GetHttpSrvPort());
    DataLen += snprintf(&Data[DataLen], MaxDataLen, "\t\t\"%s\":%d\n", JS_TCP_TIMEOUT_TAG, GetTCP_IP_TimeOutValue());
  }

  return DataLen;
}

//----------------------------------------------------------------------------

static int GetControlGroupData(char *Data, int MaxDataLen, JsonUserData_t *UserData)
{
  int i, DataLen = 0;
  u8 Channels;


  if(UserData->CurrentPart == 0)
  {
    DataLen += snprintf(&Data[DataLen], MaxDataLen, "\t\"%s\":\n\t{\n", JS_CONTROL_TAG);

    DataLen += snprintf(&Data[DataLen], MaxDataLen, "\t\t\"%s\":\"%s\",\n", JS_POWER_TAG, (GetPowState()==0)? PARAM_OFF : PARAM_ON);
    DataLen += snprintf(&Data[DataLen], MaxDataLen, "\t\t\"%s\":\"%s\",\n", JS_IR_TAG, (GetIR_State()==0)? PARAM_OFF : PARAM_ON);
    DataLen += snprintf(&Data[DataLen], MaxDataLen, "\t\t\"%s\":\"%s\",\n", JS_KEYLOCK_TAG, (GetKeyState()==0)? PARAM_OFF : PARAM_ON);
    DataLen += snprintf(&Data[DataLen], MaxDataLen, "\t\t\"%s\":%d\n", JS_BAUD_RATE_TAG, GetMatrixInterfaceBaudRate());
  }
  
  return DataLen;  
}

//----------------------------------------------------------------------------

static int GetIoGroupData(char *Data, int MaxDataLen, JsonUserData_t *UserData)
{
  int i, DataLen = 0;  
  u8 Channels;

  
  DataLen += snprintf(&Data[DataLen], MaxDataLen, "\t\"%s\":\n\t{\n", JS_IO_TAG);
 
  DataLen += snprintf(&Data[DataLen], MaxDataLen, "\t\t\"%s\":[", JS_ROUTE_TAG);
  
  Channels = GetMaxChannelPerDevice();
  for(i=0; i<Channels; i++)
  {
    DataLen += snprintf(&Data[DataLen], MaxDataLen, "\"%d\"%s", GetRoutPort(i), ((i<(Channels-1)? "," : "]\n")));
	}
    
  return DataLen;  
}

//----------------------------------------------------------------------------
  
static int GetInLablesGroupData(char *Data, int MaxDataLen, JsonUserData_t *UserData)
{
  int i, DataLen = 0;
  u8 Channels;

  
  if(UserData->CurrentPart == 0)
  {
    DataLen += snprintf(&Data[DataLen], MaxDataLen, "\t\"%s\":\n\t{\n", JS_IN_LABLES_TAG);
    UserData->Idx = 0;
    
    UserData->CurrentPart = 1;
  }  
  
  if(UserData->CurrentPart == 1)
  {  
    Channels = GetMaxChannelPerDevice();
    for(i=0; ((i<JS_CONTROL_PARTIAL) && (UserData->Idx<Channels)); i++, UserData->Idx++)
    {
      DataLen += snprintf(&Data[DataLen], MaxDataLen, "\t\t\"%s%d\":\"%s\"%c\n", JS_IN_X_TAG, (UserData->Idx+1), GetInputName(UserData->Idx), ((UserData->Idx<(Channels-1)? ',' : ' ')));
    }
    
    if(UserData->Idx < (Channels-1))
    {
      UserData->NexPart = 1;
    }    
  }

  return DataLen;
}

//----------------------------------------------------------------------------

