/*
 * app.cc
 *
 * CMPT 464 - Assignment 2
 * Authors: Ben Nourse
 *          Desmond Stular (3067040)
 *    Date: March 11, 2024
 */

#include "app.h"

// Session descriptor
int sfd;

// Node and group ID
byte groupID = 1;
byte nodeID = 1;

// Database and current size
struct dbEntry db[DATABASE_SIZE];
byte entryCount = 0;

// Reachable neighbors
byte neighbors[MAX_NEIGHBORS];
byte neighborCount = 0;

// Event variables
byte sending = YES;

/*
 * Receiver fsm: Waits for packets from other
 * nodes and then processes them.
*/
fsm receiver
{
	address packet;
	byte* type;
	struct responseMsg *delRespPL;

	// Wait to receive packet
	state WAIT_PKT:
		packet = tcv_rnp(WAIT_PKT, sfd);

	// Received packet
	state RECEIVED_PKT:
		type = (byte*)(packet+2);

		// Check type of packet
		// switch (*type) {
		// 	case DIS_REQ: 	proceed WAIT_PKT;	//todo
		// 	case DIS_RES: 	proceed WAIT_PKT;	//todo
		// 	case NEW_REC: 	proceed WAIT_PKT;	//todo
		// 	case DEL_REC: 	proceed WAIT_PKT;	//todo
		// 	case RET_REC: 	proceed WAIT_PKT;	//todo
		// 	case RES_MSG: 	proceed WAIT_PKT;	//todo
		// 	default: 		proceed WAIT_PKT;
		// }

	// Test a packet receive
	state RECEIVE_TEST:
		ser_outf(RECEIVE_TEST, "Received type: %u", type);
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

		//todo: check if entry in database. If empty send a no record found
		if (true) {
			delRespPL->groupID = groupID;
			delRespPL->type = delPtr->type;
			delRespPL->requestNumber = delPtr->requestNumber;
			delRespPL->senderID = nodeID;
			delRespPL->receiverID = delPtr->senderID;
			delRespPL->status =  DELETE_FAIL;
		}

		//todo: delete requested entry
		if (true) {
			delRespPL->groupID = groupID;
			delRespPL->type = delPtr->type;
			delRespPL->requestNumber = delPtr->requestNumber;
			delRespPL->senderID = nodeID;
			delRespPL->receiverID = delPtr->senderID;
			delRespPL->status =  DELETE_FAIL;
		}

	// Create packet
	state DELETE_REC_PACKET:
		packet = tcv_wnp(DELETE_REC_PACKET, sfd, PACKET_LEN);
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
		struct delRecordMsg *delPtr = (struct delRecordMsg *)(packet+1);

		// drop packet if not same node ID or group ID
		if (delPtr->receiverID != nodeID || delPtr->groupID != groupID) {
			proceed WAIT_PKT;
		}

		// drop if not awaiting response, or different request number
		if (waitingDelete != YES || delPtr->requestNumber != requestNumber) {
			proceed WAIT_PKT;
		}

		// change global variables for root fsm notification
		response = YES;
		delResponseStatus = delPtr->status

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

	// Initialize node
	state INIT_SESSION:
		phys_cc1350(0, PACKET_LEN);
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
		case 'd': case 'D': proceed DISCOVER_START;
		case 'g': case 'G': proceed MENU; 		// todo
		case 'n': case 'N': proceed MENU; 		// todo
		case 'f': case 'F': proceed MENU; 		// todo
		case 'c': case 'C': proceed MENU; 		// todo
		case 'r': case 'R': proceed MENU; 		// todo
		case 's': case 'S': proceed MENU; 		// todo
		case 'e': case 'E': proceed MENU; 		// todo
		default: 			proceed MENU;
	}

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
		packet = tcv_wnp(DISCOVER_PACKET, sfd, PACKET_LEN);
		packet[0] = NETWORK_ID;

		struct discoveryMsg* discPtr = (struct discoveryMsg *)(packet+1);
		*discPtr = *discPayload;

	// Send discover request packet
	state DISCOVER_SEND:
		tcv_endp(packet);
		discSendCount++;

		// Wait 3 seconds
		delay(3000 * MS, sending);
		release;

		// Send packet again if sent only once
		if (discSendCount < 2) {
			proceed DISCOVER_SEND;
		}

		// Free payload
		ufree(discPayload);
	
	// Show how many neighbors were reached
	state DISCOVER_REACHED:
		ser_outf(DISCOVER_REACHED, "Reached %d nodes!", neighborCount);
		proceed MENU;
	

	// ### END DISCOVERY REQUEST PROTOCOL END ###


	// ### START | DELETE ENTRY PROTOCOL | START ###
	byte destID;
	byte deleteIndex;
	struct delRecordMsg *delRecPl;

	// Delete protocol start; ask for destination ID
	state DELETE_START:
		ser_out(DELETE_START, "Enter destination ID: ");
	
	// Get destination ID
	state DELETE_GET_DEST:
		ser_inf(DELETE_GET_DEST, "%u", &destID);
	
	// Ask for index of entry to delete
	state DELETE_ASK_INDEX:
		ser_out(DELETE_ASK_INDEX, "Enter entry index: ");

	// Get index of entry to delete
	state DELETE_GET_INDEX:
		ser_inf(DELETE_GET_INDEX, "%u", &deleteIndex);
	
	// Build payload 
	state DELETE_PAYLOAD:
		delRecPL = (struct delRecordMsg*)umalloc(sizeof(struct delRecordMsg));
		requestNumber = (byte)(rnd() % 256);
		delRecPl->groupID = groupID;
		delRecPl->type = DEL_REC;
		delRecPl->requestNumber = requestNumber;
		delRecPl->senderID = nodeID;
		delRecPl->receiverID = destID;
		delRecPl->recordIndex = deleteIndex;

	// Fill packet with payload and send
	state DELETE_PACKET:
		packet = tcv_wnp(DELETE_PACKET, sfd, PACKET_LEN);
		packet[0] = NETWORK_ID;

		struct delRecordMsg* delPtr = (struct delRecordMsg *)(packet+1);
		*delPtr = *delRecPl;

	// Send packet to and check response
	state DELETE_SEND:
		tcv_endp(packet);

		// Wait 3 seconds
		delay(3000 * MS, sending);
		release;

		// Free payload
		ufree(discPayload);

		// If received no response
		if (response == NO) {
			proceed DELETE_NO_RESPONSE;
		}

		// If response success, proceed to success state
		if (delResponseStatus == SUCCESS) {
			proceed DELETE_RESPONSE_SUCCESS;
		}

		// If delete failed, proceed to response fail state
		else {
			proceed DELETE_RESPONSE_FAIL;
		}

	// Failed to delete as there was no response
	state DELETE_NO_RESPONSE:
		ser_out(DELETE_NO_RESPONSE, "\r\nFailed to reach the destination");
		proceed MENU;

	// Delete was successful
	state DELETE_RESPONSE_SUCCESS:
		ser_out(DELETE_RESPONSE_SUCCESS, "\r\nRecord Deleted");
		response = NO;
		proceed MENU;

	// Delete failed as the record did not exist
	state DELETE_RESPONSE_FAIL:
		ser_out(DELETE_RESPONSE_FAIL, "\r\nThe record does not exist on node %u", destNode);
		response = NO;
		proceed MENU;

}