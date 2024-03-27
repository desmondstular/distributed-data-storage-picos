/*
 * app.cc
 *
 * CMPT 464 - Assignment 2
 * Authors: Ben Nourse
 *      	Desmond Stular (3067040)
 *	Date: March 11, 2024
 */

#include "app.h"

// Session descriptor
int sfd;

// Node and group ID
byte groupID = 2;
byte nodeID = 1;

// Database and current size
struct dbEntry DB[DATABASE_SIZE];
byte entryCount = 0;

// Reachable neighbors
byte neighbors[MAX_NEIGHBORS];
byte neighborCount = 0;

// Event variables
byte sending = YES;

// Discover protocol
byte requestNum = 0;



/*
 * Receiver fsm: Waits for packets from other
 * nodes, parses them, and then sends a packet
 * response.
*/
fsm receiver
{
    // Default Variables
    address packet;

    // Packet Payloads
    struct discoveryMsg *dReqPayload;
    struct responseMsg *createRESMSG;
    
    byte requestNumberTracker = 0;
    char createResponseSwitch;
    // Wait to receive packet
    state WAIT_PKT:
   	 packet = tcv_rnp(WAIT_PKT, sfd);

    // Received packet
    state RECEIVED_PKT:
   	 byte *type = (byte*)(packet+2);
   	 
   	 ser_out(RECEIVED_PKT, "\n\r13413434");

   	 // Check type of packet
   	 switch (*type) {
   		 case DIS_REQ:     proceed DISCOVER_REQ_START;
   		 case DIS_RES:     proceed DISCOVER_RES_START;
   		 case NEW_REC:     proceed CREATE_REC_START;    //todo
   		 case DEL_REC:     proceed WAIT_PKT;    //todo
   		 case RET_REC:     proceed WAIT_PKT;    //todo
   		 case RES_MSG:     proceed RESPONSE_START;    //todo
   		 default:    	 proceed WAIT_PKT;
   	 }


    // ### START | DISCOVERY REQUEST PACKET | START ###

    // Check packet payload, and build payload if in same group
    state DISCOVER_REQ_START:
   	 struct discoveryMsg *payload = (struct discoveryMsg*)(packet+1);

   	 // If not in same group, end protocol
   	 if (payload->groupID != groupID) {
   		 proceed WAIT_PKT;
   	 }

   	 // Build discovery response payload
   	 dReqPayload = (struct discoveryMsg*)umalloc(sizeof(struct discoveryMsg));
   	 dReqPayload->groupID = groupID;
   	 dReqPayload->type = DIS_RES;
   	 dReqPayload->requestNum = payload->requestNum;
   	 dReqPayload->senderID = nodeID;
   	 dReqPayload->receiverID = payload->senderID;
    
    // Create packet and send, then end protocol
    state DISCOVER_REQ_SEND:
   	 packet = tcv_wnp(DISCOVER_REQ_SEND, sfd, DIS_PACK_LEN);
   	 packet[0] = NETWORK_ID;

   	 // Add payload to packet
   	 struct discoveryMsg* ptr = (struct discoveryMsg *)(packet+1);
   	 *ptr = *dReqPayload;

   	 // Send packet and free payload from memory
   	 tcv_endp(packet);
   	 ufree(dReqPayload);

   	 // End protocol
   	 proceed WAIT_PKT;

    // ### END | DISCOVERY REQUEST PACKET | END ###


    // ### START | DISCOVERY RESPONSE PACKET | START ###

    // Check packet payload for same groupID and request number
    state DISCOVER_RES_START:
   	 struct discoveryMsg *payload = (struct discoveryMsg*)(packet+1);
   	 byte recGroupID = payload->groupID;
   	 byte recRequestNum = payload->requestNum;
   	 byte senderID = payload->senderID;

   	 // If not in same group or different request number, stop
   	 if (recGroupID != groupID || recRequestNum != requestNum) {
   		 proceed WAIT_PKT;
   	 }

   	 // If in neighbors array, end protocol
   	 for (int i=0; i < neighborCount; i++) {
   		 if (neighbors[i] == senderID) {
   			 proceed WAIT_PKT;
   		 }
   	 }

   	 // Add to neighbors array, end protocol
   	 neighbors[neighborCount] = senderID;
   	 neighborCount++;
   	 proceed WAIT_PKT;

    // ### END | DISCOVERY RESPONSE PACKET | END ###
    
    // ### start create
    state CREATE_REC_START:
    
   	 //payload
   	 struct newRecordMsg* createRECPayload = (struct newRecordMsg *)(packet+1);
   	 
   	 ser_out(CREATE_REC_START, "\r\ngotpacket");
   	 
   	 // create response message for create
   	 createRESMSG = (struct responseMsg*)umalloc(sizeof(struct responseMsg));
   	 createRESMSG->groupID = createRECPayload->groupID;
   	 createRESMSG->type = RES_MSG;
   	 createRESMSG->requestNumber = createRECPayload->requestNumber;
   	 createRESMSG->senderID = nodeID;
   	 createRESMSG->receiverID = createRECPayload->senderID;
   	 
   	 
   	 //checks to see if nodeID are the same if no drop
   	 if (createRECPayload->receiverID != nodeID)
   	 {
   		 proceed WAIT_PKT;
   	 }
   	 if (createRECPayload->groupID != groupID)
   	 {
   		 proceed WAIT_PKT;
   	 }
   	 if(requestNumberTracker == createRECPayload->requestNumber)
   	 {
   	 	proceed WAIT_PKT;
   	 }
   	 if (entryCount <= 40)
   	 {
   		 DB[entryCount].ownerID = createRECPayload->senderID;
   		 for(int i =0; i>20;i++)
   		 {
   			 DB[entryCount].record[i] = createRECPayload->record[i];
   		 }
   		 
   		 DB[entryCount].timeStamp = seconds();
   		 entryCount++;
   		 createRESMSG->status = SUCCESS;
   	 }
   	 else
   	 {
   	 	createRESMSG->status = DATA_FULL;
   	 }
   	 requestNumberTracker = createRECPayload->requestNumber;
    	//ser_out(CREATE_REC_START, "\r\nthe message:");
    	//ser_out(CREATE_REC_START,DB[0].record);
    state create_RES_START:
   	 packet = tcv_wnp(create_RES_START, sfd, CREATE_RESPACK_LEN);
   	 packet[0] = NETWORK_ID;
   	 
    state CREATE_RES_SEND:
   	 struct responseMsg *createPTR = (struct responseMsg *)(packet+1);
   	 *createPTR= *createRESMSG;
   	 tcv_endp(packet);
   	 
   	 
   	 ufree(createRESMSG);
   	 proceed WAIT_PKT;
   	 
   state RESPONSE_START:

  	struct responseMsg* responsePayload = (struct responseMsg *)(packet+1);
  	
  	if(responsePayload->status == SUCCESS)
  	{
  		ser_out(RESPONSE_START, "\r\nData saved ");

  		
  	}
  	else if (responsePayload->status == DATA_FULL)
  	{
  		ser_out(RESPONSE_START, "\r\nNode Full");
  	}
  	proceed WAIT_PKT;
  	
}

/*
 * Root fsm that acts as the user interface.
 */
fsm root
{
    // Main Variables
    int c;
    address packet;
    struct discoveryMsg msg;

    // Discovery Request Protocol Variables
    int discSendCount;
    struct discoveryMsg *discPayload;
    
    //create protocol variables
    struct newRecordMsg *createPayload;
    int createSendCount = 0;
    
    // Initialize node
    state INIT_SESSION:
   	 phys_cc1350(0, CC1350_BUF_SZ);
   	 tcv_plug(0, &plug_null);
   	 sfd = tcv_open(NONE, 0, 0);

   	 // If session did not open
   	 if (sfd < 0)
   	 {
   		 diag("unable to open TCV session");
   		 syserror(EASSERT, "no session");
   	 }

   	 tcv_control(sfd, PHYSOPT_ON, NULL);

   	 // Start receiver FSM
   	 runfsm receiver;

    // Show sender menu and get input
    state MENU:
   	 ser_outf(MENU, "\r\nGroup %u Device #%u (%u/%u records)\r\n(G)roup ID\r\n(N)ew device ID\n\r(F)ind neighbors\r\n(C)reate record on neighbor\r\n(D)elete record on neighbor\r\n(R)etrieve record from neighbor\r\n(S)how local records\r\nR(e)set local storage\r\n\r\nSelection: ", groupID, nodeID, entryCount, DATABASE_SIZE);

    // Get user input
    state MENU_INPUT:
   	 ser_inf(MENU_INPUT, "%c", &c);

    switch (c)
    {
   	 case 'g': case 'G': proceed CHANGE_GROUPID_OUT;    	 
   	 case 'n': case 'N': proceed NEW_NODEID_OUT;    	 
   	 case 'f': case 'F': proceed DISCOVER_START;
   	 case 'c': case 'C': proceed CREATE_START;    	 
   	 case 'd': case 'D': proceed MENU;   	 // todo
   	 case 'r': case 'R': proceed MENU;    	 // todo
   	 case 's': case 'S': proceed MENU;    	 // todo
   	 case 'e': case 'E': proceed MENU;    	 // todo
   	 default:    		 proceed MENU;
    }
    // change groupID protocol
    state CHANGE_GROUPID_OUT:
   	 //states current group ID and node ID
   	 //ser_outf(CHANGE_GROUPID_OUT, "\r\nThe current group ID: %u for the node %u\n\n\rWhat do you want the new group ID to be:", groupID, nodeID);
   	 ser_out(CHANGE_GROUPID_OUT, "\r\nEnter new group id: ");
    state CHANGE_GROUPID_IN:
    	int GIDTemp = 0;
   	 // takes user input for groupID
   	 ser_inf(CHANGE_GROUPID_IN, "%u", &GIDTemp);
   	 groupID = GIDTemp;
   	 proceed MENU;
   	 
    // change nodeID protocol
    
    state NEW_NODEID_OUT:
    //states current group ID and node ID
   	 ser_out(NEW_NODEID_OUT, "\r\nEnter new node id: ");

    state NEW_NODEID_IN:
   	 // takes user input for nodeID
   	 int NIDTemp = 0;
   	 ser_inf(NEW_NODEID_IN, "%u", &NIDTemp);
   	 nodeID = NIDTemp;
   	 proceed MENU;
    // test message to makes sure everything is running
    // ### CREATE MESSAGE PROTOCOL ###
    
    
    state CREATE_START:
    
   	 //initaized create payload
   	 createSendCount = 0;
   	 createPayload = (struct newRecordMsg*)umalloc(sizeof(struct newRecordMsg));
   	 createPayload->groupID = groupID;
   	 createPayload->type = NEW_REC;
   	 createPayload->requestNumber = (byte)(lrnd() % 256);
   	 createPayload->senderID = nodeID;
   	 createPayload->receiverID = 0;
   	 
   	 
    state CREATE_RECEIVERID_OUT:
   	 ser_out(CREATE_RECEIVERID_OUT, "\r\nEnter the destination ID (0-25)");
   	 
    state CREATE_RECEIVERID_IN:
    	int temp = 0;
   	ser_inf(CREATE_RECEIVERID_IN, "%u", &temp);
    	createPayload->receiverID = temp;
    	
    state CREATE_GETMESSAGE_OUT:
   	 ser_out(CREATE_GETMESSAGE_OUT, "\r\nWhat is the message you want to send (maximum 20 characters)");
    state CREATE_GETMESSAGE_IN:
   	 char recordTemp[RECORD_SIZE];
   	 ser_inf(CREATE_GETMESSAGE_IN, "%c", &recordTemp);
   	 
   	 for(int i =0; i <RECORD_SIZE; i++)
   	 {
   	 	createPayload->record[i] = recordTemp[i];
   	 }
   	 
   	 
   	 
    state CREATE_PACKET:
   	packet = tcv_wnp(CREATE_PACKET, sfd, CREATE_PACK_LEN);
   	packet[0] = NETWORK_ID;
   	 
   	//cast create payload as a packet
   	struct newRecordMsg* recordPtr = (struct newRecordMsg *)(packet+1);
   	*recordPtr = *createPayload;
   	 
    state CREATE_SEND:
   	tcv_endp(packet);
   	delay(3000 * MS, CREATE_LOOP);
   	release;
   	
   state CREATE_LOOP:
   	createSendCount++;

   	 // Send packet twice
   	if (createSendCount < 2) {
   		proceed CREATE_PACKET;
   	}
  
   	
   	ufree(createPayload);
   	proceed MENU;
    // ### END CREATE PROTCOL ###
    // ### DISCOVERY REQUEST PROTOCOL ####

    // Reset neighbors + build payload
    state DISCOVER_START:
   	 discSendCount = 0;
   	 neighborCount = 0;
   	 //memset(neighbors, 0, sizeof(neighbors));

   	 // Fill discovery request packet
   	 discPayload = (struct discoveryMsg*)umalloc(sizeof(struct discoveryMsg));
   	 discPayload->groupID = groupID;
   	 discPayload->type = DIS_REQ;
   	 discPayload->requestNum = (byte)(lrnd() % 256);
   	 discPayload->senderID = nodeID;
   	 discPayload->receiverID = 0;

    // Fill packet with discovery payload
    state DISCOVER_PACKET:
   	 packet = tcv_wnp(DISCOVER_PACKET, sfd, DIS_PACK_LEN);
   	 packet[0] = NETWORK_ID;

   	 struct discoveryMsg* discPtr = (struct discoveryMsg *)(packet+1);
   	 *discPtr = *discPayload;

   	 // Save request number
   	 requestNum = discPayload->requestNum;

    // Send discover request packet
    state DISCOVER_SEND:
   	 // Send packet and wait 3 seconds
   	 tcv_endp(packet);
   	 delay(3000 * MS, DISCOVER_LOOP);
   	 release;
    
    state DISCOVER_LOOP:
   	 discSendCount++;

   	 // Send packet twice
   	 if (discSendCount < 2) {
   		 proceed DISCOVER_PACKET;
   	 }

   	 // Free payload
   	 ufree(discPayload);
    
    // Show how many and which neighbors were reached
    state DISCOVER_REACHED:
   	 char reached[50];

   	 // Build string of neighbors to show node user
   	 for (int i=0; i < neighborCount; i++) {
   		 form(reached, "#%u ", neighbors[i]);
   	 }

   	 // Shows neighbor nodes if found more than zero
   	 if (neighborCount != 0) {
   		 ser_outf(DISCOVER_REACHED, "\n\rFound %d neighbors!\n\rNeighbors: %s\n\r", neighborCount, reached);
   	 }
   	 else {
   		 ser_outf(DISCOVER_REACHED, "\n\rFound %d neighbors!\n\r", neighborCount, reached);
   	 }
   	 proceed MENU;
    

    // ### END DISCOVERY REQUEST PROTOCOL END ###

}




