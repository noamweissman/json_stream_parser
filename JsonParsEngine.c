
#include "JsonParsEngine.h"

#include "string.h"


static JSON_ParserStackItem_t *JSON_Push(JSON_Parser_t *Parser)
{
  JSON_ParserStackItem_t *pStack = NULL;
  
  if((Parser->Depth + 1) < JSON_STACK_MAX_DEPTH)
  {
    pStack = &Parser->Stack[++Parser->Depth];
  }

  return pStack;
}

//-----------------------------------------------------------------------------

static JSON_ParserStackItem_t *JSON_Pop(JSON_Parser_t *Parser)
{
  JSON_ParserStackItem_t *pStack = NULL;
  
  if((Parser->Depth - 1) >= JSON_STACK_MIN_DEPTH)
  {
    pStack = &Parser->Stack[--Parser->Depth];
  }

  return pStack;  
}

//-----------------------------------------------------------------------------

/**
 * Initialize the parser for default before starting to parse data
*/
void JSON_Init(JSON_Parser_t *Parser, JSON_UserFuncCB_t UserFunc)
{
  int i;
  
  Parser->Count = JSON_STACK_MIN_DEPTH;
  Parser->Data[0] = '\0';
  Parser->DataSize = 0;
  Parser->Depth = JSON_STACK_MIN_DEPTH;
  Parser->UserFuncCB = UserFunc;
  Parser->UnicodeSaved = JSON_EMPTY;
    
  for(i=0; i<JSON_STACK_MAX_DEPTH; i++)
  {  
    Parser->Stack[i].Type = eJT_None;
    Parser->Stack[i].SubType = eJSubT_None; 
  } 
  
  UserFunc(Parser);
}

//-----------------------------------------------------------------------------

/**
 * JSON_GetString parses a key or value string and collects all chars between the
 * opening " and closing " into Parser->Data buffer
 *
*/
static ProccessDataStat_t JSON_GetString(JSON_Parser_t *Parser, const char *FileData, int DataLen, int *DataPos)
{
  ProccessDataStat_t Stat = eProcData_Done;
  char Ch;
  bool DataFragmented = TRUE;
  int i, DataCopy;
  
  // read all chars until the closing '"'...
  for(DataCopy=0, i=*DataPos; ((i<DataLen) && (FileData[i] != '\0')); i++)
  {
    Ch = FileData[i];
    
    // save unicode data !
    if(Parser->UnicodeSaved != JSON_EMPTY)
    {
      if(Parser->UnicodeSaved < 4)
      {
        // If it isn't a hex character we have an error 
        if( !(
              ((Ch >= 48) && (Ch <= 57))    ||  // 0-9
              ((Ch >= 65) && (Ch <= 70))    ||  // A-F
              ((Ch >= 97) && (Ch <= 102))       // a-f
             )
          )
        {
          Stat = eProcData_Error;
          goto EXIT_STRING_PARSE;
        }
        else
        {
          Parser->Data[Parser->DataSize++] = Ch;
          DataCopy++;
          Parser->UnicodeSaved++;
          continue;          
        }
      }
      else
      {
        Parser->UnicodeSaved = JSON_EMPTY;
      }
    }      
    
    
    // Quote: end of string
    if(Ch == '\"')
    {
      // terminate the key value string
      Parser->Data[Parser->DataSize] = '\0';
      DataFragmented = FALSE;
      
      // this will cause the loop to end.
      i = DataLen;
    }
    else if((Ch == '\\') && (Parser->DataSize + 1 < DataLen))
    {      
      // Backslash: Quoted symbol expected, check if we have the hole string ?
      switch(Ch)
      {
        // Allowed escaped symbols
        case '\"': 
        case '/' : 
        case '\\': 
        case 'b' :
        case 'f' : 
        case 'r' : 
        case 'n' : 
        case 't' :
          // ????
          Parser->Data[Parser->DataSize++] = Ch;
          DataCopy++;
        break;
          
        // Allows escaped symbol \uXXXX
        case 'u':
          // save the u + the \ that was prevoiusly saved
          Parser->Data[Parser->DataSize++] = Ch;
          DataCopy++;
          Parser->UnicodeSaved = 0;
        break;
        
        default:
          Stat = eProcData_Error;
        break;
      }
    }
    else
    {
      Parser->Data[Parser->DataSize++] = Ch;
      DataCopy++;
    }
    
    // originally the goto statment was in the above default block
    // but compiler complained 'statmeant is unreachable"...
    if(Stat == eProcData_Error)
    {
      goto EXIT_STRING_PARSE;
    }
  }

  
  // if we did not find the '"' it means we have fragmented data
  if(DataFragmented == TRUE)
  {
    Stat = eProcData_Fragment;
  }
  else
  {
    // data processed, not fragmented. Update DataPos
    // to point to the char after the '"'
    *DataPos += DataCopy;
  }

  
EXIT_STRING_PARSE:  
  
  return Stat;
}

//-----------------------------------------------------------------------------

/**
 * JSON_ParseBuffer is the main JSON engine parser. It parses one buffer 
 * after the other (stream).
 *
 * FileData is a pointer to the current buffer
 * DataLen is the count bytes to parse in the current buffer
*/
int JSON_ParseBuffer(JSON_Parser_t *Parser, const char *FileData, int DataLen)
{
  int DataPos;
  char Ch;
  JSON_ParserStackItem_t *pStack;
  ProccessDataStat_t ProccessDataStat;
  
  
  for(DataPos=0; ((DataPos < DataLen) && (FileData[DataPos] != '\0')); DataPos++)
  {
    // read char from buffer and decide what we got ?
    Ch = FileData[DataPos];
    Parser->Count++;
    
    // use a helper pointer
    pStack = &Parser->Stack[Parser->Depth];
    
    
    if(
       ((pStack->Type == eJT_StartKey) && (pStack->SubType == eJSubT_KeyString))      ||
       ((pStack->Type == eJT_StartValue) && (pStack->SubType == eJSubT_ValueString))
      )
    {
      // parse data and collect the key string. 
      ProccessDataStat = JSON_GetString(Parser, FileData, DataLen, &DataPos);
      if(ProccessDataStat == eProcData_Done)
      {
        // we finished handling the key string or the value string. change type
        // not to enter into this again...
        if(pStack->Type == eJT_StartKey)
        {
          pStack->Type = eJT_EndKey;
        }
        else if((pStack->Type == eJT_StartValue) && (pStack->SubType == eJSubT_ValueString))
        {
          pStack->Type = eJT_EndValue;
        }
        
        // call user callback with key/value string
        Parser->UserFuncCB(Parser);
        Parser->DataSize = 0;        
      }
      else 
      {
        // if data has some iligal value in it return with an error
        if(ProccessDataStat == eProcData_Error)
        {
          // an object it is an error
          Parser->Count = JSON_DATA_ERROR;
        }  

        // data is probably fragmented, get out from the JSON_ParseBuffer
        // get more data and continue
        goto JSON_PARSE_EXIT;
      }
    }
    else
    {    
      // if we have an array collect all data and trnsfer it to the user 
      // CB for handling
      if(pStack->SubType == eJSubT_StartArray)
      {
        if(Ch != ']')
        {
          Parser->Data[Parser->DataSize++] = Ch;
          continue;
        }  
      }
      
      
      // evaluate the character just read. It is either a value or a new object
      switch(Ch)
      {
        case '{':
          if(pStack->Type == eJT_None)
          {
            pStack->Type = eJT_StartObject;
            Parser->UserFuncCB(Parser);
            
            pStack = JSON_Push(Parser);
            if(pStack == NULL)
            {
              Parser->Count = JSON_NO_STACK;
              goto JSON_PARSE_EXIT;
            }
            
            pStack->Type = eJT_StartKey;
          }
          else if(pStack->Type == eJT_StartValue)
          {
            // if it is not an array data handle it here, else
            // it will be handled in user CB
            if(pStack->SubType != eJSubT_StartArray)
            {              
              // we have a nesetd object, first advance the stack
              // dept and then take a pointer to the next one.
              pStack = JSON_Push(Parser);
              if(pStack == NULL)
              {
                Parser->Count = JSON_NO_STACK;
                goto JSON_PARSE_EXIT;
              }
              
              pStack->Type = eJT_StartObject;
              Parser->UserFuncCB(Parser);
              
              pStack->Type = eJT_StartKey;
            }
          }
          else
          {
            // if Ch is '{' but we are already started handling
            // an object it is an error
            Parser->Count = JSON_DATA_ERROR;
            goto JSON_PARSE_EXIT;
          }
        break;

        //---------------------------------------------------------------------------
          
        case '[':
          if(pStack->Type == eJT_StartValue)
          {
            pStack->SubType = eJSubT_StartArray;
            Parser->DataSize = 0;
            
            // if we start parsing an array, add the data including the
            // array brace
            Parser->Data[Parser->DataSize++] = '[';
          }
        break;
    
        //---------------------------------------------------------------------------
    
        case '\"': 
          if(pStack->Type == eJT_StartKey)
          {
            pStack->SubType = eJSubT_KeyString;
          }
          else if(pStack->Type == eJT_StartValue)
          {
            pStack->SubType = eJSubT_ValueString;
          }
        break;

        //---------------------------------------------------------------------------
          
        case ':':
          if(pStack->Type == eJT_EndKey)
          {            
            pStack->Type = eJT_StartValue;
          }  
          else
          {                        
            // if Ch is ':' but we are reading the key or string value
            // it is an error. Key's or strings will be handled in a seperate
            // function !
            Parser->Count = JSON_DATA_ERROR;
            goto JSON_PARSE_EXIT;
          }
        break;
        
        //---------------------------------------------------------------------------              
          
        case '}':
          // if we got '}' after parsing a value it means end of 
          // object and maybe end of JSON
          // if it is a string it was handled above after calling function
          // JSON_GetString. Now we need to handle this special value before continuing
          if((pStack->SubType == eJSubT_Special) || (pStack->SubType == eJSubT_Number))
          {
            Parser->UserFuncCB(Parser);
          }
        
          if((pStack->Type == eJT_EndValue) || (pStack->Type == eJT_EndObject))
          {
            if(Parser->Depth >= JSON_STACK_MIN_DEPTH)
            {
              pStack->Type = eJT_None;
              pStack->SubType = eJSubT_None;
              
              // decrese dept by one.
              pStack = JSON_Pop(Parser);
              if(pStack == NULL)
              {
                Parser->Count = JSON_STACK_POP_FAIL;
                goto JSON_PARSE_EXIT;
              }
              
              // normal object ending
              if(Parser->Depth > JSON_STACK_MIN_DEPTH)
              {
                pStack->Type = eJT_EndObject;
                Parser->UserFuncCB(Parser);
                Parser->DataSize = 0;
                continue;
              }
                
              // this is the normal file ending
              if(((Parser->Depth == JSON_STACK_MIN_DEPTH) && (pStack->Type == eJT_StartObject)))
              {
                pStack->Type = eJT_EndObject;
                Parser->UserFuncCB(Parser);
              }
              else
              {
                // this is an error, this should not happen
                Parser->Count = JSON_DATA_ERROR;
                
                pStack->Type = eJT_Error;
                Parser->UserFuncCB(Parser);
              }
              
              goto JSON_PARSE_EXIT;
            }
          }
        break;

        //---------------------------------------------------------------------------          
          
        case ']':
          if(pStack->SubType == eJSubT_StartArray)
          {
            // we procecessed an array now we end it, call user CB and 
            // look what else we have
            Parser->Data[Parser->DataSize++] = ']';
            Parser->Data[Parser->DataSize] = '\0';
            pStack->Type = eJT_EndValue;
            Parser->UserFuncCB(Parser);
            Parser->DataSize = 0;
            
            pStack->SubType = eJSubT_EndArray;
          }
        break;
    
        //---------------------------------------------------------------------------
    
        case ',':          
          if((pStack->Type == eJT_EndValue) || (pStack->Type == eJT_EndObject))
          {
            if((pStack->SubType == eJSubT_Special) || (pStack->SubType == eJSubT_Number))
            {
              Parser->UserFuncCB(Parser);
              Parser->DataSize = 0;
            }
            
            pStack->Type = eJT_StartKey;
            pStack->SubType = eJSubT_None;
          }
          else
          {
            if(pStack->SubType != eJSubT_StartArray)
            {            
              // if Ch is ',' but we are reading the key or string value
              // it is an error ',' in a string is handled inside function
              // JSON_GetString
              Parser->Count = JSON_DATA_ERROR;
              goto JSON_PARSE_EXIT;
            }
          }     
        break;
        
        //---------------------------------------------------------------------------      
          
        case '\t': 
        case '\r': 
        case '\n': 
        case ' ' :
          // whith spaces .. do nothing
        break;
        
        //---------------------------------------------------------------------------      
        
        case 'f': // false
        case 't': // false
        case 'n': // false          
          switch(Ch)
          {
            case 'f':  
              strcpy(&Parser->Data[Parser->DataSize], JSON_FALSE_VALUE);
              Parser->DataSize = JSON_FALSE_SIZE;
            break;
            //----------------------------------------------------------
            case 't':  
              strcpy(&Parser->Data[Parser->DataSize], JSON_TRUE_VALUE);
              Parser->DataSize = JSON_TRUE_SIZE;
            break;
            //----------------------------------------------------------
            case 'n':  
              strcpy(&Parser->Data[Parser->DataSize], JSON_NULL_VALUE);
              Parser->DataSize = JSON_NULL_SIZE;
            break;
            //----------------------------------------------------------            
          }
          
          pStack->SubType = eJSubT_Special;
          Parser->Data[Parser->DataSize] = '\0';
          pStack->Type = eJT_EndValue;
        break;
        //-------------------------------------
        //-------------------------------------          
        case '-': // negative number
        case '0': // 0 char
        case '1': // 0 char          
        case '2': // 0 char
        case '3': // 0 char          
        case '4': // 0 char
        case '5': // 0 char          
        case '6': // 0 char
        case '7': // 0 char          
        case '8': // 0 char
        case '9': // 0 char          
        case '.': // 0 char              
        case 'e': // 0 char
        case 'E': // 0 char  
          if(pStack->SubType != eJSubT_Special)
          {
            Parser->Data[Parser->DataSize++] = Ch;
            Parser->Data[Parser->DataSize] = '\0';
            pStack->SubType = eJSubT_Number;
            pStack->Type = eJT_EndValue;
          }
        break;
                  
      } // end switch       
    } // end else
  } // end for

JSON_PARSE_EXIT:
  
  // this is an error !!!!!
  if(Parser->Count < 0)
  {
    // reinitialize user function to initialize it !!
    JSON_Init(Parser,  Parser->UserFuncCB);
  }
    
  
  return Parser->Count;
}
