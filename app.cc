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
byte groupID = 2;
byte nodeID = 1;

// Database and current size
struct dbEntry db[DATABASE_SIZE];
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

	// Wait to receive packet
	state WAIT_PKT:
		packet = tcv_rnp(WAIT_PKT, sfd);

	// Received packet
	state RECEIVED_PKT:
		byte *type = (byte*)(packet+2);

		// Check type of packet
		switch (*type) {
			case DIS_REQ: 	proceed DISCOVER_REQ_START;
			case DIS_RES: 	proceed DISCOVER_RES_START;
			case NEW_REC: 	proceed WAIT_PKT;	//todo
			case DEL_REC: 	proceed WAIT_PKT;	//todo
			case RET_REC: 	proceed WAIT_PKT;	//todo
			case RES_MSG: 	proceed WAIT_PKT;	//todo
			default: 		proceed WAIT_PKT;
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
		case 'g': case 'G': proceed MENU; 		// todo
		case 'n': case 'N': proceed MENU; 		// todo
		case 'f': case 'F': proceed DISCOVER_START;
		case 'c': case 'C': proceed MENU; 		// todo
		case 'd': case 'D': proceed MENU;		// todo
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