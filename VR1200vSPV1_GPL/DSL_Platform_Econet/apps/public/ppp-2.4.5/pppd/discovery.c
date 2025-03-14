/***********************************************************************
*
* discovery.c
*
* Perform PPPoE discovery
*
* Copyright (C) 1999 by Roaring Penguin Software Inc.
*
***********************************************************************/

static char const RCSID[] =
"$Id: discovery.c,v 1.6 2008/06/15 04:35:50 paulus Exp $";

#define _GNU_SOURCE 1
#include "pppoe.h"
#include "pppd.h"

#include <string.h>
#include <stdlib.h>
#include <errno.h>

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#ifdef HAVE_SYS_UIO_H
#include <sys/uio.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef USE_LINUX_PACKET
#include <sys/ioctl.h>
#include <fcntl.h>
#endif

#include <signal.h>

/* Added by Yang Caiyong, 11-Dec-28.
 * For cmm log definition.
 */ 
#ifdef TP_CMM_LOG
#include <os_log.h>

#define MAX_LOG_LEN 256

struct buffer_info bufinfo;
char buf[MAX_LOG_LEN];

/* 每次启动pppd进程，都尝试发PADT包终结上次的session. Added by Wang Jianfeng 2014-05-05*/
unsigned char g_peerETH[6] = {0};
unsigned int g_sessionID;
extern unsigned int oldSession;
extern unsigned char pppHostUniq[16];

#endif /* TP_CMM_LOG */
/* Ended by Yang Caiyong, 11-Dec-28. */

/**********************************************************************
*%FUNCTION: parseForHostUniq
*%ARGUMENTS:
* type -- tag type
* len -- tag length
* data -- tag data.
* extra -- user-supplied pointer.  This is assumed to be a pointer to int.
*%RETURNS:
* Nothing
*%DESCRIPTION:
* If a HostUnique tag is found which matches our PID, sets *extra to 1.
***********************************************************************/
static void
parseForHostUniq(UINT16_t type, UINT16_t len, unsigned char *data,
		 void *extra)
{
    	int *val = (int *) extra;
 
	if (pppHostUniq[0] != 0)
	{
		if (type == TAG_HOST_UNIQ && len == strlen(pppHostUniq) && (strncmp(pppHostUniq, data, len) == 0))
		{
			*val = 1;
		}
	}
	else
	{
	    if (type == TAG_HOST_UNIQ && len == sizeof(pid_t)) {
			pid_t tmp;
			memcpy(&tmp, data, len);
			if (tmp == getpid()) {
			    *val = 1;
			}
	    }
	}
}

/**********************************************************************
*%FUNCTION: packetIsForMe
*%ARGUMENTS:
* conn -- PPPoE connection info
* packet -- a received PPPoE packet
*%RETURNS:
* 1 if packet is for this PPPoE daemon; 0 otherwise.
*%DESCRIPTION:
* If we are using the Host-Unique tag, verifies that packet contains
* our unique identifier.
***********************************************************************/
static int
packetIsForMe(PPPoEConnection *conn, PPPoEPacket *packet)
{
    int forMe = 0;

    /* If packet is not directed to our MAC address, forget it */
    if (memcmp(packet->ethHdr.h_dest, conn->myEth, ETH_ALEN)) return 0;

    /* If we're not using the Host-Unique tag, then accept the packet */
    if (!conn->useHostUniq) return 1;

    parsePacket(packet, parseForHostUniq, &forMe);
    return forMe;
}

/**********************************************************************
*%FUNCTION: parsePADOTags
*%ARGUMENTS:
* type -- tag type
* len -- tag length
* data -- tag data
* extra -- extra user data.  Should point to a PacketCriteria structure
*          which gets filled in according to selected AC name and service
*          name.
*%RETURNS:
* Nothing
*%DESCRIPTION:
* Picks interesting tags out of a PADO packet
***********************************************************************/
static void
parsePADOTags(UINT16_t type, UINT16_t len, unsigned char *data,
	      void *extra)
{
    struct PacketCriteria *pc = (struct PacketCriteria *) extra;
    PPPoEConnection *conn = pc->conn;
    int i;

    switch(type) {
    case TAG_AC_NAME:
	pc->seenACName = 1;
	if (conn->printACNames) {
	    info("Access-Concentrator: %.*s", (int) len, data);
	}
	if (conn->acName && len == strlen(conn->acName) &&
	    !strncmp((char *) data, conn->acName, len)) {
	    pc->acNameOK = 1;
	}
	break;
    case TAG_SERVICE_NAME:
	pc->seenServiceName = 1;
	if (conn->serviceName && len == strlen(conn->serviceName) &&
	    !strncmp((char *) data, conn->serviceName, len)) {
	    pc->serviceNameOK = 1;
	}
	break;
    case TAG_AC_COOKIE:
	conn->cookie.type = htons(type);
	conn->cookie.length = htons(len);
	memcpy(conn->cookie.payload, data, len);
	break;
    case TAG_RELAY_SESSION_ID:
	conn->relayId.type = htons(type);
	conn->relayId.length = htons(len);
	memcpy(conn->relayId.payload, data, len);
	break;
    case TAG_SERVICE_NAME_ERROR:
 	/* Added by Yang Caiyong, 11-Dec-28.
 	 * For log info.
 	 */ 
	log_to_cmm(LOG_ERROR, "PADO: Service-Name-Error: %.*s", (int) len, data);
 	/* Ended by Yang Caiyong, 11-Dec-28. */
	error("PADO: Service-Name-Error: %.*s", (int) len, data);
	conn->error = 1;
	break;
    case TAG_AC_SYSTEM_ERROR:
	/* Added by Yang Caiyong, 11-Dec-28.
 	 * For log info.
 	 */ 
	log_to_cmm(LOG_ERROR, "PADO: System-Error: %.*s", (int) len, data);
 	/* Ended by Yang Caiyong, 11-Dec-28. */
	error("PADO: System-Error: %.*s", (int) len, data);
	conn->error = 1;
	break;
    case TAG_GENERIC_ERROR:
	/* Added by Yang Caiyong, 11-Dec-28.
 	 * For log info.
 	 */ 
	log_to_cmm(LOG_ERROR, "PADO: Generic-Error: %.*s", (int) len, data);
 	/* Ended by Yang Caiyong, 11-Dec-28. */
	error("PADO: Generic-Error: %.*s", (int) len, data);
	conn->error = 1;
	break;
    }
}

/**********************************************************************
*%FUNCTION: parsePADSTags
*%ARGUMENTS:
* type -- tag type
* len -- tag length
* data -- tag data
* extra -- extra user data (pointer to PPPoEConnection structure)
*%RETURNS:
* Nothing
*%DESCRIPTION:
* Picks interesting tags out of a PADS packet
***********************************************************************/
static void
parsePADSTags(UINT16_t type, UINT16_t len, unsigned char *data,
	      void *extra)
{
    PPPoEConnection *conn = (PPPoEConnection *) extra;
    switch(type) {
    case TAG_SERVICE_NAME:
	dbglog("PADS: Service-Name: '%.*s'", (int) len, data);
	break;
    case TAG_SERVICE_NAME_ERROR:
	/* Added by Yang Caiyong, 11-Dec-28.
 	 * For log info.
 	 */ 
	log_to_cmm(LOG_ERROR, "PADS: Service-Name-Error: %.*s", (int) len, data);
 	/* Ended by Yang Caiyong, 11-Dec-28. */
	error("PADS: Service-Name-Error: %.*s", (int) len, data);
	conn->error = 1;
	break;
    case TAG_AC_SYSTEM_ERROR:
	/* Added by Yang Caiyong, 11-Dec-28.
 	 * For log info.
 	 */ 
	log_to_cmm(LOG_ERROR, "PADS: System-Error: %.*s", (int) len, data);
 	/* Ended by Yang Caiyong, 11-Dec-28. */
	error("PADS: System-Error: %.*s", (int) len, data);
	conn->error = 1;
	break;
    case TAG_GENERIC_ERROR:
	/* Added by Yang Caiyong, 11-Dec-28.
 	 * For log info.
 	 */ 
	log_to_cmm(LOG_ERROR, "PADS: Generic-Error: %.*s", (int) len, data);
 	/* Ended by Yang Caiyong, 11-Dec-28. */
	error("PADS: Generic-Error: %.*s", (int) len, data);
	conn->error = 1;
	break;
    case TAG_RELAY_SESSION_ID:
	conn->relayId.type = htons(type);
	conn->relayId.length = htons(len);
	memcpy(conn->relayId.payload, data, len);
	break;
    }
}

/***********************************************************************
*%FUNCTION: sendPADI
*%ARGUMENTS:
* conn -- PPPoEConnection structure
*%RETURNS:
* Nothing
*%DESCRIPTION:
* Sends a PADI packet
***********************************************************************/
static void
sendPADI(PPPoEConnection *conn)
{
    PPPoEPacket packet;
    unsigned char *cursor = packet.payload;
    PPPoETag *svc = (PPPoETag *) (&packet.payload);
    UINT16_t namelen = 0;
    UINT16_t plen;
    int omit_service_name = 0;
	pid_t pid;
   	UINT16_t hostUniqLen = 0;
	
    if (conn->serviceName) {
	namelen = (UINT16_t) strlen(conn->serviceName);
	if (!strcmp(conn->serviceName, "NO-SERVICE-NAME-NON-RFC-COMPLIANT")) {
	    omit_service_name = 1;
	}
    }

    /* Set destination to Ethernet broadcast address */
    memset(packet.ethHdr.h_dest, 0xFF, ETH_ALEN);
    memcpy(packet.ethHdr.h_source, conn->myEth, ETH_ALEN);

    packet.ethHdr.h_proto = htons(Eth_PPPOE_Discovery);
    packet.vertype = PPPOE_VER_TYPE(1, 1);
    packet.code = CODE_PADI;
    packet.session = 0;

    if (!omit_service_name) {
	plen = TAG_HDR_SIZE + namelen;
	CHECK_ROOM(cursor, packet.payload, plen);

	svc->type = TAG_SERVICE_NAME;
	svc->length = htons(namelen);

	if (conn->serviceName) {
	    memcpy(svc->payload, conn->serviceName, strlen(conn->serviceName));
	}
	cursor += namelen + TAG_HDR_SIZE;
    } else {
	plen = 0;
    }

    /* If we're using Host-Uniq, copy it over */
    if (conn->useHostUniq) {
	PPPoETag hostUniq;
	if (pppHostUniq[0] != 0)
	{
		hostUniqLen = strlen(pppHostUniq);
		hostUniq.type = htons(TAG_HOST_UNIQ);
		hostUniq.length = htons(hostUniqLen);
		memcpy(hostUniq.payload, pppHostUniq, hostUniqLen);
		memcpy(cursor, &hostUniq, hostUniqLen + TAG_HDR_SIZE);
		cursor += hostUniqLen + TAG_HDR_SIZE;
		plen += hostUniqLen + TAG_HDR_SIZE;
	}
	else
	{
		pid = getpid();
		hostUniq.type = htons(TAG_HOST_UNIQ);
		hostUniq.length = htons(sizeof(pid));
		memcpy(hostUniq.payload, &pid, sizeof(pid));
		CHECK_ROOM(cursor, packet.payload, sizeof(pid) + TAG_HDR_SIZE);
		memcpy(cursor, &hostUniq, sizeof(pid) + TAG_HDR_SIZE);
		cursor += sizeof(pid) + TAG_HDR_SIZE;
		plen += sizeof(pid) + TAG_HDR_SIZE;
	}
    }

    packet.length = htons(plen);

    sendPacket(conn, conn->discoverySocket, &packet, (int) (plen + HDR_SIZE));

	//add by yangxv, 2007.12.10
	memset(buf, 0, MAX_LOG_LEN);
	bufinfo.ptr = buf;
	bufinfo.len = MAX_LOG_LEN;
	
	vslp_printer(&bufinfo, "sent [PADI");
	if (conn->serviceName)
	{
		vslp_printer(&bufinfo, " Service-Name:%s", conn->serviceName);
	}
	if (conn->useHostUniq)
	{
		vslp_printer(&bufinfo, " Host-Uniq(0x%08x)", pid);
	}
	vslp_printer(&bufinfo, "]");
	log_to_cmm(LOG_INFORM, "%s", buf);
}

/**********************************************************************
*%FUNCTION: waitForPADO
*%ARGUMENTS:
* conn -- PPPoEConnection structure
* timeout -- how long to wait (in seconds)
*%RETURNS:
* Nothing
*%DESCRIPTION:
* Waits for a PADO packet and copies useful information
***********************************************************************/
void
waitForPADO(PPPoEConnection *conn, int timeout)
{
    fd_set readable;
    int r;
    struct timeval tv;
    PPPoEPacket packet;
    int len;

    struct PacketCriteria pc;
    pc.conn          = conn;
    pc.acNameOK      = (conn->acName)      ? 0 : 1;
    pc.serviceNameOK = (conn->serviceName) ? 0 : 1;
    pc.seenACName    = 0;
    pc.seenServiceName = 0;
    conn->error = 0;

	/* 将设置定时器的操作提到do...while循环外面，避免在收不到正确的PADO，又一直收到其他杂包的情况下，
	 * 定时器不断被刷新，导致死循环。杂包有可能是其他DUT发出来的PADI广播包。
	 * by Wang HaoBin 2014-05-16.
	 */
	tv.tv_sec = timeout;
	tv.tv_usec = 0;

    do {
	if (BPF_BUFFER_IS_EMPTY) {

	    FD_ZERO(&readable);
	    FD_SET(conn->discoverySocket, &readable);

	    while(1) {
		r = select(conn->discoverySocket+1, &readable, NULL, NULL, &tv);
		if (r >= 0 || errno != EINTR) break;
	    }
	    if (r < 0) {
		log_to_cmm(LOG_NOTICE, "select (waitForPADO errno = %d)", errno);
		error("select (waitForPADO): %m");
		return;
	    }
	    if (r == 0) return;        /* Timed out */
	}

	/* Get the packet */
	receivePacket(conn->discoverySocket, &packet, &len);

	/* Check length */
	if (ntohs(packet.length) + HDR_SIZE > len) {
		log_to_cmm(LOG_NOTICE, "Bogus PPPoE length field (%u)", (unsigned int) ntohs(packet.length));
	    error("Bogus PPPoE length field (%u)",
		   (unsigned int) ntohs(packet.length));
	    continue;
	}

#ifdef USE_BPF
	/* If it's not a Discovery packet, loop again */
	if (etherType(&packet) != Eth_PPPOE_Discovery) continue;
#endif

	/* If it's not for us, loop again */
	if (!packetIsForMe(conn, &packet)) continue;

	if (packet.code == CODE_PADO) {
	    if (NOT_UNICAST(packet.ethHdr.h_source)) {
		log_to_cmm(LOG_NOTICE, "Ignoring PADO packet from non-unicast MAC address");
		error("Ignoring PADO packet from non-unicast MAC address");
		continue;
	    }
	    if (conn->req_peer
		&& memcmp(packet.ethHdr.h_source, conn->req_peer_mac, ETH_ALEN) != 0) {
		log_to_cmm(LOG_NOTICE, "Ignoring PADO packet from wrong MAC address");
		warn("Ignoring PADO packet from wrong MAC address");
		continue;
	    }
	    if (parsePacket(&packet, parsePADOTags, &pc) < 0)
	    {
			log_to_cmm(LOG_NOTICE, "parse PADO failure.");
			return;
	    }
	    if (conn->error)
	    {
			log_to_cmm(LOG_NOTICE, "received error PADO packet.");
			return;
	    }
	    if (!pc.seenACName) {
		log_to_cmm(LOG_NOTICE, "Ignoring PADO packet with no AC-Name tag");
		error("Ignoring PADO packet with no AC-Name tag");
		continue;
	    }
	    if (!pc.seenServiceName) {
		log_to_cmm(LOG_NOTICE, "Ignoring PADO packet with no Service-Name tag");
		error("Ignoring PADO packet with no Service-Name tag");
		continue;
	    }
	    conn->numPADOs++;
	    if (pc.acNameOK && pc.serviceNameOK) {
		memcpy(conn->peerEth, packet.ethHdr.h_source, ETH_ALEN);
		conn->discoveryState = STATE_RECEIVED_PADO;
		log_to_cmm(LOG_INFORM, "rcvd  [PADO PeerMac(%02x-%02x-%02x-%02x-%02x-%02x)]", 
					conn->peerEth[0], conn->peerEth[1], conn->peerEth[2], 
					conn->peerEth[3], conn->peerEth[4], conn->peerEth[5]);
		break;
	    }
	}
    } while (conn->discoveryState != STATE_RECEIVED_PADO);
}

/***********************************************************************
*%FUNCTION: sendPADR
*%ARGUMENTS:
* conn -- PPPoE connection structur
*%RETURNS:
* Nothing
*%DESCRIPTION:
* Sends a PADR packet
***********************************************************************/
static void
sendPADR(PPPoEConnection *conn)
{
    PPPoEPacket packet;
    PPPoETag *svc = (PPPoETag *) packet.payload;
    unsigned char *cursor = packet.payload;

    UINT16_t namelen = 0;
    UINT16_t plen;
    UINT16_t hostUniqLen = 0;

    if (conn->serviceName) {
	namelen = (UINT16_t) strlen(conn->serviceName);
    }
    plen = TAG_HDR_SIZE + namelen;
    CHECK_ROOM(cursor, packet.payload, plen);

    memcpy(packet.ethHdr.h_dest, conn->peerEth, ETH_ALEN);
    memcpy(packet.ethHdr.h_source, conn->myEth, ETH_ALEN);

    packet.ethHdr.h_proto = htons(Eth_PPPOE_Discovery);
    packet.vertype = PPPOE_VER_TYPE(1, 1);
    packet.code = CODE_PADR;
    packet.session = 0;

    svc->type = TAG_SERVICE_NAME;
    svc->length = htons(namelen);
    if (conn->serviceName) {
	memcpy(svc->payload, conn->serviceName, namelen);
    }
    cursor += namelen + TAG_HDR_SIZE;

    /* If we're using Host-Uniq, copy it over */
    if (conn->useHostUniq) {
	PPPoETag hostUniq;
	if (pppHostUniq[0] != 0)
	{
		hostUniqLen = strlen(pppHostUniq);
		hostUniq.type = htons(TAG_HOST_UNIQ);
		hostUniq.length = htons(hostUniqLen);
		memcpy(hostUniq.payload, pppHostUniq, hostUniqLen);
		memcpy(cursor, &hostUniq, hostUniqLen + TAG_HDR_SIZE);
		cursor += hostUniqLen + TAG_HDR_SIZE;
		plen += hostUniqLen + TAG_HDR_SIZE;
	}
	else
	{
		pid_t pid = getpid();
		hostUniq.type = htons(TAG_HOST_UNIQ);
		hostUniq.length = htons(sizeof(pid));
		memcpy(hostUniq.payload, &pid, sizeof(pid));
		CHECK_ROOM(cursor, packet.payload, sizeof(pid)+TAG_HDR_SIZE);
		memcpy(cursor, &hostUniq, sizeof(pid) + TAG_HDR_SIZE);
		cursor += sizeof(pid) + TAG_HDR_SIZE;
		plen += sizeof(pid) + TAG_HDR_SIZE;
	}
    }

    /* Copy cookie and relay-ID if needed */
    if (conn->cookie.type) {
	CHECK_ROOM(cursor, packet.payload,
		   ntohs(conn->cookie.length) + TAG_HDR_SIZE);
	memcpy(cursor, &conn->cookie, ntohs(conn->cookie.length) + TAG_HDR_SIZE);
	cursor += ntohs(conn->cookie.length) + TAG_HDR_SIZE;
	plen += ntohs(conn->cookie.length) + TAG_HDR_SIZE;
    }

    if (conn->relayId.type) {
	CHECK_ROOM(cursor, packet.payload,
		   ntohs(conn->relayId.length) + TAG_HDR_SIZE);
	memcpy(cursor, &conn->relayId, ntohs(conn->relayId.length) + TAG_HDR_SIZE);
	cursor += ntohs(conn->relayId.length) + TAG_HDR_SIZE;
	plen += ntohs(conn->relayId.length) + TAG_HDR_SIZE;
    }

    packet.length = htons(plen);
    sendPacket(conn, conn->discoverySocket, &packet, (int) (plen + HDR_SIZE));

	memset(buf, 0, MAX_LOG_LEN);
	bufinfo.ptr = buf;
	bufinfo.len = MAX_LOG_LEN;
	
	vslp_printer(&bufinfo, "sent [PADR");
	if (conn->serviceName)
	{
		vslp_printer(&bufinfo, " Service-Name:%s", conn->serviceName);
	}
	if (conn->useHostUniq)
	{
		vslp_printer(&bufinfo, " Host-Uniq(0x%08x)", getpid());
	}
	vslp_printer(&bufinfo, "]");
	log_to_cmm(LOG_INFORM, "%s", buf);
}

/**********************************************************************
*%FUNCTION: waitForPADS
*%ARGUMENTS:
* conn -- PPPoE connection info
* timeout -- how long to wait (in seconds)
*%RETURNS:
* Nothing
*%DESCRIPTION:
* Waits for a PADS packet and copies useful information
***********************************************************************/
static void
waitForPADS(PPPoEConnection *conn, int timeout)
{
    fd_set readable;
    int r;
    struct timeval tv;
    PPPoEPacket packet;
    int len;

    conn->error = 0;

	/* 将设置定时器的操作提到do...while循环外面，避免在收不到正确的PADS，又一直收到其他杂包的情况下，
	 * 定时器不断被刷新，导致死循环。杂包有可能是其他DUT发出来的PADI广播包。
	 * by Wang HaoBin 2014-05-16.
	 */
	tv.tv_sec = timeout;
	tv.tv_usec = 0;
	
    do {
	if (BPF_BUFFER_IS_EMPTY) {

	    FD_ZERO(&readable);
	    FD_SET(conn->discoverySocket, &readable);

	    while(1) {
		r = select(conn->discoverySocket+1, &readable, NULL, NULL, &tv);
		if (r >= 0 || errno != EINTR) break;
	    }
	    if (r < 0) {
		log_to_cmm(LOG_NOTICE, "select (waitForPADS errno = %d)", errno);
		error("select (waitForPADS): %m");
		return;
	    }
	    if (r == 0) return;
	}

	/* Get the packet */
	receivePacket(conn->discoverySocket, &packet, &len);

	/* Check length */
	if (ntohs(packet.length) + HDR_SIZE > len) {
		log_to_cmm(LOG_NOTICE, "Bogus PPPoE length field (%u)", (unsigned int) ntohs(packet.length));
	    error("Bogus PPPoE length field (%u)",
		   (unsigned int) ntohs(packet.length));
	    continue;
	}

#ifdef USE_BPF
	/* If it's not a Discovery packet, loop again */
	if (etherType(&packet) != Eth_PPPOE_Discovery) continue;
#endif

	/* If it's not from the AC, it's not for me */
	if (memcmp(packet.ethHdr.h_source, conn->peerEth, ETH_ALEN)) continue;

	/* If it's not for us, loop again */
	if (!packetIsForMe(conn, &packet)) continue;

	/* Is it PADS?  */
	if (packet.code == CODE_PADS) {
	    /* Parse for goodies */
	    if (parsePacket(&packet, parsePADSTags, conn) < 0)
	    {
			log_to_cmm(LOG_NOTICE, "parse PADS failure.");
			return;
	    }
	    if (conn->error)
	    {
			log_to_cmm(LOG_NOTICE, "received PADS error packet.");
			return;
	    }
	    conn->discoveryState = STATE_SESSION;
		log_to_cmm(LOG_INFORM, "rcvd  [PADS SessionID(0x%04x)]", packet.session);
	    break;
	}
    } while (conn->discoveryState != STATE_SESSION);

    /* Don't bother with ntohs; we'll just end up converting it back... */
    conn->session = packet.session;
	/* 保存当前的session ID和server的MAC地址. Added by Wang Jianfeng 2014-05-06*/
	g_sessionID = packet.session;
	memcpy(g_peerETH, conn->peerEth, ETH_ALEN);

    info("PPP session is %d", (int) ntohs(conn->session));

    /* RFC 2516 says session id MUST NOT be zero or 0xFFFF */
    if (ntohs(conn->session) == 0 || ntohs(conn->session) == 0xFFFF) {
	error("Access concentrator used a session value of %x -- the AC is violating RFC 2516", (unsigned int) ntohs(conn->session));
    }
}

/**********************************************************************
*%FUNCTION: discovery
*%ARGUMENTS:
* conn -- PPPoE connection info structure
*%RETURNS:
* Nothing
*%DESCRIPTION:
* Performs the PPPoE discovery phase
***********************************************************************/
void
discovery(PPPoEConnection *conn)
{
    int padiAttempts = 0;
    int padrAttempts = 0;
    int timeout = conn->discoveryTimeout;
	int ret;
	discover_stage = 1;
	ret = sigsetjmp(discover_sigjmp, 1);
	if(ret == 1)
	{
		/* pppd收到SIGTERM，立即return，不等待discovery超时，这样能使pppd进程快速退出。*/
		if (conn->discoverySocket >= 0)
		{
			close(conn->discoverySocket);
	    	conn->discoverySocket = -1;
		}
		conn->discoveryState = STATE_SENT_PADI;
		discover_stage = 0;
		return;
	}

    conn->discoverySocket =
	openInterface(conn->ifName, Eth_PPPOE_Discovery, conn->myEth);

	/* 每次启动pppd进程，都尝试发PADT包终结上次的session. Added by Wang Jianfeng 2014-05-05*/
	if (oldSession != 0xffff)
	{
		sendPADT(conn, NULL);
		oldSession = 0xffff;
	}

    do {
	padiAttempts++;
	if (padiAttempts > MAX_PADI_ATTEMPTS) {
	    /*warn("Timeout waiting for PADO packets");*/
	 	/* Added by Yang Caiyong, 11-Dec-28.
	 	 * For cmm log.
	 	 */ 
		if (new_phase_hook)
			(*new_phase_hook)(PHASE_DIS_TIMEOUT);
	    log_to_cmm(LOG_ERROR, "Timeout waiting for PADO packets");
	 	/* Ended by Yang Caiyong, 11-Dec-28. */
	    close(conn->discoverySocket);
	    conn->discoverySocket = -1;
		discover_stage = 0;
	    return;
	}
	sendPADI(conn);
	conn->discoveryState = STATE_SENT_PADI;
	waitForPADO(conn, timeout);

	timeout *= 2;
    } while (conn->discoveryState == STATE_SENT_PADI);

    timeout = conn->discoveryTimeout;
    do {
	padrAttempts++;
	if (padrAttempts > MAX_PADI_ATTEMPTS) {
	    /*warn("Timeout waiting for PADS packets");*/
		/* Added by Yang Caiyong, 11-Dec-28.
	 	 * For cmm log.
	 	 */ 
		if (new_phase_hook)
			(*new_phase_hook)(PHASE_DIS_TIMEOUT);
	    log_to_cmm(LOG_ERROR, "Timeout waiting for PADS packets");
	 	/* Ended by Yang Caiyong, 11-Dec-28. */
	    close(conn->discoverySocket);
	    conn->discoverySocket = -1;
		discover_stage = 0;
	    return;
	}
	sendPADR(conn);
	conn->discoveryState = STATE_SENT_PADR;
	waitForPADS(conn, timeout);
	timeout *= 2;
    } while (conn->discoveryState == STATE_SENT_PADR);

    /* We're done. */
    conn->discoveryState = STATE_SESSION;
	discover_stage = 0;
    return;
}
