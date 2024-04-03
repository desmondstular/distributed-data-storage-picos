/*
 * app.cc
 *
 * CMPT 464 - Assignment 2
 * Authors: Ben Nourse (3086842)
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
byte response = NO;

// Root and receiver communication
byte requestNum = 0;
byte responseType = 0;
byte responseStatus = 0;

int reponseTimeOut =0;
/*
 * Deletes a database entry by shifting entries
 * over if their is a hole from deletion.
*/
void deleteEntry (int index) {
	int i;
	struct dbEntry temp;
	// Shift over each struct till last index
	for (i = index; i < entryCount-1; i++) {
		DB[i] = DB[i+1];
	}

	DB[i] = temp;

	// Remove one from count
	entryCount --;
}


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
    struct responseMsg *delRespPL;
    struct responseMsg *retrieveRESMSG;


    byte requestNumberTracker = 0;
    // Wait to receive packet
    state WAIT_PKT:
   		packet = tcv_rnp(WAIT_PKT, sfd);

    // Received packet
    state RECEIVED_PKT:
   		byte *type = (byte*)(packet+2);

		// Check type of packet
		switch (*type) {
			case DIS_REQ:     proceed DISCOVER_REQ_START;
			case DIS_RES:     proceed DISCOVER_RES_START;
			case NEW_REC:     proceed CREATE_REC_START;    //todo
			case DEL_REC:     proceed DELETE_REC_START;
			case RET_REC:     proceed RETRIEVE_REC_START;    //todo
			case RES_MSG:     proceed CHECK_RES_TYPE;
			default:    	 proceed WAIT_PKT;
		}

	// Checks reponse message protocol type
	state CHECK_RES_TYPE:
		switch (responseType) {
			case CREATE:	proceed RESPONSE_START;
			case RETRIEVE:	proceed RESPONSE_START;
			case DELETE:	proceed DELETE_RESP_START; 
			default:		proceed WAIT_PKT;
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
			strncpy(DB[entryCount].record,createRECPayload->record,RECORD_SIZE);
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
   	 //### end create
	 //## START RETRIEEVE RECIVEVE
	state RETRIEVE_REC_START:
		struct retRecordMsg* retrieveRECPayload = (struct retRecordMsg *)(packet+1);
		 
		retrieveRESMSG = (struct responseMsg*)umalloc(sizeof(struct responseMsg));
		retrieveRESMSG->groupID = retrieveRECPayload->groupID;
		retrieveRESMSG->type = RES_MSG;
		retrieveRESMSG->requestNumber = retrieveRECPayload->requestNumber;
		retrieveRESMSG->senderID = nodeID;
		retrieveRESMSG->receiverID = retrieveRECPayload->senderID;

		 //checks to see if nodeID are the same if no drop
  	  	if (retrieveRECPayload->receiverID != nodeID)
		{
			proceed WAIT_PKT;
		}
		if (retrieveRECPayload->groupID != groupID)
		{
			proceed WAIT_PKT;
		}
		if(requestNumberTracker == retrieveRECPayload->requestNumber)
		{
			proceed WAIT_PKT;
		}
		requestNumberTracker = retrieveRECPayload->requestNumber;
		
		if(strlen(DB[retrieveRECPayload->recordIndex].record) != 0)
		{
			strncpy(retrieveRESMSG->record,DB[retrieveRECPayload->recordIndex].record,RECORD_SIZE);
			retrieveRESMSG->status = SUCCESS;
		}
		else
		{
			retrieveRESMSG->status = NO_ENTRY;
		}

	state RETRIEVE_RES_START:
		packet = tcv_wnp(RETRIEVE_RES_START, sfd, RETRIEVE_RESPACK_LEN);
		packet[0] = NETWORK_ID;
	
	state RETRIEVE_RES_SEND:
		struct responseMsg *RETRIEVEPTR = (struct responseMsg *)(packet+1);
		*RETRIEVEPTR= *retrieveRESMSG;
		tcv_endp(packet);
		
		ufree(retrieveRESMSG);
		proceed WAIT_PKT;

    state RESPONSE_START:
		struct responseMsg* responsePayload = (struct responseMsg *)(packet+1);
		reponseTimeOut = 1;
		if(responsePayload->status == SUCCESS)
		{
			if(responseType == CREATE)
			{
				ser_out(RESPONSE_START, "\r\nData saved ");
			}
			else if (responseType == RETRIEVE)
			{
				ser_outf(RESPONSE_START, "\r\nRecord Received from %u: %s",responsePayload->senderID,responsePayload->record);
			}

			

			
		}
		else if (responsePayload->status == DATA_FULL)
		{
			ser_out(RESPONSE_START, "\r\nNode Full");
		}
		else if(responsePayload->status == NO_ENTRY)
		{
			 ser_outf(RESPONSE_START, "\r\nThe record does not exist on node %u",responsePayload->senderID);
		}
		proceed WAIT_PKT;
    
    // ### START | Delete Record Protocol | Start ###

	state DELETE_REC_START:
		struct delRecordMsg *delPtr = (struct delRecordMsg *)(packet+1);
		// drop packet if not same node ID or group ID
		if (delPtr->receiverID != nodeID || delPtr->groupID != groupID) {
			proceed WAIT_PKT;
		}
		
		// Initialize packet payload
		delRespPL = (struct responseMsg*)umalloc(sizeof(struct responseMsg));

		// delete fail if index does not have entry
		if (delPtr->recordIndex >= entryCount) {
			delRespPL->status =  DELETE_FAIL;
		} 
		else {
			struct dbEntry temp;
			if (entryCount == 1) {
				DB[0] = temp;
			}
			else {
				int i = delPtr->recordIndex;
				// Delete entry at index
				// Shift over each struct till last index
				for (i; i < entryCount-1; i++) {
					DB[i] = DB[i+1];
				}
				DB[i] = temp;
			}

			// Remove one from count, set to success status
			entryCount--;
			delRespPL->status =  SUCCESS;
		}

		// Build rest of payload
		delRespPL->groupID = groupID;
		delRespPL->type = RES_MSG;
		delRespPL->requestNumber = delPtr->requestNumber;
		delRespPL->senderID = nodeID;
		delRespPL->receiverID = delPtr->senderID;

	state TEST:
		ser_out(TEST, "\n\rdelete test2");

	// Create packet
	state DELETE_REC_PACKET:
		packet = tcv_wnp(DELETE_REC_PACKET, sfd, RESPONSE_PACK_LEN);
		packet[0] = NETWORK_ID;

	// Add payload, send packet, end protocol
	state DELETE_REC_SEND:
		struct responseMsg *ptr = (struct responseMsg *)(packet+1);
		*ptr = *delRespPL;
		tcv_endp(packet);
		ufree(delRespPL);
		proceed WAIT_PKT;

	// ### END | DELETE RECORD PROTOCOL | END ###

	
	// ### START | DELETE RECORD RESPONSE | START ###

	state DELETE_RESP_START:
		ser_out(DELETE_RESP_START, "\n\r Response test \n\r");
		struct responseMsg *delPtr = (struct responseMsg *)(packet+1);

		// drop packet if not same node ID or group ID
		if (delPtr->receiverID != nodeID || delPtr->groupID != groupID) {
			proceed WAIT_PKT;
		}

		// drop if not awaiting response, or different request number
		if (delPtr->requestNumber != requestNum) {
			proceed WAIT_PKT;
		}

		// change global variables for root fsm notification
		response = YES;
		responseStatus = delPtr->status;

		// End of protocol
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
    
    // create protocol variables
    struct newRecordMsg *createPayload;
    int createSendCount = 0;

	// Delete Protocol Variables
	byte deleteIndex = 0;
	int dest = 0;
	struct delRecordMsg *delRecPl;
	//  retreieve potocol varaibles
	int retrieveSendCount = 0;
	struct retRecordMsg *retrievePayload;

	// Display DB Entries Variables
	int currentIndex = 0;
    
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
			case 'd': case 'D': proceed DELETE_START;
			case 'r': case 'R': proceed RETRIEVE_START;    	 // todo
			case 's': case 'S': proceed DISPLAY_START;
			case 'e': case 'E': proceed RESET_START;
			default:    		proceed MENU;
		}
    // change groupID protocol
    state CHANGE_GROUPID_OUT:
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
		responseType = CREATE;
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
		ser_in(CREATE_GETMESSAGE_IN, createPayload->record,RECORD_SIZE);

   	 
   	 
   	 
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

	 // ##3 START RRETRIEVE PRTOCOL ##


    
	state RETRIEVE_START:
		responseType = RETRIEVE;
		retrieveSendCount = 0;
		retrievePayload = (struct retRecordMsg*)umalloc(sizeof(struct retRecordMsg));
		retrievePayload->groupID = groupID;
		retrievePayload->type = RET_REC;
		retrievePayload->requestNumber = (byte)(lrnd() % 256);
		retrievePayload->senderID = nodeID;
		retrievePayload->receiverID = 0;
    state RETRIEVE_RECEIVERID_OUT:
		ser_out(RETRIEVE_RECEIVERID_OUT, "\r\nEnter the destination ID (0-25) ");
	state RETRIEVE_RECEIVERID_IN:
		int temp = 0;
		ser_inf(RETRIEVE_RECEIVERID_IN, "%u", &temp);
		retrievePayload->receiverID = temp;

	state RETRIEVE_INDEX_OUT:
		 ser_out(RETRIEVE_INDEX_OUT, "\r\nWhat record index do you want to retrieve ");
	state RETRIEVE_INDEX_IN:
		int tempIndex = 0;
		ser_inf(RETRIEVE_INDEX_IN, "%u",&tempIndex);
		retrievePayload->recordIndex = tempIndex;
	state RETRIEVE_PACKET:
		packet = tcv_wnp(RETRIEVE_PACKET, sfd, RETRIEVE_PACKET_LEN);
		packet[0] = NETWORK_ID;
		//cast reteive  payload as a packet
		struct retRecordMsg* recordRETPtr = (struct retRecordMsg *)(packet+1);
		*recordRETPtr = *retrievePayload;
	state RETRIEVE_SEND:
		tcv_endp(packet);
		delay(3000 * MS, RETRIEVE_LOOP);
		release;
	
	state RETRIEVE_LOOP:
		retrieveSendCount++;
		// Send packet twice
		if (retrieveSendCount < 2) {
			proceed RETRIEVE_PACKET;
		}
		if(reponseTimeOut != 1)
		{
			ser_out(CREATE_LOOP, "\r\nNo response");
		}
		ufree(retrievePayload);
		proceed MENU;

	// ### retrieve END
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
		currentIndex = 0;

		// Shows neighbor nodes if found more than zero
		if (neighborCount != 0) {
			ser_outf(DISCOVER_REACHED, "\n\rFound %d neighbors!\n\rNeighbors: | ", neighborCount);
		}
		else {
			ser_outf(DISCOVER_REACHED, "\n\rFound %d neighbors!\n\r", neighborCount);
			proceed MENU;
		}
	
	state DISCOVER_NEIGHBOR:
		ser_outf(DISCOVER_NEIGHBOR, "#%u | ", neighbors[currentIndex]);
		currentIndex++;

	// If more neighbors, keep printing, otherwise stop
	state DISCOVER_END:
		if (currentIndex < neighborCount) {
			proceed DISCOVER_NEIGHBOR;
		}

		currentIndex = 0;
		proceed MENU;

    // ### END DISCOVERY REQUEST PROTOCOL END ###


	// ### START | DELETE ENTRY PROTOCOL | START ###

	// Delete protocol start; ask for destination ID
	state DELETE_START:
		ser_out(DELETE_START, "Enter destination: ");

	// Get destination ID
	state DELETE_GET_DEST:
		ser_inf(DELETE_GET_DEST, "%d", &dest);

	// Ask for index of entry to delete
	state DELETE_INDEX:
		ser_out(DELETE_INDEX, "Enter index: ");

	// Get index of entry to delete
	state DELETE_GET_INDEX:
		ser_inf(DELETE_GET_INDEX, "%u", &deleteIndex);
	
	// Build payload 
	state DELETE_PAYLOAD:
		responseType = DELETE;
		delRecPl = (struct delRecordMsg*)umalloc(sizeof(struct delRecordMsg));
		requestNum = (byte)(lrnd() % 256);
		delRecPl->groupID = groupID;
		delRecPl->type = DEL_REC;
		delRecPl->requestNumber = requestNum;
		delRecPl->senderID = nodeID;
		delRecPl->receiverID = dest;
		delRecPl->recordIndex = deleteIndex;

	// Fill packet with payload and send
	state DELETE_PACKET:
		packet = tcv_wnp(DELETE_PACKET, sfd, DELETE_PACK_LEN);
		packet[0] = NETWORK_ID;

		struct delRecordMsg* delPtr = (struct delRecordMsg *)(packet+1);
		*delPtr = *delRecPl;

	// Send packet to and check response
	state DELETE_SEND:
		tcv_endp(packet);

		// Wait 3 seconds
		delay(3000 * MS, DELETE_SENT);
		release;
	
	state DELETE_SENT:

		// Free payload
		ufree(discPayload);

		// Clear response type
		responseType = 0;

		// If received no response
		if (response == NO) {
			proceed DELETE_NO_RESPONSE;
		}

		// If response success, proceed to success state
		if (responseStatus == SUCCESS) {
			proceed DELETE_RESPONSE_SUCCESS;
		}

		// If delete failed, proceed to response fail state
		else {
			proceed DELETE_RESPONSE_FAIL;
		}

	// Failed to delete as there was no response
	state DELETE_NO_RESPONSE:
		ser_out(DELETE_NO_RESPONSE, "\r\nFailed to reach the destination\n\r");
		proceed MENU;

	// Delete was successful
	state DELETE_RESPONSE_SUCCESS:
		ser_out(DELETE_RESPONSE_SUCCESS, "\r\nRecord Deleted\n\r");
		response = NO;
		proceed MENU;

	// Delete failed as the record did not exist
	state DELETE_RESPONSE_FAIL:
		ser_outf(DELETE_RESPONSE_FAIL, "\r\nThe record does not exist on node %u\n\r", dest);
		response = NO;
		proceed MENU;

	// ### END | DELETE ENTRY PROTOCOL | END ###


	// ### START | DISPLAY DATABASE | START ###

	// START: Display all entries in the nodes db
	state DISPLAY_START:
		currentIndex = 0;
		// If empty, display message and leave
		if (entryCount == 0) {
			ser_out(DISPLAY_START, "\n\rStorage empty\n\r");
			proceed MENU;
		}
		// display table header
		ser_out(DISPLAY_START, "\r\nIndex\tTimestamp\tOwner ID\tRecord Data\r\n");

	// Displays entry at current index
	state DISPLAY_ENTRY:
		char *record;
		struct dbEntry *ptr = &DB[currentIndex];
		word owner = ptr->ownerID;
		lword timeStamp = ptr->timeStamp;
		record = ptr->record;

		// Display entry
		ser_outf(DISPLAY_ENTRY, "%u\t%u\t\t%u\t%s\r\n", currentIndex, timeStamp, owner, ptr->record);

	// Check if more entries, if not end
	state DISPLAY_END:
		currentIndex++;

		// If still more entries, keep displaying
		if (currentIndex < entryCount) {
			proceed DISPLAY_ENTRY;
		}
		
		proceed MENU;

	// ### END | DISPLAY DATABASE | END ###


	// ### START | RESET STORAGE | START ###

	// START: Clear database entries stored on node
	state RESET_START:
		struct dbEntry empty;
		for (int i=0; i < DATABASE_SIZE; i++) {
			DB[i] = empty;
		}
		entryCount = 0;
	
	state RESET_END:
		ser_out(RESET_END, "\n\rStorage is now empty\n\r");
		proceed MENU;

	// ### END | RESET STORAGE | END ###
}