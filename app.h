/*
 * app.h
 *
*/

#ifndef APP_H_
#define APP_H_

// Maximum sizes
#define RECORD_SIZE 20;
#define DATABASE_SIZE 40;

// Response message statuses
#define SUCCESS 0x01;
#define DATA_FULL 0x02;
#define DELETE_FAIL 0x03;
#define NO_ENTRY 0x04;

// Message structures
struct discoveryMsg {
  word groupID;
  byte type;
  byte requestNum;
  byte senderID;
  byte receiverID;
};

struct newRecordMsg {
  word groupID;
  byte type;
  byte requestNumber;
  byte senderID;
  byte receiverID;
  char record[20];
}

struct delRetrRecordMsg {
  word groupID;
  byte type;
  byte requestNumber;
  byte senderID;
  byte receiverID;
  char record[20];
  byte recordIndex;
  byte padding; //not used
};

struct responseMsg {
  word groupID;
  byte type;
  byte requestNumber;
  byte senderID;
  byte receiverID;
  byte status;
  byte padding;
  char record[20];
};



#endif

