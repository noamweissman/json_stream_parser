#ifndef __NW_JSON_H__
#define __NW_JSON_H__


#ifdef PC_DEFINED
  #include "typedefs.h"
#else
  #include "include_types.h"
#endif



#ifdef __cplusplus
extern "C" {
#endif

/**
  This JSON parser was based on portions from JSMN parser.
  
  JSMN uses static alocation for tokens and needs the full JSON file
  in a memory buffer. This is nice for small files but on embedded systems
  with minimal RAM this is a big problem.
  
  This JSON parser used portions of JSMN code together with some ideas from 
  other parsers. All this in order to create a stream parser.
  
  create on May 2016 by Noam Weissman
*/


#define JSON_BUFFER_SIZE        256
#define JSON_EMPTY              (-1)
#define JSON_STACK_MIN_DEPTH    0
#define JSON_STACK_MAX_DEPTH    10
  
#define JSON_FALSE_VALUE        "false"
#define JSON_FALSE_SIZE         5

#define JSON_TRUE_VALUE         "true"
#define JSON_TRUE_SIZE          4

#define JSON_NULL_VALUE         "null"
#define JSON_NULL_SIZE          4


typedef enum
{
  // stack dept limit reached
  JSON_NO_STACK       = (-1),

  // stack dept limit reached
  JSON_STACK_POP_FAIL = (-2),
  
  // string has errors
  JSON_DATA_ERROR     = (-3)
  
} JSON_Error_t;


// structure forward type;
struct JSON_ParserStruct;


/**
 * JSON is constracted from Objects. Every object has a Key and a Value pair/pairs
 * 
 * A Key is a string, always sorunded by double quotes while a Value can be onr
 * of the folloings:
 **  String
 **  Number
 **  Object
 **  Array
 **  Special value: true, false or null
*/
typedef enum
{
  eJT_None       = 0,
  eJT_StartObject   ,
  eJT_EndObject     ,  
  eJT_StartKey      ,
  eJT_EndKey        ,
  eJT_StartValue    ,
  eJT_EndValue      ,
  eJT_Error  
  
} JSON_Type_t;


typedef enum
{
  eJSubT_None       = 0,
  eJSubT_KeyString     ,
  eJSubT_ValueString   ,
  eJSubT_Special       ,
  eJSubT_Number        ,
  eJSubT_Fragment      ,
  eJSubT_EndData       ,  
  eJSubT_StartArray    ,
  eJSubT_EndArray      ,  

} JSON_SubType_t;


typedef enum
{
  eProcData_Done     = 0,
  eProcData_Fragment    ,
  eProcData_Error         

} ProccessDataStat_t;


// user function call back, user parsing module
typedef void(*JSON_UserFuncCB_t)(struct JSON_ParserStruct*);


typedef struct
{
  // Holds the current data type
  JSON_Type_t Type;

  // Holds the current data type
  JSON_SubType_t SubType;

} JSON_ParserStackItem_t;



/**
 * JSON parser structure.
 */
typedef struct JSON_ParserStruct
{
  // This buffer holds the temporary data read from the JSON file to
  // be used in the user call back.
  char Data[JSON_BUFFER_SIZE];

  // DataSize holds the byte count of daat in Buffer
  u32 DataSize;

  // Parser depth 
  int Depth;

  // parser stack
  JSON_ParserStackItem_t Stack[JSON_STACK_MAX_DEPTH];
  
  // Parser count... counts item found in file. If error it will be 
  // assigned a negative value 
  int Count;
  
  // user function call back, user parsing module, fucntion defined above.
  JSON_UserFuncCB_t UserFuncCB;
  
  // if we have a unicode fragment this value hold how many chars
  // we already saved
  int UnicodeSaved;

} JSON_Parser_t;


//-----------------------------------------------------------------------------

/**
 * Initialize the JSON parser object
 */
void JSON_Init(JSON_Parser_t *JSON_Parser, JSON_UserFuncCB_t UserFunc);

/**
 * Run JSON parser. It parses a JSON data string from Data buffer
 */
int JSON_ParseBuffer(JSON_Parser_t *JSON_Parser, const char *Data, int DataLen);

#ifdef __cplusplus
}
#endif


#endif // __NW_JSON_H__
