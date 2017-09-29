#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
#include "project2.h"

//Cheat at checksumming using zlib, because rolling your own crypto/hashing stuff is awful
#define checksum(packet) (int)crc32(0, (const Bytef*)&packet, sizeof(struct pkt))
#define corrupt(packet, cksum) (cksum != checksum(packet))
#define ackprop(x) (x == ACK0? 0 : 1) //i am lazy
#define TIME_DELAY 1000
 
/* ***************************************************************************
 ALTERNATING BIT AND GO-BACK-N NETWORK EMULATOR: VERSION 1.1  J.F.Kurose

   This code should be used for unidirectional or bidirectional
   data transfer protocols from A to B and B to A.
   Network properties:
   - one way network delay averages five time units (longer if there
     are other messages in the channel for GBN), but can be larger
   - packets can be corrupted (either the header or the data portion)
     or lost, according to user-defined probabilities
   - packets may be delivered out of order.

   Compile as gcc -g project2.c student2.c -o p2
**********************************************************************/



/********* STUDENTS WRITE THE NEXT SEVEN ROUTINES *********/
/* 
 * The routines you will write are detailed below. As noted above, 
 * such procedures in real-life would be part of the operating system, 
 * and would be called by other procedures in the operating system.  
 * All these routines are in layer 4.
 */

/* 
 * A_output(message), where message is a structure of type msg, containing 
 * data to be sent to the B-side. This routine will be called whenever the 
 * upper layer at the sending side (A) has a message to send. It is the job 
 * of your protocol to insure that the data in such a message is delivered 
 * in-order, and correctly, to the receiving side upper layer.
 */

//Global state variables, because we need to save state between function calls
enum S_STATE {SWAIT0, SACK0, SWAIT1, SACK1} SENDER_STATE = SWAIT0;
enum R_STATE {RWAIT0, RWAIT1} RECEIVER_STATE = RWAIT0;
struct pkt lastPacket;

//Packet transmission queue stuff
struct pktChain {
	struct pktChain* next;
	struct msg packet;
};
typedef struct pktChain pktChain;
pktChain* queueFront = NULL;
pktChain* queueBack = NULL;
unsigned int queueSize = 0;

struct msg getNextMsg(){
	struct msg ret;
	if (queueFront == NULL){
		memset(&ret, 0, sizeof(struct pkt));
		return ret;
	}

	ret = queueFront->packet;
	pktChain* tmp = queueFront;
	queueFront = queueFront->next;
	if (queueFront == NULL)
		queueBack = NULL;
	free(tmp);
	queueSize--;
	return ret;
}

void pushMsg(struct msg packet){
	pktChain* nLink = (pktChain*)malloc(sizeof(pktChain));
	memset(nLink, 0, sizeof(pktChain));
	nLink->packet = packet;
	queueSize++;

	//Empty queue; the first packet and the last packet are the same
	if (queueBack == NULL){
		queueFront = nLink;
		queueBack = nLink;
	}
	else {
		//Non-empty queue; point the last packet at our new packet,
		//then set the last packet pointer to point at the new one
		queueBack->next = nLink;
		queueBack = nLink;
	}
}

/*
 * Process the next packet on the queue.
 */
void A_sendNextMsg(){
	if ((SENDER_STATE == SWAIT0 || SENDER_STATE == SWAIT1) && queueSize > 0) { //Idiot check, because the person who wrote this code is an idiot
		struct pkt packet;
		memset(&packet, 0, sizeof(packet));
		packet.acknum = 0;
		packet.seqnum = (SENDER_STATE == SWAIT0 ? 0 : 1);
		strcpy(packet.payload, getNextMsg().data);
		packet.checksum = checksum(packet);
		lastPacket = packet;
		tolayer3(AEntity, packet);
		SENDER_STATE = (SENDER_STATE == RWAIT0 ? SACK0 : SACK1);
		startTimer(AEntity, TIME_DELAY);
	}
}

void A_output(struct msg message) {
	pushMsg(message);
	if (SENDER_STATE == SWAIT0 || SENDER_STATE == SWAIT1)
		A_sendNextMsg();
}

/*
 * Just like A_output, but residing on the B side.  USED only when the 
 * implementation is bi-directional.
 */
void B_output(struct msg message)  {

}

/* 
 * A_input(packet), where packet is a structure of type pkt. This routine 
 * will be called whenever a packet sent from the B-side (i.e., as a result 
 * of a tolayer3() being done by a B-side procedure) arrives at the A-side. 
 * packet is the (possibly corrupted) packet sent from the B-side.
 */
void A_input(struct pkt packet) {
	if (SENDER_STATE == SACK0 || SENDER_STATE == SACK1){
		//This packet *should* be an ACK
		if (!packet.acknum)
			return;

		//Verify the checksum matches. The original checksum was calculated with packet.checksum = 0, so we have to
		//do the same trick here.
		const int checksum = packet.checksum;
		packet.checksum = 0;
		if (corrupt(packet, checksum))
			return;

		//Verify that we have the right ACK number, which is stored in the sequence number. This is probably a bad idea.
		if (packet.seqnum != (SENDER_STATE == SACK0 ? 0 : 1))
			return;

		//Now we've confirmed that we've got an ACK, it had the right number, and the packet wasn't corrupted. Stop
		//the timer and advance to the next state, whatever that might be.
		stopTimer(AEntity);
		SENDER_STATE = (SENDER_STATE == SACK0 ? SWAIT1 : SWAIT0);
		if (queueSize > 0)
			A_sendNextMsg();
	}
}

/*
 * A_timerinterrupt()  This routine will be called when A's timer expires 
 * (thus generating a timer interrupt). You'll probably want to use this 
 * routine to control the retransmission of packets. See starttimer() 
 * and stoptimer() in the writeup for how the timer is started and stopped.
 */
void A_timerinterrupt() {
	//Retransmit the last packet.
	tolayer3(AEntity, lastPacket);
	stopTimer(AEntity); //Is this needed? No idea. Do I care? Not really.
	startTimer(AEntity, TIME_DELAY);
}  

/* The following routine will be called once (only) before any other    */
/* entity A routines are called. You can use it to do any initialization */
void A_init() {
}


/* 
 * Note that with simplex transfer from A-to-B, there is no routine  B_output() 
 */

/*
 * B_input(packet),where packet is a structure of type pkt. This routine 
 * will be called whenever a packet sent from the A-side (i.e., as a result 
 * of a tolayer3() being done by a A-side procedure) arrives at the B-side. 
 * packet is the (possibly corrupted) packet sent from the A-side.
 */
void B_input(struct pkt packet) {
	//Verify that the packet isn't corrupted and verify that it has the right ACK number.
	const int checksum = packet.checksum;
	packet.checksum = 0;
	if (corrupt(packet, checksum)){
		struct pkt badAck;
		memset(&badAck, 0, sizeof(struct pkt));
		badAck.acknum = 1;
		badAck.seqnum = (RECEIVER_STATE == RWAIT0 ? 1 : 0);
		badAck.checksum = checksum(badAck);
		tolayer3(BEntity, badAck);
		return;
	}
	if (packet.seqnum != (RECEIVER_STATE == RWAIT0 ? 0 : 1)){
		struct pkt badAck;
		memset(&badAck, 0, sizeof(struct pkt));
		badAck.acknum = 1;
		badAck.seqnum = (RECEIVER_STATE == RWAIT0 ? 1 : 0);
		badAck.checksum = checksum(badAck);
		tolayer3(BEntity, badAck);
		return;
	}

	//Valid packet confirmed. Send off the data to layer 5, and send an appropriate ACK.
	struct msg message;
	strcpy(message.data, packet.payload);
	tolayer5(BEntity, message);
	struct pkt ack;
	memset(&ack, 0, sizeof(ack));
	ack.acknum = 1;
	ack.seqnum = (RECEIVER_STATE == RWAIT0 ? 0 : 1);
	ack.checksum = checksum(ack);
	tolayer3(BEntity, ack);
	RECEIVER_STATE = (RECEIVER_STATE == RWAIT0 ? RWAIT1 : RWAIT0);
}

/*
 * B_timerinterrupt()  This routine will be called when B's timer expires 
 * (thus generating a timer interrupt). You'll probably want to use this 
 * routine to control the retransmission of packets. See starttimer() 
 * and stoptimer() in the writeup for how the timer is started and stopped.
 */
void  B_timerinterrupt() {
}

/* 
 * The following routine will be called once (only) before any other   
 * entity B routines are called. You can use it to do any initialization 
 */
void B_init() {
}

