#include <stdio.h>


#include "AppJson.h"

#define JSON_TEST_BUFF_SIZE          200
#define JSON_TEST_BUFF_MAX_SIZE      (JSON_TEST_BUFF_SIZE + 1)

int main(int argc, char* argv[])
{
  FILE *fp;
  bool FirstPart;
  int RetSize;
  char TempBuff[JSON_TEST_BUFF_MAX_SIZE] = {0};
  u32 Groups;
  JsonUserData_t JsonUserData;



  if((fp = fopen("TestFile.json", "wt+")) != NULL)
  {
    Groups = JS_CFG_ALL_GROUPS;
    FirstPart = 1;
    RetSize = 0;


    do
    {
      if(FirstPart == 1)
      {
        JsonUserData.UniqueID = 0;
        JsonUserData.ProccessedGroupCount = 0;
        JsonUserData.NumOfGroupsToProccess = 0;
        JsonUserData.NexPart = 0;
        JsonUserData.CurrentPart = 0;
      }

      RetSize = GetDeviceConfigData(TempBuff, JSON_TEST_BUFF_SIZE, Groups, &JsonUserData);

      if(RetSize > 0)
      {
        fwrite(TempBuff, 1, RetSize, fp);
      }

      FirstPart = 0;

    } while(RetSize > 0);
  }
  else
  {
     fprintf(stderr, "Cannot open input file.\n");
     return 1;
  }

  fclose(fp);

	return 0;
}
