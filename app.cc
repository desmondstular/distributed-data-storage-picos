/*
 * app.cc
 *
 * CMPT 464 - Assignment 2
 * Authors: Ben Nourse
 *          Desmond Stular (3067040)
 *    Date: March 11, 2024
*/

#include "app.h"
#include "phys_cc1350.h"
#include "plug_null.h"
#include "serf.h"
#include "ser.h"
#include "sysio.h"
#include "tcv.h"

// Session descriptor
int sfd;

// Node and group ID
byte nodeID = 1;
byte groupID = 1;


fsm root {

}

