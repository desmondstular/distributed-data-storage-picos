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

	// ### DISCOVER REQUEST PACKET ###
	state DISCOVER_REQ:

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

}