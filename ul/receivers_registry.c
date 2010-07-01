/**
 * @file receiver_registry.c
 *
 * A registry of external applications willing to receive chunks.
 *
 * Based on a static array of information about registered applications.
 *
 * Napa-Wine project 2009-2010
 * @author: Giuseppe Tropea <giuseppe.tropea@lightcomm.it>
 */

#include "chunk_external_interface.h"
#include "ul_commons.h"

int ulRegisterApplication(char *address, int *port, char* path, int *pos) {
  //the following hopefully initializes all alements of the array with the same values...
  static ApplicationInfo apps[UL_MAX_EXTERNAL_APPLICATIONS] = {
    {"", 0, "", 0} //address, port, path, status
  };

  if(*pos >= UL_MAX_EXTERNAL_APPLICATIONS) {
    error("Registering application index > MAX error");
    return UL_RETURN_FAIL;
  }
  if(address[0] == '\0' && *port == 0 && path[0] == '\0') { //caller wants to retrieve status of application
    //give back the values of the application at position pos
    sprintf(address, "%s", apps[*pos].address);
    *port = apps[*pos].port;
    sprintf(path, "%s", apps[*pos].path);
    //debug("Somebody asked for the status of application at %s:%d%s, position %d", address, *port, path, *pos);
    return UL_RETURN_OK;
  }
  else { //register an application at the first free position
    //FIND A FREE INDEX
    int free_index = 1; //just to try, skipping position zero...
    //REGISTER BY FILLING INFO
    sprintf(apps[free_index].address, "%s", address);
    apps[free_index].port = *port;
    sprintf(apps[free_index].path, "%s", path);
    //give back the registered info
    *pos = free_index;
    info("Somebody registered an external application at %s:%d%s, position %d", address, *port, path, *pos);

    return UL_RETURN_OK;
  }
}
