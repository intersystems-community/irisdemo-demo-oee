#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <stdbool.h>

const char* FILE_DATA_SOURCE_FOLDER = "/app/data/";

static char currentFilename[1024];

/*
Find the next available file on /app/data/ folder. 
We are not looping on over all the files just yet. 
We are just picking the first one we find in there for now.
We will use the column names to define the name of the nodes.
*/
bool findNextFile() 
{
  DIR *dataSourceDir;
  struct dirent *dirEntry;

  currentFilename[0]=0;

  dataSourceDir = opendir(FILE_DATA_SOURCE_FOLDER);

  if (dataSourceDir)
  {
    while ((dirEntry = readdir(dataSourceDir)) != NULL)
    {
      if (dirEntry->d_type == DT_REG)
      {
        strcpy(currentFilename, FILE_DATA_SOURCE_FOLDER);
        strcat(currentFilename, dirEntry->d_name);
        closedir(dataSourceDir);
        return(true);
      }
    }
    closedir(dataSourceDir);
  }
  
  return(false); //Not found
}

char* getCurrentFile() {
  return currentFilename;
}
