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

/*
 * Receiver fsm: Waits for packets from other
 * nodes and then processes them.
*/
fsm receiver
{
	address packet;

	// Wait to receive packet
	state WAIT_PKT:
		packet = tcv_rnp(WAIT_PKT, sfd);

	// Received packet
	state RECEIVED_PKT:
		byte* type = (byte*)(packet+2);

		// Check type of packet
		switch (*type) {
			case DIS_REQ: 	proceed WAIT_PKT;	//todo
			case DIS_RES: 	proceed WAIT_PKT;	//todo
			case NEW_REC: 	proceed WAIT_PKT;	//todo
			case DEL_REC: 	proceed WAIT_PKT;	//todo
			case RET_REC: 	proceed WAIT_PKT;	//todo
			case RES_MSG: 	proceed WAIT_PKT;	//todo
			default: 		proceed WAIT_PKT;
		}
}

/*
 * Root fsm that acts as the user interface.
 */
fsm root
{
	int c;
	address packet;
	struct discoveryMsg msg;

	// Initialize node
	state INIT_SESSION : phys_cc1350(0, PACKET_LEN);

	tcv_plug(0, &plug_null);
	sfd = tcv_open(NONE, 0, 0);

	// If session did not open
	if (sfd < 0)
	{
		diag("unable to open TCV session");
		syserror(EASSERT, "no session");
	}

	tcv_control(sfd, PHYSOPT_ON, NULL);

	// Show sender menu and get input
	state MENU :
		ser_outf(MENU, "\r\nGroup %u Device #%u (%u/%u records)\r\n(G)roup ID\r\n(N)ew device ID\n\r(F)ind neighbors\r\n(C)reate record on neighbor\r\n(D)elete record on neighbor\r\n(R)etrieve record from neighbor\r\n(S)how local records\r\nR(e)set local storage\r\n\r\nSelection: ", groupID, nodeID, entryCount, DATABASE_SIZE);

	// Get user input
	state MENU_INPUT :
		ser_inf(MENU_INPUT, "%c", &c);

	switch (c)
	{
		case 'g': case 'G': proceed MENU; 		// todo
		case 'n': case 'N': proceed MENU; 		// todo
		case 'f': case 'F': proceed MENU; 		// todo
		case 'c': case 'C': proceed MENU; 		// todo
		case 'd': case 'D': proceed MENU; 		// todo
		case 'r': case 'R': proceed MENU; 		// todo
		case 's': case 'S': proceed MENU; 		// todo
		case 'e': case 'E': proceed MENU; 		// todo
		default: 			proceed MENU;
	}
}
