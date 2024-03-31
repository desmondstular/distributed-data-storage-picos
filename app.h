/*
 * app.h
 *
 * Authors: Ben Nourse
 *   		 Desmond Stular (3067040)
 *
 *       Date: March 19, 2024
 */

#ifndef APP_H_
#define APP_H_

// PICOS Libraries
#include "phys_cc1350.h"
#include "plug_null.h"
#include "serf.h"
#include "ser.h"
#include "sysio.h"
#include "tcv.h"

// Millisecond for PICOS
#define MS (1024/1000)

// Default Network ID
#define NETWORK_ID 0

// Maximum sizes
#define RECORD_SIZE     20
#define DATABASE_SIZE     40
#define MAX_NEIGHBORS     25
#define CC1350_BUF_SZ     250

// Packet Lengths (header(2) + payload + trailer(2))
#define DIS_PACK_LEN        10
#define CREATE_PACK_LEN     30
#define CREATE_RESPACK_LEN	32
#define RESPONSE_PACK_LEN 10
#define DELETE_PACK_LEN   10

// Message Types
#define DIS_REQ 0x00
#define DIS_RES 0x01
#define NEW_REC 0x02
#define DEL_REC 0x03
#define RET_REC 0x04
#define RES_MSG 0x05

// Response message statuses
#define SUCCESS 0x01
#define DATA_FULL 0x02
#define DELETE_FAIL 0x03
#define NO_ENTRY 0x04


/*** Message structures ***/

/*
 * Message sent/received by the node to discover
 * and obtain a list of reachable neighbors that
 * belongs to the node group. Also used by node
 * to identify the structure of a receiving message.
 */
struct discoveryMsg
{
    word groupID;
    byte type;
    byte requestNum;
    byte senderID;
    byte receiverID;
};

/*
 * Message is sent when a sender node wishes to
 * create and store a record in another node node.
 */
struct newRecordMsg
{
    word groupID;
    byte type;
    byte requestNumber;
    byte senderID;
    byte receiverID;
    char record[RECORD_SIZE];
};

/*
 * Message is sent when a sender node wishes to
 * delete a record stored in another node.
 */
struct delRecordMsg
{
	word groupID;
	byte type;
	byte requestNumber;
	byte senderID;
	byte receiverID;
	byte recordIndex;
	byte padding; // not used
};

/*
 * Message is sent when a sender node wishes to
 * retrieve a record stored in another node.
 */
struct retRecordMsg
{
    word groupID;
    byte type;
    byte requestNumber;
    byte senderID;
    byte receiverID;
    char record[RECORD_SIZE];
    byte recordIndex;
    byte padding; // not used
};

/*
 * This message is formed and sent by a node
 * after receiving a request message (Create
 * Record Message, Delete, or Retrieve) and
 * after performing the corresponding action.
 */
struct responseMsg
{
    word groupID;
    byte type;
    byte requestNumber;
    byte senderID;
    byte receiverID;
    byte status;
    byte padding;
    char record[RECORD_SIZE];
};

/*
 * The Database entry struct
 * contains ownerID timeStamp and string message
 */
struct dbEntry
{
    word ownerID;
    lword timeStamp;
    char record[RECORD_SIZE];
};


#endif




