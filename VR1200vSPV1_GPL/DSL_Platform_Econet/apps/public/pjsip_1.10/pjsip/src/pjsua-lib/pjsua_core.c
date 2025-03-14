/* $Id: pjsua_core.c 3553 2011-05-05 06:14:19Z nanang $ */
/* 
 * Copyright (C) 2008-2011 Teluu Inc. (http://www.teluu.com)
 * Copyright (C) 2003-2008 Benny Prijono <benny@prijono.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 */
#include <pjsua-lib/pjsua.h>
#include <pjsua-lib/pjsua_internal.h>

/*ycw-pjsip*/
#include "cmsip_assert.h"


#define THIS_FILE   "pjsua_core.c"

#if SUPPORT_STUN
/* Internal prototypes */
static void resolve_stun_entry(pjsua_stun_resolve *sess);
#endif

/* PJSUA application instance. */
struct pjsua_data pjsua_var;


struct pjsua_data* pjsua_get_var(void)
{
    return &pjsua_var;
}


/* Display error */
void pjsua_perror( const char *sender, const char *title, 
			   pj_status_t status)
{
    char errmsg[PJ_ERR_MSG_SIZE];

    pj_strerror(status, errmsg, sizeof(errmsg));
    PJ_LOG(1,(sender, "%s: %s [status=%d]", title, errmsg, status));
}


static void init_data()
{
    unsigned i;

    pj_bzero(&pjsua_var, sizeof(pjsua_var));

    for (i=0; i<PJ_ARRAY_SIZE(pjsua_var.acc); ++i)
    {
		pjsua_var.acc[i].index = i;
    }
    
    for (i=0; i<PJ_ARRAY_SIZE(pjsua_var.tpdata); ++i)
    {
		pjsua_var.tpdata[i].index = i;
    }

#if SUPPORT_STUN
    pjsua_var.stun_status = PJ_EUNKNOWN;
#endif
    pjsua_var.nat_status = PJ_EPENDING;	
#if SUPPORT_STUN
    pj_list_init(&pjsua_var.stun_res);
#endif
/*yuchuwei@2012-04-05: we don't need the global outbound proxy*/
#	if 0
    pj_list_init(&pjsua_var.outbound_proxy);
#	endif

    pjsua_config_default(&pjsua_var.ua_cfg);
}


#if (1 <= PJ_LOG_MAX_LEVEL)
void pjsua_logging_config_default(pjsua_logging_config *cfg)
{
    pj_bzero(cfg, sizeof(*cfg));

    cfg->msg_logging = PJ_TRUE;
    cfg->level = 5;
    cfg->console_level = 4;
    cfg->decor = PJ_LOG_HAS_SENDER | PJ_LOG_HAS_TIME | 
		 PJ_LOG_HAS_MICRO_SEC | PJ_LOG_HAS_NEWLINE |
		 PJ_LOG_HAS_SPACE;
#if defined(PJ_WIN32) && PJ_WIN32 != 0
    cfg->decor |= PJ_LOG_HAS_COLOR;
#endif
}

void pjsua_logging_config_dup(pj_pool_t *pool,
#if (1 <= PJ_LOG_MAX_LEVEL)
				      pjsua_logging_config *dst,
#endif /* (1 <= PJ_LOG_MAX_LEVEL) */
				      const pjsua_logging_config *src)
{
    pj_memcpy(dst, src, sizeof(*src));
    pj_strdup_with_null(pool, &dst->log_filename, &src->log_filename);
}
#endif /* (1 <= PJ_LOG_MAX_LEVEL) */

void pjsua_config_default(pjsua_config *cfg)
{
    pj_bzero(cfg, sizeof(*cfg));

	/*ycw-pjsip*/
	#if 0
    cfg->max_calls = ((PJSUA_MAX_CALLS) < 4) ? (PJSUA_MAX_CALLS) : 4;
	#else
	cfg->max_calls = PJSUA_MAX_CALLS;
	#endif
    cfg->thread_cnt = 1;
    cfg->nat_type_in_sdp = 1;
#if SUPPORT_STUN
    cfg->stun_ignore_failure = PJ_TRUE;
#endif
    cfg->force_lr = PJ_TRUE;
/*ycw-20120420: disable unsolicited mwi*/
#	if 0
   cfg->enable_unsolicited_mwi = PJ_TRUE;
#	else
	cfg->enable_unsolicited_mwi = PJ_FALSE;
#	endif
#if defined(PJMEDIA_HAS_SRTP) && (PJMEDIA_HAS_SRTP != 0)
    cfg->use_srtp = PJSUA_DEFAULT_USE_SRTP;
    cfg->srtp_secure_signaling = PJSUA_DEFAULT_SRTP_SECURE_SIGNALING;
#endif
    cfg->hangup_forked_call = PJ_TRUE;

    cfg->use_timer = PJSUA_SIP_TIMER_OPTIONAL;
    pjsip_timer_setting_default(&cfg->timer_setting);
}

void pjsua_config_dup(pj_pool_t *pool,
			      pjsua_config *dst,
			      const pjsua_config *src)
{
    unsigned i;

    pj_memcpy(dst, src, sizeof(*src));

/*yuchuwei@2012-04-05: we don't need the global outbound proxy*/
#	if 0
    for (i=0; i<src->outbound_proxy_cnt; ++i) {
	pj_strdup_with_null(pool, &dst->outbound_proxy[i],
			    &src->outbound_proxy[i]);
    }
#	endif

    for (i=0; i<src->cred_count; ++i) {
	pjsip_cred_dup(pool, &dst->cred_info[i], &src->cred_info[i]);
    }

    pj_strdup_with_null(pool, &dst->user_agent, &src->user_agent);
#ifdef INCLUDE_PSTN
    pj_strdup_with_null(pool, &dst->stun_domain, &src->stun_domain);
    pj_strdup_with_null(pool, &dst->stun_host, &src->stun_host);

    for (i=0; i<src->stun_srv_cnt; ++i) {
	pj_strdup_with_null(pool, &dst->stun_srv[i], &src->stun_srv[i]);
    }
#endif
}

void pjsua_msg_data_init(pjsua_msg_data *msg_data)
{
    pj_bzero(msg_data, sizeof(*msg_data));
    pj_list_init(&msg_data->hdr_list);
    pjsip_media_type_init(&msg_data->multipart_ctype, NULL, NULL);
    pj_list_init(&msg_data->multipart_parts);
}

void pjsua_transport_config_default(pjsua_transport_config *cfg)
{
    pj_bzero(cfg, sizeof(*cfg));
    pjsip_tls_setting_default(&cfg->tls_setting);
}

void pjsua_transport_config_dup(pj_pool_t *pool,
					pjsua_transport_config *dst,
					const pjsua_transport_config *src)
{
    PJ_UNUSED_ARG(pool);
    pj_memcpy(dst, src, sizeof(*src));
}

void pjsua_acc_config_default(pjsua_acc_config *cfg)
{
    pj_bzero(cfg, sizeof(*cfg));

    cfg->reg_timeout = PJSUA_REG_INTERVAL;
    cfg->reg_delay_before_refresh = PJSIP_REGISTER_CLIENT_DELAY_BEFORE_REFRESH;
    cfg->unreg_timeout = PJSUA_UNREG_TIMEOUT;
    pjsip_publishc_opt_default(&cfg->publish_opt);
    cfg->unpublish_max_wait_time_msec = PJSUA_UNPUBLISH_MAX_WAIT_TIME_MSEC;
    cfg->transport_id = PJSUA_INVALID_ID;
    cfg->allow_contact_rewrite = PJ_TRUE;
    cfg->require_100rel = pjsua_var.ua_cfg.require_100rel;
    cfg->use_timer = pjsua_var.ua_cfg.use_timer;
    cfg->timer_setting = pjsua_var.ua_cfg.timer_setting;
    cfg->ka_interval = 60;
    cfg->ka_data = pj_str("\r\n");
#if defined(PJMEDIA_HAS_SRTP) && (PJMEDIA_HAS_SRTP != 0)
    cfg->use_srtp = pjsua_var.ua_cfg.use_srtp;
    cfg->srtp_secure_signaling = pjsua_var.ua_cfg.srtp_secure_signaling;
    cfg->srtp_optional_dup_offer = pjsua_var.ua_cfg.srtp_optional_dup_offer;
#endif
    cfg->reg_retry_interval = PJSUA_REG_RETRY_INTERVAL;
    cfg->contact_rewrite_method = PJSUA_CONTACT_REWRITE_METHOD;
    cfg->use_rfc5626 = PJ_FALSE;
	 /*ycw-pjsip*/
#	if 0
    cfg->reg_use_proxy = PJSUA_REG_USE_OUTBOUND_PROXY |
			 PJSUA_REG_USE_ACC_PROXY;
#	else
	cfg->reg_use_proxy = 0;
#	endif
	 
#if defined(PJMEDIA_STREAM_ENABLE_KA) && PJMEDIA_STREAM_ENABLE_KA!=0
    cfg->use_stream_ka = (PJMEDIA_STREAM_ENABLE_KA != 0);
#endif
    pj_list_init(&cfg->reg_hdr_list);
    pj_list_init(&cfg->sub_hdr_list);
    cfg->call_hold_type = PJSUA_CALL_HOLD_TYPE_DEFAULT;

	 /*ycw-pjsip*/
	 cfg->cmAcctIndex = -1;

#ifdef INCLUDE_TFC_ES
	cfg->isAuthed = PJ_FALSE;
	cfg->supportNAI = PJ_FALSE;
	cfg->requestPriv = PJ_FALSE;
	cfg->preferId = pj_str("");
#endif
}

#	if defined(SUPPORT_PRESENCE) && SUPPORT_PRESENCE!=0
void pjsua_buddy_config_default(pjsua_buddy_config *cfg)
{
    pj_bzero(cfg, sizeof(*cfg));
}
#	endif
void pjsua_media_config_default(pjsua_media_config *cfg)
{
    pj_bzero(cfg, sizeof(*cfg));

    cfg->clock_rate = PJSUA_DEFAULT_CLOCK_RATE;
    cfg->snd_clock_rate = 0;
    cfg->channel_count = 1;
    cfg->audio_frame_ptime = PJSUA_DEFAULT_AUDIO_FRAME_PTIME;
    cfg->max_media_ports = PJSUA_MAX_CONF_PORTS;
    cfg->has_ioqueue = PJ_TRUE;
    cfg->thread_cnt = 1;
    cfg->quality = PJSUA_DEFAULT_CODEC_QUALITY;
    cfg->ilbc_mode = PJSUA_DEFAULT_ILBC_MODE;
    cfg->ec_tail_len = PJSUA_DEFAULT_EC_TAIL_LEN;
	 #if 0
    cfg->snd_rec_latency = PJMEDIA_SND_DEFAULT_REC_LATENCY;
    cfg->snd_play_latency = PJMEDIA_SND_DEFAULT_PLAY_LATENCY;
	 #endif
    cfg->jb_init = cfg->jb_min_pre = cfg->jb_max_pre = cfg->jb_max = -1;
	 #if 0
    cfg->snd_auto_close_time = 1;
	 #endif
#if	PJSUA_ADD_ICE_TAGS
    cfg->ice_max_host_cands = -1;
    pj_ice_sess_options_default(&cfg->ice_opt);
    cfg->turn_conn_type = PJ_TURN_TP_UDP;
#endif
}


/*****************************************************************************
 * This is a very simple PJSIP module, whose sole purpose is to display
 * incoming and outgoing messages to log. This module will have priority
 * higher than transport layer, which means:
 *
 *  - incoming messages will come to this module first before reaching
 *    transaction layer.
 *
 *  - outgoing messages will come to this module last, after the message
 *    has been 'printed' to contiguous buffer by transport layer and
 *    appropriate transport instance has been decided for this message.
 *
 */

#ifdef CMSIP_DEBUG
/* Notification on incoming messages */
static pj_bool_t logging_on_rx_msg(pjsip_rx_data *rdata)
{
    PJ_LOG(4,(THIS_FILE, "RX %d bytes %s from %s %s:%d:\n"
			 "%.*s\n"
			 "--end msg--",
			 rdata->msg_info.len,
			 pjsip_rx_data_get_info(rdata),
			 rdata->tp_info.transport->type_name,
			 rdata->pkt_info.src_name,
			 rdata->pkt_info.src_port,
			 (int)rdata->msg_info.len,
			 rdata->msg_info.msg_buf));
    
    /* Always return false, otherwise messages will not get processed! */
    return PJ_FALSE;
}

/* Notification on outgoing messages */
static pj_status_t logging_on_tx_msg(pjsip_tx_data *tdata)
{
    
    /* Important note:
     *	tp_info field is only valid after outgoing messages has passed
     *	transport layer. So don't try to access tp_info when the module
     *	has lower priority than transport layer.
     */

    PJ_LOG(4,(THIS_FILE, "TX %d bytes %s to %s %s:%d:\n"
			 "%.*s\n"
			 "--end msg--",
			 (tdata->buf.cur - tdata->buf.start),
			 pjsip_tx_data_get_info(tdata),
			 tdata->tp_info.transport->type_name,
			 tdata->tp_info.dst_name,
			 tdata->tp_info.dst_port,
			 (int)(tdata->buf.cur - tdata->buf.start),
			 tdata->buf.start));

    /* Always return success, otherwise message will not get sent! */
    return PJ_SUCCESS;
}
#endif

/* The module instance. */
static pjsip_module pjsua_msg_logger = 
{
    NULL, NULL,				/* prev, next.		*/
    { "mod-pjsua-log", 13 },		/* Name.		*/
    -1,					/* Id			*/
    PJSIP_MOD_PRIORITY_TRANSPORT_LAYER-1,/* Priority	        */
    NULL,				/* load()		*/
    NULL,				/* start()		*/
    NULL,				/* stop()		*/
    NULL,				/* unload()		*/
	#ifdef CMSIP_DEBUG
    &logging_on_rx_msg,			/* on_rx_request()	*/
    &logging_on_rx_msg,			/* on_rx_response()	*/
    &logging_on_tx_msg,			/* on_tx_request.	*/
    &logging_on_tx_msg,			/* on_tx_response()	*/
    #else
	 NULL,NULL,NULL,NULL,
	 #endif
    NULL,				/* on_tsx_state()	*/

};


/*****************************************************************************
 * Another simple module to handle incoming OPTIONS request
 */

/* Notification on incoming request */
static pj_bool_t options_on_rx_request(pjsip_rx_data *rdata)
{
    pjsip_tx_data *tdata;
    pjsip_response_addr res_addr;
    pjmedia_transport_info tpinfo;
    pjmedia_sdp_session *sdp;
    const pjsip_hdr *cap_hdr;
    pj_status_t status;

    /* Only want to handle OPTIONS requests */
    if (pjsip_method_cmp(&rdata->msg_info.msg->line.req.method,
			 pjsip_get_options_method()) != 0)
    {
	return PJ_FALSE;
    }

    /* Don't want to handle if shutdown is in progress */
    if (pjsua_var.thread_quit_flag) {
	pjsip_endpt_respond_stateless(pjsua_var.endpt, rdata, 
				      PJSIP_SC_TEMPORARILY_UNAVAILABLE, NULL,
				      NULL, NULL);
	return PJ_TRUE;
    }

    /* Create basic response. */
    status = pjsip_endpt_create_response(pjsua_var.endpt, rdata, 200, NULL, 
					 &tdata);
    if (status != PJ_SUCCESS) {
	pjsua_perror(THIS_FILE, "Unable to create OPTIONS response", status);
	return PJ_TRUE;
    }

    /* Add Allow header */
    cap_hdr = pjsip_endpt_get_capability(pjsua_var.endpt, PJSIP_H_ALLOW, NULL);
    if (cap_hdr) {
	pjsip_msg_add_hdr(tdata->msg, 
			  (pjsip_hdr*) pjsip_hdr_clone(tdata->pool, cap_hdr));
    }

    /* Add Accept header */
    cap_hdr = pjsip_endpt_get_capability(pjsua_var.endpt, PJSIP_H_ACCEPT, NULL);
    if (cap_hdr) {
	pjsip_msg_add_hdr(tdata->msg, 
			  (pjsip_hdr*) pjsip_hdr_clone(tdata->pool, cap_hdr));
    }

    /* Add Supported header */
    cap_hdr = pjsip_endpt_get_capability(pjsua_var.endpt, PJSIP_H_SUPPORTED, NULL);
    if (cap_hdr) {
	pjsip_msg_add_hdr(tdata->msg, 
			  (pjsip_hdr*) pjsip_hdr_clone(tdata->pool, cap_hdr));
    }

    /* Add Allow-Events header from the evsub module */
    cap_hdr = pjsip_evsub_get_allow_events_hdr(NULL);
    if (cap_hdr) {
	pjsip_msg_add_hdr(tdata->msg, 
			  (pjsip_hdr*) pjsip_hdr_clone(tdata->pool, cap_hdr));
    }

    /* Add User-Agent header */
    if (pjsua_var.ua_cfg.user_agent.slen) {
	const pj_str_t USER_AGENT = { "User-Agent", 10};
	pjsip_hdr *h;

	h = (pjsip_hdr*) pjsip_generic_string_hdr_create(tdata->pool,
							 &USER_AGENT,
							 &pjsua_var.ua_cfg.user_agent);
	pjsip_msg_add_hdr(tdata->msg, h);
    }

    /* Get media socket info, make sure transport is ready */
    if (pjsua_var.calls[0].med_tp) {
	pjmedia_transport_info_init(&tpinfo);
	pjmedia_transport_get_info(pjsua_var.calls[0].med_tp, &tpinfo);

	/* Add SDP body, using call0's RTP address */
	/*ycw-pjsip-ptime*/
	#if 0
	status = pjmedia_endpt_create_sdp(pjsua_var.med_endpt, tdata->pool, 1,
					  &tpinfo.sock_info, &sdp);
	#else
	/*OPTIONS request want to get the capabilaty of this SIP statck. We need not send ptime's
	value.*/
	status = pjmedia_endpt_create_sdp(pjsua_var.med_endpt, tdata->pool, 1,
					  &tpinfo.sock_info, 0, PJ_FALSE, &sdp);
	#endif
	if (status == PJ_SUCCESS) {
	    pjsip_create_sdp_body(tdata->pool, sdp, &tdata->msg->body);
	}
    }

    /* Send response statelessly */
    pjsip_get_response_addr(tdata->pool, rdata, &res_addr);
    status = pjsip_endpt_send_response(pjsua_var.endpt, &res_addr, tdata, NULL, NULL);
    if (status != PJ_SUCCESS)
	pjsip_tx_data_dec_ref(tdata);

    return PJ_TRUE;
}


/* The module instance. */
static pjsip_module pjsua_options_handler = 
{
    NULL, NULL,				/* prev, next.		*/
    { "mod-pjsua-options", 17 },	/* Name.		*/
    -1,					/* Id			*/
    //PJSIP_MOD_PRIORITY_APPLICATION,	/* Priority	        */
    PJSIP_MOD_PRIORITY_TRANSPORT_LAYER-1,	/* Priority	        */
    NULL,				/* load()		*/
    NULL,				/* start()		*/
    NULL,				/* stop()		*/
    NULL,				/* unload()		*/
    &options_on_rx_request,		/* on_rx_request()	*/
    NULL,				/* on_rx_response()	*/
    NULL,				/* on_tx_request.	*/
    NULL,				/* on_tx_response()	*/
    NULL,				/* on_tsx_state()	*/

};


/*****************************************************************************
 * These two functions are the main callbacks registered to PJSIP stack
 * to receive SIP request and response messages that are outside any
 * dialogs and any transactions.
 */

/*
 * Handler for receiving incoming requests.
 *
 * This handler serves multiple purposes:
 *  - it receives requests outside dialogs.
 *  - it receives requests inside dialogs, when the requests are
 *    unhandled by other dialog usages. Example of these
 *    requests are: MESSAGE.
 */
static pj_bool_t mod_pjsua_on_rx_request(pjsip_rx_data *rdata)
{
    pj_bool_t processed = PJ_FALSE;
    PJSUA_LOCK();

    if (rdata->msg_info.msg->line.req.method.id == PJSIP_INVITE_METHOD)
	 {
		processed = pjsua_call_on_incoming(rdata);
    }

    PJSUA_UNLOCK();

    return processed;
}


/*
 * Handler for receiving incoming responses.
 *
 * This handler serves multiple purposes:
 *  - it receives strayed responses (i.e. outside any dialog and
 *    outside any transactions).
 *  - it receives responses coming to a transaction, when pjsua
 *    module is set as transaction user for the transaction.
 *  - it receives responses inside a dialog, when these responses
 *    are unhandled by other dialog usages.
 */
static pj_bool_t mod_pjsua_on_rx_response(pjsip_rx_data *rdata)
{
    PJ_UNUSED_ARG(rdata);
    return PJ_FALSE;
}


/*****************************************************************************
 * Logging.
 */
#if (1 <= PJ_LOG_MAX_LEVEL)
/* Log callback */
static void log_writer(int level, const char *buffer, int len)
{
    /* Write to file, stdout or application callback. */

    if (pjsua_var.log_file) {
	pj_ssize_t size = len;
	pj_file_write(pjsua_var.log_file, buffer, &size);
	/* This will slow things down considerably! Don't do it!
	 pj_file_flush(pjsua_var.log_file);
	*/
    }

    if (level <= (int)pjsua_var.log_cfg.console_level) {
	if (pjsua_var.log_cfg.cb)
	    (*pjsua_var.log_cfg.cb)(level, buffer, len);
	else
	    pj_log_write(level, buffer, len);
    }
}

/*
 * Application can call this function at any time (after pjsua_create(), of
 * course) to change logging settings.
 */
pj_status_t pjsua_reconfigure_logging(const pjsua_logging_config *cfg)
{
    pj_status_t status;

    /* Save config. */
    pjsua_logging_config_dup(pjsua_var.pool, &pjsua_var.log_cfg, cfg);

    /* Redirect log function to ours */
    pj_log_set_log_func( &log_writer );

    /* Set decor */
    pj_log_set_decor(pjsua_var.log_cfg.decor);

    /* Set log level */
    pj_log_set_level(pjsua_var.log_cfg.level);

    /* Close existing file, if any */
    if (pjsua_var.log_file) {
	pj_file_close(pjsua_var.log_file);
	pjsua_var.log_file = NULL;
    }

    /* If output log file is desired, create the file: */
    if (pjsua_var.log_cfg.log_filename.slen) {
	unsigned flags = PJ_O_WRONLY;
	flags |= pjsua_var.log_cfg.log_file_flags;
	status = pj_file_open(pjsua_var.pool, 
			      pjsua_var.log_cfg.log_filename.ptr,
			      flags, 
			      &pjsua_var.log_file);

	if (status != PJ_SUCCESS) {
	    pjsua_perror(THIS_FILE, "Error creating log file", status);
	    return status;
	}
    }

    /* Unregister msg logging if it's previously registered */
    if (pjsua_msg_logger.id >= 0) {
	pjsip_endpt_unregister_module(pjsua_var.endpt, &pjsua_msg_logger);
	pjsua_msg_logger.id = -1;
    }

    /* Enable SIP message logging */
    if (pjsua_var.log_cfg.msg_logging)
	pjsip_endpt_register_module(pjsua_var.endpt, &pjsua_msg_logger);

    return PJ_SUCCESS;
}
#endif


/*****************************************************************************
 * PJSUA Base API.
 */

/* Worker thread function. */
static int worker_thread(void *arg)
{
    enum { TIMEOUT = 10 };

    PJ_UNUSED_ARG(arg);

    while (!pjsua_var.thread_quit_flag)
	 {
		int count;

		count = pjsua_handle_events(TIMEOUT);
		if (count < 0)
	   {
	   	pj_thread_sleep(TIMEOUT);
		}
    }

    return 0;
}


/* Init random seed */
/*ycw-pjsip*/
#	if 0
static void init_random_seed(void)
#	else
static void init_random_seed(pj_str_t *defaultIp)
#	endif
{
    pj_sockaddr addr;
    const pj_str_t *hostname;
    pj_uint32_t pid;
    pj_time_val t;
    unsigned seed=0;

    /* Add hostname */
    hostname = pj_gethostname();
    seed = pj_hash_calc(seed, hostname->ptr, (int)hostname->slen);

    /* Add primary IP address */
	 /*ycw-pjsip*/
	 if (defaultIp)
	 {
	 	if (pj_inet_pton(pj_AF_INET(), defaultIp, &addr.ipv4.sin_addr)== PJ_SUCCESS)
			seed = pj_hash_calc(seed, &addr.ipv4.sin_addr, 4);
	 }
	 else
	 {
    	if (pj_gethostip(pj_AF_INET(), &addr)==PJ_SUCCESS)
			seed = pj_hash_calc(seed, &addr.ipv4.sin_addr, 4);
	 }

    /* Get timeofday */
    pj_gettimeofday(&t);
    seed = pj_hash_calc(seed, &t, sizeof(t));

    /* Add PID */
    pid = pj_getpid();
    seed = pj_hash_calc(seed, &pid, sizeof(pid));

    /* Init random seed */
    pj_srand(seed);
}

/*
 * Instantiate pjsua application.
 */
pj_status_t pjsua_create(void)
{
    pj_status_t status;

	/*ycw-pjsip: Must create the socket interactive with CM here, because Account
	 register will be performed when initialize PJSIP*/
 	status = cmsip_sockCreate(&g_cmsip_cliSockfd, CMSIP_SOCK_PATH);
	CMSIP_ASSERT(status >= 0);
	if (status < 0)
	{
		pjsua_perror(THIS_FILE, "can not create cmsip socket!!!", status);
		return status;
	}

    /* Init pjsua data */
    init_data();

#if (1 <= PJ_LOG_MAX_LEVEL)
    /* Set default logging settings */
    pjsua_logging_config_default(&pjsua_var.log_cfg);
#endif /* (1 <= PJ_LOG_MAX_LEVEL) */

    /* Init PJLIB: */
    status = pj_init();
    PJ_ASSERT_RETURN(status == PJ_SUCCESS, status);

/*ycw-pjsip*/
#	if 0 
    /* Init random seed */
    init_random_seed();
#	endif

    /* Init PJLIB-UTIL: */
    status = pjlib_util_init();
    PJ_ASSERT_RETURN(status == PJ_SUCCESS, status);
#if SUPPORT_STUN
    /* Init PJNATH */
    status = pjnath_init();
    PJ_ASSERT_RETURN(status == PJ_SUCCESS, status);
#endif /* SUPPORT_STUN */
    /* Set default sound device ID */
	/*ycw-pjsip-20110610-delete sound device*/
	#if 0
    pjsua_var.cap_dev = PJMEDIA_AUD_DEFAULT_CAPTURE_DEV;
    pjsua_var.play_dev = PJMEDIA_AUD_DEFAULT_PLAYBACK_DEV;
	#endif

    /* Init caching pool. */
    pj_caching_pool_init(&pjsua_var.cp, NULL, 0);

    /* Create memory pool for application. */
    pjsua_var.pool = pjsua_pool_create("pjsua", 1000, 1000);
    
    PJ_ASSERT_RETURN(pjsua_var.pool, PJ_ENOMEM);

    /* Create mutex */
    status = pj_mutex_create_recursive(pjsua_var.pool, "pjsua", 
				       &pjsua_var.mutex);
    if (status != PJ_SUCCESS)
	 {
		pjsua_perror(THIS_FILE, "Unable to create mutex", status);
		return status;
    }

    /* Must create SIP endpoint to initialize SIP parser. The parser
     * is needed for example when application needs to call pjsua_verify_url().
     */
    status = pjsip_endpt_create(&pjsua_var.cp.factory, 
				pj_gethostname()->ptr, 
				&pjsua_var.endpt);
    PJ_ASSERT_RETURN(status == PJ_SUCCESS, status);

    return PJ_SUCCESS;
}

/*ycw-pjsip*/
void pjsua_init_random_seed(pj_str_t* defaultIp)
{	
	init_random_seed(defaultIp);
}

/*
 * Initialize pjsua with the specified settings. All the settings are 
 * optional, and the default values will be used when the config is not
 * specified.
 */
pj_status_t pjsua_init( const pjsua_config *ua_cfg,
#if (1 <= PJ_LOG_MAX_LEVEL)
				const pjsua_logging_config *log_cfg,
#endif /* (1 <= PJ_LOG_MAX_LEVEL) */
				const pjsua_media_config *media_cfg)
{
    pjsua_config	 default_cfg;
    pjsua_media_config	 default_media_cfg;
    const pj_str_t	 STR_OPTIONS = { "OPTIONS", 7 };
    pjsip_ua_init_param  ua_init_param;
     pj_status_t status;


    /* Create default configurations when the config is not supplied */

    if (ua_cfg == NULL) {
	pjsua_config_default(&default_cfg);
	ua_cfg = &default_cfg;
    }

    if (media_cfg == NULL) {
	pjsua_media_config_default(&default_media_cfg);
	media_cfg = &default_media_cfg;
    }

#if (1 <= PJ_LOG_MAX_LEVEL)
    /* Initialize logging first so that info/errors can be captured */
    if (log_cfg) {
	status = pjsua_reconfigure_logging(log_cfg);
	if (status != PJ_SUCCESS)
	    return status;
    }
#endif /* (1 <= PJ_LOG_MAX_LEVEL) */

#if defined(PJ_IPHONE_OS_HAS_MULTITASKING_SUPPORT) && \
    PJ_IPHONE_OS_HAS_MULTITASKING_SUPPORT != 0
    if (!(pj_get_sys_info()->flags & PJ_SYS_HAS_IOS_BG)) {
	PJ_LOG(5, (THIS_FILE, "Device does not support "
			      "background mode"));
	pj_activesock_enable_iphone_os_bg(PJ_FALSE);
    }
#endif

/*yuchuwei@2012-04-05: we don't use SIP DNS resolver,instead, we use gethostbyname API only*/
	/*enable this feature, By yuchuwei ,For Telefonica*/
    /* If nameserver is configured, create DNS resolver instance and
     * set it to be used by SIP resolver.
     */
#if PJSIP_HAS_RESOLVER
    if (ua_cfg->nameserver_count) {
		unsigned i;

		/* Create DNS resolver */
		status = pjsip_endpt_create_resolver(pjsua_var.endpt, 
						     &pjsua_var.resolver);
		if (status != PJ_SUCCESS) {
		    pjsua_perror(THIS_FILE, "Error creating resolver", status);
		    return status;
		}

		/* Configure nameserver for the DNS resolver */
		status = pj_dns_resolver_set_ns(pjsua_var.resolver, 
						ua_cfg->nameserver_count,
						ua_cfg->nameserver, NULL);
		if (status != PJ_SUCCESS) {
		    pjsua_perror(THIS_FILE, "Error setting nameserver", status);
		    return status;
		}

		/* Set this DNS resolver to be used by the SIP resolver */
		status = pjsip_endpt_set_resolver(pjsua_var.endpt, pjsua_var.resolver);
		if (status != PJ_SUCCESS) {
		    pjsua_perror(THIS_FILE, "Error setting DNS resolver", status);
		    return status;
		}

		/* Print nameservers */
		for (i=0; i<ua_cfg->nameserver_count; ++i) {
		    PJ_LOG(4,(THIS_FILE, "Nameserver %.*s added",
			      (int)ua_cfg->nameserver[i].slen,
			      ua_cfg->nameserver[i].ptr));
		}
    }
#else
	PJ_LOG(2,(THIS_FILE, 
		  "DNS resolver is disabled (PJSIP_HAS_RESOLVER==0)"));
#endif

    /* Init SIP UA: */
    /* Initialize transaction layer: */
    status = pjsip_tsx_layer_init_module(pjsua_var.endpt);
    PJ_ASSERT_RETURN(status == PJ_SUCCESS, status);


    /* Initialize UA layer module: */
    pj_bzero(&ua_init_param, sizeof(ua_init_param));
    if (ua_cfg->hangup_forked_call) {
	ua_init_param.on_dlg_forked = &on_dlg_forked;
    }
	
#if defined(INCLUDE_TFC_ES) && PJ_RFC3960_SUPPORT
	ua_init_param.mute_dlg_forked = &mute_dlg_forked;
#endif

	
    status = pjsip_ua_init_module( pjsua_var.endpt, &ua_init_param);
    PJ_ASSERT_RETURN(status == PJ_SUCCESS, status);


    /* Initialize Replaces support. */
    status = pjsip_replaces_init_module( pjsua_var.endpt );
    PJ_ASSERT_RETURN(status == PJ_SUCCESS, status);

    /* Initialize 100rel support */
    status = pjsip_100rel_init_module(pjsua_var.endpt);
    PJ_ASSERT_RETURN(status == PJ_SUCCESS, status);

    /* Initialize session timer support */
    status = pjsip_timer_init_module(pjsua_var.endpt);
    PJ_ASSERT_RETURN(status == PJ_SUCCESS, status);
    /* Initialize and register PJSUA application module. */
    {
	const pjsip_module mod_initializer = 
	{
	NULL, NULL,		    /* prev, next.			*/
	{ "mod-pjsua", 9 },	    /* Name.				*/
	-1,			    /* Id				*/
	PJSIP_MOD_PRIORITY_APPLICATION,	/* Priority			*/
	NULL,			    /* load()				*/
	NULL,			    /* start()				*/
	NULL,			    /* stop()				*/
	NULL,			    /* unload()				*/
	&mod_pjsua_on_rx_request,   /* on_rx_request()			*/
	&mod_pjsua_on_rx_response,  /* on_rx_response()			*/
	NULL,			    /* on_tx_request.			*/
	NULL,			    /* on_tx_response()			*/
	NULL,			    /* on_tsx_state()			*/
	};

	pjsua_var.mod = mod_initializer;

	status = pjsip_endpt_register_module(pjsua_var.endpt, &pjsua_var.mod);
	PJ_ASSERT_RETURN(status == PJ_SUCCESS, status);
    }

/*yuchuwei@2012-04-05: we don't use the global outbound proxy*/
#	if 0
    /* Parse outbound proxies */
    for (i=0; i<ua_cfg->outbound_proxy_cnt; ++i) {
	pj_str_t tmp;
    	pj_str_t hname = { "Route", 5};
	pjsip_route_hdr *r;

	pj_strdup_with_null(pjsua_var.pool, &tmp, &ua_cfg->outbound_proxy[i]);

	r = (pjsip_route_hdr*)
	    pjsip_parse_hdr(pjsua_var.pool, &hname, tmp.ptr,
			    (unsigned)tmp.slen, NULL);
	if (r == NULL) {
	    pjsua_perror(THIS_FILE, "Invalid outbound proxy URI",
			 PJSIP_EINVALIDURI);
	    return PJSIP_EINVALIDURI;
	}

	if (pjsua_var.ua_cfg.force_lr) {
	    pjsip_sip_uri *sip_url;
	    if (!PJSIP_URI_SCHEME_IS_SIP(r->name_addr.uri) &&
		!PJSIP_URI_SCHEME_IS_SIP(r->name_addr.uri))
	    {
		return PJSIP_EINVALIDSCHEME;
	    }
	    sip_url = (pjsip_sip_uri*)r->name_addr.uri;
	    sip_url->lr_param = 1;
	}

	pj_list_push_back(&pjsua_var.outbound_proxy, r);
    }
#	endif
    
    /* Initialize PJSUA call subsystem: */
    status = pjsua_call_subsys_init(ua_cfg);
    if (status != PJ_SUCCESS)
	goto on_error;

#ifdef SUPPORT_STUN
    /* Convert deprecated STUN settings */
    if (pjsua_var.ua_cfg.stun_srv_cnt==0) {
	if (pjsua_var.ua_cfg.stun_domain.slen) {
	    pjsua_var.ua_cfg.stun_srv[pjsua_var.ua_cfg.stun_srv_cnt++] = 
		pjsua_var.ua_cfg.stun_domain;
	}
	if (pjsua_var.ua_cfg.stun_host.slen) {
	    pjsua_var.ua_cfg.stun_srv[pjsua_var.ua_cfg.stun_srv_cnt++] = 
		pjsua_var.ua_cfg.stun_host;
	}
    }

    /* Start resolving STUN server */
    status = resolve_stun_server(PJ_FALSE);
    if (status != PJ_SUCCESS && status != PJ_EPENDING) {
	pjsua_perror(THIS_FILE, "Error resolving STUN server", status);
	return status;
    }
#endif /* SUPPORT_STUN */

    /* Initialize PJSUA media subsystem */
    status = pjsua_media_subsys_init(media_cfg);
    if (status != PJ_SUCCESS)
	goto on_error;


    /* Init core SIMPLE module : */
    status = pjsip_evsub_init_module(pjsua_var.endpt);
    PJ_ASSERT_RETURN(status == PJ_SUCCESS, status);

#	if defined(SUPPORT_PRESENCE) && SUPPORT_PRESENCE!=0
    /* Init presence module: */
    status = pjsip_pres_init_module( pjsua_var.endpt, pjsip_evsub_instance());
    PJ_ASSERT_RETURN(status == PJ_SUCCESS, status);
#	endif

    /* Initialize MWI support */
    status = pjsip_mwi_init_module(pjsua_var.endpt, pjsip_evsub_instance());

    /* Init PUBLISH module */
    pjsip_publishc_init_module(pjsua_var.endpt);

    /* Init xfer/REFER module */
    status = pjsip_xfer_init_module( pjsua_var.endpt );
    PJ_ASSERT_RETURN(status == PJ_SUCCESS, status);

#	if defined(SUPPORT_PRESENCE) && SUPPORT_PRESENCE!=0
    /* Init pjsua presence handler: */
    status = pjsua_pres_init();
    if (status != PJ_SUCCESS)
	goto on_error;
#	endif

	 /*Init mwi*/
	 status = pjsua_mwi_init();
	 if (status != PJ_SUCCESS)
	 {
	 	goto on_error;
	 }

#	if defined(SUPPORT_IM) && SUPPORT_IM!=0
    /* Init out-of-dialog MESSAGE request handler. */
    status = pjsua_im_init();
    if (status != PJ_SUCCESS)
	goto on_error;
#	endif

    /* Register OPTIONS handler */
    pjsip_endpt_register_module(pjsua_var.endpt, &pjsua_options_handler);

    /* Add OPTIONS in Allow header */
    pjsip_endpt_add_capability(pjsua_var.endpt, NULL, PJSIP_H_ALLOW,
			       NULL, 1, &STR_OPTIONS);

    /* Start worker thread if needed. */
    if (pjsua_var.ua_cfg.thread_cnt)
	 {
	unsigned i;

	if (pjsua_var.ua_cfg.thread_cnt > PJ_ARRAY_SIZE(pjsua_var.thread))
		{
	    pjsua_var.ua_cfg.thread_cnt = PJ_ARRAY_SIZE(pjsua_var.thread);
		}
		
		for (i=0; i<pjsua_var.ua_cfg.thread_cnt; ++i)
		{
	    status = pj_thread_create(pjsua_var.pool, "pjsua", &worker_thread,
				      NULL, 0, 0, &pjsua_var.thread[i]);
	    if (status != PJ_SUCCESS)
		    {
		goto on_error;
	}
		}
		
	PJ_LOG(4,(THIS_FILE, "%d SIP worker threads created", 
		  pjsua_var.ua_cfg.thread_cnt));
    }
	 else 
	 {
	PJ_LOG(4,(THIS_FILE, "No SIP worker threads created"));
    }

	 	 /*ycw-pjsip. Set the telephone-event flag according CM's configuration*/
	 if (ua_cfg->has_telephone_event)
	 {
	 	CMSIP_ASSERT(pjsua_var.med_endpt);
	 	pjmedia_endpt_set_flag(pjsua_var.med_endpt, PJMEDIA_ENDPT_HAS_TELEPHONE_EVENT_FLAG, (const void*)&ua_cfg->has_telephone_event);
	 }


    /* Done! */
# if 0
    PJ_LOG(3,(THIS_FILE, "pjsua version %s for %s initialized", 
			 pj_get_version(), pj_get_sys_info()->info.ptr));
#	endif

    return PJ_SUCCESS;

on_error:
    pjsua_destroy();
    return status;
}


/* Sleep with polling */
static void busy_sleep(unsigned msec)
{
    pj_time_val timeout, now;

    pj_gettimeofday(&timeout);
    timeout.msec += msec;
    pj_time_val_normalize(&timeout);

    do {
	int i;
	i = msec / 10;
	while (pjsua_handle_events(10) > 0 && i > 0)
	    --i;
	pj_gettimeofday(&now);
    } while (PJ_TIME_VAL_LT(now, timeout));
}

#if SUPPORT_STUN
/* Internal function to destroy STUN resolution session
 * (pj_stun_resolve).
 */
static void destroy_stun_resolve(pjsua_stun_resolve *sess)
{
    PJSUA_LOCK();
    pj_list_erase(sess);
    PJSUA_UNLOCK();

    pj_assert(sess->stun_sock==NULL);
    pj_pool_release(sess->pool);
}

/* This is the internal function to be called when STUN resolution
 * session (pj_stun_resolve) has completed.
 */
static void stun_resolve_complete(pjsua_stun_resolve *sess)
{
    pj_stun_resolve_result result;

    pj_bzero(&result, sizeof(result));
    result.token = sess->token;
    result.status = sess->status;
    result.name = sess->srv[sess->idx];
    pj_memcpy(&result.addr, &sess->addr, sizeof(result.addr));

    if (result.status == PJ_SUCCESS) {
	char addr[PJ_INET6_ADDRSTRLEN+10];
	pj_sockaddr_print(&result.addr, addr, sizeof(addr), 3);
	PJ_LOG(4,(THIS_FILE, 
		  "STUN resolution success, using %.*s, address is %s",
		  (int)sess->srv[sess->idx].slen,
		  sess->srv[sess->idx].ptr,
		  addr));
    } else {
	char errmsg[PJ_ERR_MSG_SIZE];
	pj_strerror(result.status, errmsg, sizeof(errmsg));
	PJ_LOG(1,(THIS_FILE, "STUN resolution failed: %s", errmsg));
    }

    sess->cb(&result);

    if (!sess->blocking) {
	destroy_stun_resolve(sess);
    }
}

/* This is the callback called by the STUN socket (pj_stun_sock)
 * to report it's state. We use this as part of testing the
 * STUN server.
 */
static pj_bool_t test_stun_on_status(pj_stun_sock *stun_sock, 
				     pj_stun_sock_op op,
				     pj_status_t status)
{
    pjsua_stun_resolve *sess;

    sess = (pjsua_stun_resolve*) pj_stun_sock_get_user_data(stun_sock);
    pj_assert(stun_sock == sess->stun_sock);

    if (status != PJ_SUCCESS) {
	char errmsg[PJ_ERR_MSG_SIZE];
	pj_strerror(status, errmsg, sizeof(errmsg));

	PJ_LOG(4,(THIS_FILE, "STUN resolution for %.*s failed: %s",
		  (int)sess->srv[sess->idx].slen,
		  sess->srv[sess->idx].ptr, errmsg));

	sess->status = status;

	pj_stun_sock_destroy(stun_sock);
	sess->stun_sock = NULL;

	++sess->idx;
	resolve_stun_entry(sess);

	return PJ_FALSE;

    } else if (op == PJ_STUN_SOCK_BINDING_OP) {
	pj_stun_sock_info ssi;

	pj_stun_sock_get_info(stun_sock, &ssi);
	pj_memcpy(&sess->addr, &ssi.srv_addr, sizeof(sess->addr));

	sess->status = PJ_SUCCESS;
	pj_stun_sock_destroy(stun_sock);
	sess->stun_sock = NULL;

	stun_resolve_complete(sess);

	return PJ_FALSE;

    } else
	return PJ_TRUE;
    
}

/* This is an internal function to resolve and test current
 * server entry in pj_stun_resolve session. It is called by
 * pjsua_resolve_stun_servers() and test_stun_on_status() above
 */
static void resolve_stun_entry(pjsua_stun_resolve *sess)
{
    /* Loop while we have entry to try */
    for (; sess->idx < sess->count; ++sess->idx) {
	const int af = pj_AF_INET();
	pj_str_t hostpart;
	pj_uint16_t port;
	pj_stun_sock_cb stun_sock_cb;
	
	pj_assert(sess->idx < sess->count);

	/* Parse the server entry into host:port */
	sess->status = pj_sockaddr_parse2(af, 0, &sess->srv[sess->idx],
					  &hostpart, &port, NULL);
	if (sess->status != PJ_SUCCESS) {
	    PJ_LOG(2,(THIS_FILE, "Invalid STUN server entry %.*s", 
		      (int)sess->srv[sess->idx].slen, 
		      sess->srv[sess->idx].ptr));
	    continue;
	}
	
	/* Use default port if not specified */
	if (port == 0)
	    port = PJ_STUN_PORT;

	pj_assert(sess->stun_sock == NULL);

	PJ_LOG(4,(THIS_FILE, "Trying STUN server %.*s (%d of %d)..",
		  (int)sess->srv[sess->idx].slen,
		  sess->srv[sess->idx].ptr,
		  sess->idx+1, sess->count));

	/* Use STUN_sock to test this entry */
	pj_bzero(&stun_sock_cb, sizeof(stun_sock_cb));
	stun_sock_cb.on_status = &test_stun_on_status;
	sess->status = pj_stun_sock_create(&pjsua_var.stun_cfg, "stunresolve",
					   pj_AF_INET(), &stun_sock_cb,
					   NULL, sess, &sess->stun_sock);
	if (sess->status != PJ_SUCCESS) {
	    char errmsg[PJ_ERR_MSG_SIZE];
	    pj_strerror(sess->status, errmsg, sizeof(errmsg));
	    PJ_LOG(4,(THIS_FILE, 
		     "Error creating STUN socket for %.*s: %s",
		      (int)sess->srv[sess->idx].slen,
		      sess->srv[sess->idx].ptr, errmsg));

	    continue;
	}

	sess->status = pj_stun_sock_start(sess->stun_sock, &hostpart,
					  port, pjsua_var.resolver);
	if (sess->status != PJ_SUCCESS) {
	    char errmsg[PJ_ERR_MSG_SIZE];
	    pj_strerror(sess->status, errmsg, sizeof(errmsg));
	    PJ_LOG(4,(THIS_FILE, 
		     "Error starting STUN socket for %.*s: %s",
		      (int)sess->srv[sess->idx].slen,
		      sess->srv[sess->idx].ptr, errmsg));

	    pj_stun_sock_destroy(sess->stun_sock);
	    sess->stun_sock = NULL;
	    continue;
	}
	/* Done for now, testing will resume/complete asynchronously in
	 * stun_sock_cb()
	 */
	return;
    }

    if (sess->idx >= sess->count) {
	/* No more entries to try */
	PJ_ASSERT_ON_FAIL(sess->status != PJ_SUCCESS, 
			  sess->status = PJ_EUNKNOWN);
	stun_resolve_complete(sess);
    }
}


/*
 * Resolve STUN server.
 */
pj_status_t pjsua_resolve_stun_servers( unsigned count,
						pj_str_t srv[],
						pj_bool_t wait,
						void *token,
						pj_stun_resolve_cb cb)
{
    pj_pool_t *pool;
    pjsua_stun_resolve *sess;
    pj_status_t status;
    unsigned i;

    PJ_ASSERT_RETURN(count && srv && cb, PJ_EINVAL);

    pool = pjsua_pool_create("stunres", 256, 256);
    if (!pool)
	return PJ_ENOMEM;

    sess = PJ_POOL_ZALLOC_T(pool, pjsua_stun_resolve);
    sess->pool = pool;
    sess->token = token;
    sess->cb = cb;
    sess->count = count;
    sess->blocking = wait;
    sess->status = PJ_EPENDING;
    sess->srv = (pj_str_t*) pj_pool_calloc(pool, count, sizeof(pj_str_t));
    for (i=0; i<count; ++i) {
	pj_strdup(pool, &sess->srv[i], &srv[i]);
    }
    PJSUA_LOCK();
    pj_list_push_back(&pjsua_var.stun_res, sess);
    PJSUA_UNLOCK();

    resolve_stun_entry(sess);

    if (!wait)
	return PJ_SUCCESS;

    while (sess->status == PJ_EPENDING) {
	pjsua_handle_events(50);
    }

    status = sess->status;
    destroy_stun_resolve(sess);

    return status;
}

/*
 * Cancel pending STUN resolution.
 */
pj_status_t pjsua_cancel_stun_resolution( void *token,
						  pj_bool_t notify_cb)
{
    pjsua_stun_resolve *sess;
    unsigned cancelled_count = 0;
    PJSUA_LOCK();
    sess = pjsua_var.stun_res.next;
    while (sess != &pjsua_var.stun_res) {
	pjsua_stun_resolve *next = sess->next;

	if (sess->token == token) {
	    if (notify_cb) {
		pj_stun_resolve_result result;

		pj_bzero(&result, sizeof(result));
		result.token = token;
		result.status = PJ_ECANCELLED;

		sess->cb(&result);
	    }

	    destroy_stun_resolve(sess);
	    ++cancelled_count;
	}

	sess = next;
    }
    PJSUA_UNLOCK();

    return cancelled_count ? PJ_SUCCESS : PJ_ENOTFOUND;
}

static void internal_stun_resolve_cb(const pj_stun_resolve_result *result)
{
    pjsua_var.stun_status = result->status;
    if (result->status == PJ_SUCCESS) {
	pj_memcpy(&pjsua_var.stun_srv, &result->addr, sizeof(result->addr));
    }
}

/*
 * Resolve STUN server.
 */
pj_status_t resolve_stun_server(pj_bool_t wait)
{
    if (pjsua_var.stun_status == PJ_EUNKNOWN) {
	pj_status_t status;

	/* Initialize STUN configuration */
	pj_stun_config_init(&pjsua_var.stun_cfg, &pjsua_var.cp.factory, 0,
			    pjsip_endpt_get_ioqueue(pjsua_var.endpt),
			    pjsip_endpt_get_timer_heap(pjsua_var.endpt));

	/* Start STUN server resolution */
	if (pjsua_var.ua_cfg.stun_srv_cnt) {
	    pjsua_var.stun_status = PJ_EPENDING;
	    status = pjsua_resolve_stun_servers(pjsua_var.ua_cfg.stun_srv_cnt,
						pjsua_var.ua_cfg.stun_srv,
						wait, NULL,
						&internal_stun_resolve_cb);
	    if (wait || status != PJ_SUCCESS) {
		pjsua_var.stun_status = status;
	    }
	} else {
	    pjsua_var.stun_status = PJ_SUCCESS;
	}

    } else if (pjsua_var.stun_status == PJ_EPENDING) {
	/* STUN server resolution has been started, wait for the
	 * result.
	 */
	if (wait) {
	    while (pjsua_var.stun_status == PJ_EPENDING)
		pjsua_handle_events(10);
	}
    }

    if (pjsua_var.stun_status != PJ_EPENDING &&
	pjsua_var.stun_status != PJ_SUCCESS &&
	pjsua_var.ua_cfg.stun_ignore_failure)
    {
	PJ_LOG(2,(THIS_FILE, 
		  "Ignoring STUN resolution failure (by setting)"));
	pjsua_var.stun_status = PJ_SUCCESS;
    }

    return pjsua_var.stun_status;
}
#endif /* SUPPORT_STUN */

/*
 * Destroy pjsua.
 */
pj_status_t pjsua_destroy(void)
{
	int i;  /* Must be signed */
	/*ycw-firewall*/
	#if defined(PJSIP_HAS_FIREWALL_FILTER)&& PJSIP_HAS_FIREWALL_FILTER!=0
	 PJ_FIREWALL_RULE* pFwRule = NULL;
	#endif

	pj_str_t ifname = {NULL, 0};

	/* Signal threads to quit: */
	pjsua_var.thread_quit_flag = 1;
	/* Wait worker threads to quit: */
	for (i=0; i<(int)pjsua_var.ua_cfg.thread_cnt; ++i)
	{
		if (pjsua_var.thread[i])
		{
			pj_thread_join(pjsua_var.thread[i]);
			pj_thread_destroy(pjsua_var.thread[i]);
			pjsua_var.thread[i] = NULL;
		}
	}
	if (pjsua_var.endpt)
	{
		unsigned max_wait;

		PJ_LOG(4,(THIS_FILE, "Shutting down..."));

		/* Terminate all calls. */
		if (!app_exit_without_unregister)
		{
			CMSIP_PRINT("Hangup all calls...");
			pjsua_call_hangup_all();
		}

		/* Set all accounts to offline */
		for (i=0; i<(int)PJ_ARRAY_SIZE(pjsua_var.acc); ++i)
		{
			if (!pjsua_var.acc[i].valid
#	if defined(SUPPORT_ACCOUNT_RTT) && SUPPORT_ACCOUNT_RTT!=0
				|| pjsua_var.acc[i].regDuration<=0
#	else
				|| !pjsua_var.acc[i].regOK
#	endif
				)
				continue;
			
#	if defined(SUPPORT_PRESENCE) && SUPPORT_PRESENCE!=0
			pjsua_var.acc[i].online_status = PJ_FALSE;
			pj_bzero(&pjsua_var.acc[i].rpid, sizeof(pjrpid_element));
#	endif

		}

		/* Destroy media (to shutdown media transports etc) */
		pjsua_media_subsys_destroy();

		if (!app_exit_without_unregister)
		{
			CMSIP_PRINT("Shutdown pres/mwi/media/register...");
#	if defined(SUPPORT_PRESENCE) && SUPPORT_PRESENCE!=0
			/* Terminate all presence subscriptions. */
			pjsua_pres_shutdown();
#	endif

			/* 
			 * brief	Terminate all mwi subscriptions. yuchuwei@20120412
			 */
			pjsua_mwi_shutdown();

			#	if defined(SUPPORT_PRESENCE) && SUPPORT_PRESENCE!=0
			/* Wait for sometime until all publish client sessions are done
			* (ticket #364)
			*/
			/* First stage, get the maximum wait time */
			max_wait = 100;
			for (i=0; i<(int)PJ_ARRAY_SIZE(pjsua_var.acc); ++i)
			{
				if (!pjsua_var.acc[i].valid) continue;
				if (pjsua_var.acc[i].cfg.unpublish_max_wait_time_msec > max_wait)
				{
					max_wait = pjsua_var.acc[i].cfg.unpublish_max_wait_time_msec;
				}
			}

			/* Second stage, wait for unpublications to complete */
			for (i=0; i<(int)(max_wait/50); ++i)
			{
				unsigned j;
				for (j=0; j<PJ_ARRAY_SIZE(pjsua_var.acc); ++j)
				{
					if (!pjsua_var.acc[j].valid) continue;
					if (pjsua_var.acc[j].publish_sess) break;
				}
			 
				if (j != PJ_ARRAY_SIZE(pjsua_var.acc))
				{
					busy_sleep(50);
				}
				else
				{
					break;
				}
			}

			/* Third stage, forcefully destroy unfinished unpublications */
			for (i=0; i<(int)PJ_ARRAY_SIZE(pjsua_var.acc); ++i)
			{
				if (pjsua_var.acc[i].publish_sess)
				{
					pjsip_publishc_destroy(pjsua_var.acc[i].publish_sess);
					pjsua_var.acc[i].publish_sess = NULL;
				}
			}
#	endif

			/*ycw-pjsip. UnSubscribe all accounts' MWI*/
			for (i=0; i<(int)PJ_ARRAY_SIZE(pjsua_var.acc); ++i)
			{
				if (!pjsua_var.acc[i].valid
#	if defined(SUPPORT_ACCOUNT_RTT) && SUPPORT_ACCOUNT_RTT!=0
					|| !pjsua_var.acc[i].regDuration /*ycw-pjsip-regDuration*/
#	else
					|| !pjsua_var.acc[i].regOK
#	endif
					)
					continue;

				 pjsua_var.acc[i].cfg.mwi_enabled = PJ_FALSE;

				if (pjsua_var.acc[i].mwi_sub)
				{
			   		pjsua_start_mwi(&pjsua_var.acc[i]);
				}
			}	
			/* Unregister all accounts */
			for (i=0; i<(int)PJ_ARRAY_SIZE(pjsua_var.acc); ++i)
			{
				if (!pjsua_var.acc[i].valid
#	if defined(SUPPORT_ACCOUNT_RTT) && SUPPORT_ACCOUNT_RTT!=0
					|| !pjsua_var.acc[i].regDuration /*ycw-pjsip-regDuration*/
#	else
					|| !pjsua_var.acc[i].regOK
#	endif
					)
					continue;

				if (pjsua_var.acc[i].regc)
				{
					pjsua_acc_set_registration(i, PJ_FALSE, PJ_FALSE);
				}
#	if defined(SUPPORT_ACCOUNT_RTT) && SUPPORT_ACCOUNT_RTT!=0
				/*ycw-pjsip*/
				pjsua_var.acc[i].regDuration = 0;
#	else
				pjsua_var.acc[i].regOK = PJ_FALSE;
#	endif
			}
#if SUPPORT_STUN
			/* Terminate any pending STUN resolution */
			if (!pj_list_empty(&pjsua_var.stun_res))
			{
				pjsua_stun_resolve *sess = pjsua_var.stun_res.next;
				while (sess != &pjsua_var.stun_res)
				{
					pjsua_stun_resolve *next = sess->next;
					destroy_stun_resolve(sess);
					sess = next;
				}
			}
#endif /* SUPPORT_STUN */
			/* Wait until all unregistrations are done (ticket #364) */
			/* First stage, get the maximum wait time */
			max_wait = 100;
			for (i=0; i<(int)PJ_ARRAY_SIZE(pjsua_var.acc); ++i)
			{
				if (!pjsua_var.acc[i].valid
#	if defined(SUPPORT_ACCOUNT_RTT) && SUPPORT_ACCOUNT_RTT!=0
					|| !pjsua_var.acc[i].regDuration/*ycw-pjsip-regDuration*/
#	else
					|| !pjsua_var.acc[i].regOK
#	endif
					)
					continue;
				 
				if (pjsua_var.acc[i].cfg.unreg_timeout > max_wait)
					max_wait = pjsua_var.acc[i].cfg.unreg_timeout;
			}
			
			/* Second stage, wait for unregistrations to complete */
			for (i=0; i<(int)(max_wait/50); ++i)
			{
				unsigned j;
				for (j=0; j<PJ_ARRAY_SIZE(pjsua_var.acc); ++j)
				{
					if (!pjsua_var.acc[j].valid
#	if defined(SUPPORT_ACCOUNT_RTT) && SUPPORT_ACCOUNT_RTT!=0
						|| !pjsua_var.acc[j].regDuration/*ycw-pjsip-regDuration*/
#	else
						|| !pjsua_var.acc[j].regOK
#	endif
						)
				   		continue;

					if (pjsua_var.acc[j].regc)
				   		break;
				}
				 
				if (j != PJ_ARRAY_SIZE(pjsua_var.acc))
					busy_sleep(50);
				else
					break;
			}
			/* Note variable 'i' is used below */

			/* Wait for some time to allow unregistration and ICE/TURN
			 * transports shutdown to complete: 
			 */
			if (i < 20)
				busy_sleep(1000 - i*50);
		}

		PJ_LOG(4,(THIS_FILE, "Destroying..."));

		/* Must destroy endpoint first before destroying pools in
		 * buddies or accounts, since shutting down transaction layer
		 * may emit events which trigger some buddy or account callbacks
		 * to be called.
		 */
		pjsip_endpt_destroy(pjsua_var.endpt);
		pjsua_var.endpt = NULL;

#	if defined(SUPPORT_PRESENCE) && SUPPORT_PRESENCE!=0
		/* Destroy pool in the buddy object */
		for (i=0; i<(int)PJ_ARRAY_SIZE(pjsua_var.buddy); ++i)
		{
			if (pjsua_var.buddy[i].pool)
			{
				pj_pool_release(pjsua_var.buddy[i].pool);
				pjsua_var.buddy[i].pool = NULL;
	    	}
		}
#	endif

		/* Destroy accounts */
		for (i=0; i<(int)PJ_ARRAY_SIZE(pjsua_var.acc); ++i)
		{
			if (pjsua_var.acc[i].pool)
			{
				pj_pool_release(pjsua_var.acc[i].pool);
				pjsua_var.acc[i].pool = NULL;
			}
		}
		/*ycw-firewall*/
	 	#if defined(PJSIP_HAS_FIREWALL_FILTER)&& PJSIP_HAS_FIREWALL_FILTER!=0
		pFwRule = &pjsua_var.fwRule;
		pj_firewall_set_rule_accept(PJ_FIREWALLCFG_DEL, pFwRule->protocol, &pFwRule->destination, 
				NULL, pFwRule->dport, &pFwRule->source, NULL, pFwRule->sport, pjsua_var.fwType);
		#endif

		/*ycw-20120613: Debind the socket to the network interface*/
		for(i = 0; i<(int)PJ_ARRAY_SIZE(pjsua_var.tpdata); ++i)
		{
			if (PJSIP_TRANSPORT_UDP == pjsua_var.tpdata[i].type)
			{
				pjsip_udp_transport_bindSockToDev(pjsua_var.tpdata[i].data.tp, 
					ifname);
			}
			else
			{
				/*ycw-20120613:To process other type transport*/
				PJ_TODO(DEBIND_SOCKET_TO_DEVICE);
			}
		}
	}

	/* Destroy mutex */
	if (pjsua_var.mutex)
	{
		pj_mutex_destroy(pjsua_var.mutex);
		pjsua_var.mutex = NULL;
	}

	/* Destroy pool and pool factory. */
	if (pjsua_var.pool)
	{
		pj_pool_release(pjsua_var.pool);
		pjsua_var.pool = NULL;
		pj_caching_pool_destroy(&pjsua_var.cp);

		PJ_LOG(4,(THIS_FILE, "PJSUA destroyed..."));

#if (1 <= PJ_LOG_MAX_LEVEL)
		/* End logging */
		if (pjsua_var.log_file)
		{
	   		pj_file_close(pjsua_var.log_file);
	   		pjsua_var.log_file = NULL;
		}
#endif /* (1 <= PJ_LOG_MAX_LEVEL) */

		/* Shutdown PJLIB */
		pj_shutdown();
	}
	/* Clear pjsua_var */
	pj_bzero(&pjsua_var, sizeof(pjsua_var));

	/* Done. */
	return PJ_SUCCESS;
}


/**
 * Application is recommended to call this function after all initialization
 * is done, so that the library can do additional checking set up
 * additional 
 *
 * @return		PJ_SUCCESS on success, or the appropriate error code.
 */
pj_status_t pjsua_start(void)
{
    pj_status_t status;

	/*ycw-pjsip*/
	status = pjsua_acc_subsys_start();
	if (status != PJ_SUCCESS)
	{
		PJ_LOG(3, (THIS_FILE, "Can not create Registration Monitor Timer"));
	}

    status = pjsua_call_subsys_start();
    if (status != PJ_SUCCESS)
	return status;

    status = pjsua_media_subsys_start();
    if (status != PJ_SUCCESS)
	return status;

#	if defined(SUPPORT_PRESENCE) && SUPPORT_PRESENCE!=0
    status = pjsua_pres_start();
    if (status != PJ_SUCCESS)
	return status;
#	endif

	status = pjsua_mwi_start();
	if (status != PJ_SUCCESS)
	{
		return status;
	}

    return PJ_SUCCESS;
}


/**
 * Poll pjsua for events, and if necessary block the caller thread for
 * the specified maximum interval (in miliseconds).
 */
int pjsua_handle_events(unsigned msec_timeout)
{
#if defined(PJ_SYMBIAN) && PJ_SYMBIAN != 0

    return pj_symbianos_poll(-1, msec_timeout);

#else

    unsigned count = 0;
    pj_time_val tv;
    pj_status_t status;

    tv.sec = 0;
    tv.msec = msec_timeout;
    pj_time_val_normalize(&tv);

    status = pjsip_endpt_handle_events2(pjsua_var.endpt, &tv, &count);

    if (status != PJ_SUCCESS)
    {
		return -status;
    }

    return count;
    
#endif
}


/*
 * Create memory pool.
 */
pj_pool_t* pjsua_pool_create( const char *name, pj_size_t init_size,
				      pj_size_t increment)
{
    /* Pool factory is thread safe, no need to lock */
    return pj_pool_create(&pjsua_var.cp.factory, name, init_size, increment, 
			  NULL);
}


/*
 * Internal function to get SIP endpoint instance of pjsua, which is
 * needed for example to register module, create transports, etc.
 * Probably is only valid after #pjsua_init() is called.
 */
pjsip_endpoint* pjsua_get_pjsip_endpt(void)
{
    return pjsua_var.endpt;
}

/*
 * Internal function to get media endpoint instance.
 * Only valid after #pjsua_init() is called.
 */
pjmedia_endpt* pjsua_get_pjmedia_endpt(void)
{
    return pjsua_var.med_endpt;
}

/*
 * Internal function to get PJSUA pool factory.
 */
pj_pool_factory* pjsua_get_pool_factory(void)
{
    return &pjsua_var.cp.factory;
}

/*****************************************************************************
 * PJSUA SIP Transport API.
 */

/*
 * Tools to get address string.
 */
static const char *addr_string(const pj_sockaddr_t *addr)
{
    static char str[128];
    str[0] = '\0';
    pj_inet_ntop(((const pj_sockaddr*)addr)->addr.sa_family, 
		 pj_sockaddr_get_addr(addr),
		 str, sizeof(str));
    return str;
}

void pjsua_acc_on_tp_state_changed(pjsip_transport *tp,
				   pjsip_transport_state state,
				   const pjsip_transport_state_info *info);

/* Callback to receive transport state notifications */
static void on_tp_state_callback(pjsip_transport *tp,
				 pjsip_transport_state state,
				 const pjsip_transport_state_info *info)
{
    if (pjsua_var.ua_cfg.cb.on_transport_state) {
	(*pjsua_var.ua_cfg.cb.on_transport_state)(tp, state, info);
    }
    if (pjsua_var.old_tp_cb) {
	(*pjsua_var.old_tp_cb)(tp, state, info);
    }
    pjsua_acc_on_tp_state_changed(tp, state, info);
}

/*
 * Create and initialize SIP socket (and possibly resolve public
 * address via STUN, depending on config).
 */
static pj_status_t create_sip_udp_sock(int af,
				       const pjsua_transport_config *cfg,
				       pj_sock_t *p_sock,
				       pj_sockaddr *p_pub_addr)
{
#if SUPPORT_STUN
    char stun_ip_addr[PJ_INET6_ADDRSTRLEN];
#endif
    unsigned port = cfg->port;
#if SUPPORT_STUN
    pj_str_t stun_srv;
#endif
    pj_sock_t sock;
    pj_sockaddr bind_addr;
    pj_status_t status;

	 /*ycw-pjsip.enable bind address reuse*/
	 int on = 1;
#if SUPPORT_STUN
    /* Make sure STUN server resolution has completed */
    status = resolve_stun_server(PJ_TRUE);
    if (status != PJ_SUCCESS)
	 {
		pjsua_perror(THIS_FILE, "Error resolving STUN server", status);
		return status;
    }
#endif /* SUPPORT_STUN */
    /* Initialize bound address */
    if (cfg->bound_addr.slen)
	 {
		status = pj_sockaddr_init(af, &bind_addr, &cfg->bound_addr, 
				  (pj_uint16_t)port);
		if (status != PJ_SUCCESS)
		{
	   	pjsua_perror(THIS_FILE, 
			 "Unable to resolve transport bound address", 
			 status);
	    	return status;
		}
    }
	 else
	 {
		pj_sockaddr_init(af, &bind_addr, NULL, (pj_uint16_t)port);
    }

    /* Create socket */
    status = pj_sock_socket(af, pj_SOCK_DGRAM(), 0, &sock);
    if (status != PJ_SUCCESS)
	 {
		pjsua_perror(THIS_FILE, "socket() error", status);
		return status;
    }

    /* Apply QoS, if specified */
    status = pj_sock_apply_qos2(sock, cfg->qos_type, 
				&cfg->qos_params, 
				2, THIS_FILE, "SIP UDP socket");

	 /*ycw-pjsip.enable bind address reuse*/
	 /*ycw-pjsip-note. 目前只使用UDP，如果开启TCP，还需设置TCP socket*/
	 status = pj_sock_setsockopt(sock, pj_SOL_SOCKET(), pj_SO_REUSEADDR(), 
	 					&on, sizeof(on));

    /* Bind socket */
    status = pj_sock_bind(sock, &bind_addr, pj_sockaddr_get_len(&bind_addr));
    if (status != PJ_SUCCESS)
	 {
		pjsua_perror(THIS_FILE, "bind() error", status);
		pj_sock_close(sock);
		return status;
    }

    /* If port is zero, get the bound port */
    if (port == 0)
	 {
		pj_sockaddr bound_addr;
		int namelen = sizeof(bound_addr);
		status = pj_sock_getsockname(sock, &bound_addr, &namelen);
		if (status != PJ_SUCCESS)
		{
	   	pjsua_perror(THIS_FILE, "getsockname() error", status);
	   	pj_sock_close(sock);
	   	return status;
		}

		port = pj_sockaddr_get_port(&bound_addr);
    }
#if SUPPORT_STUN
    if (pjsua_var.stun_srv.addr.sa_family != 0)
	 {
		pj_ansi_strcpy(stun_ip_addr,pj_inet_ntoa(pjsua_var.stun_srv.ipv4.sin_addr));
		stun_srv = pj_str(stun_ip_addr);
    }
	 else
	 {
		stun_srv.slen = 0;
    }
#endif
    /* Get the published address, either by STUN or by resolving
     * the name of local host.
     */
    if (pj_sockaddr_has_addr(p_pub_addr))
	 {
		/*
		 * Public address is already specified, no need to resolve the 
		 * address, only set the port.
		 */
		if (pj_sockaddr_get_port(p_pub_addr) == 0)
	   {
	   	pj_sockaddr_set_port(p_pub_addr, (pj_uint16_t)port);
		}

    }
#if SUPPORT_STUN
	 else if (stun_srv.slen)
	 {
		/*
		 * STUN is specified, resolve the address with STUN.
		 */
		if (af != pj_AF_INET())
		{
	   	pjsua_perror(THIS_FILE, "Cannot use STUN", PJ_EAFNOTSUP);
	   	pj_sock_close(sock);
	   	return PJ_EAFNOTSUP;
		}

		status = pjstun_get_mapped_addr(&pjsua_var.cp.factory, 1, &sock,
				         &stun_srv, pj_ntohs(pjsua_var.stun_srv.ipv4.sin_port),
					 &stun_srv, pj_ntohs(pjsua_var.stun_srv.ipv4.sin_port),
				         &p_pub_addr->ipv4);
		if (status != PJ_SUCCESS)
		{
	   	pjsua_perror(THIS_FILE, "Error contacting STUN server", status);
	   	pj_sock_close(sock);
	   	return status;
		}
    }
#endif
	 else
	 {
		pj_bzero(p_pub_addr, sizeof(pj_sockaddr));

		if (pj_sockaddr_has_addr(&bind_addr))
		{
	   	pj_sockaddr_copy_addr(p_pub_addr, &bind_addr);
		}
		else
		{
	   	status = pj_gethostip(af, p_pub_addr);
	   	if (status != PJ_SUCCESS)
			{
				pjsua_perror(THIS_FILE, "Unable to get local host IP", status);
				pj_sock_close(sock);
				return status;
			}
		}

		p_pub_addr->addr.sa_family = (pj_uint16_t)af;
		pj_sockaddr_set_port(p_pub_addr, (pj_uint16_t)port);
    }

    *p_sock = sock;

    PJ_LOG(4,(THIS_FILE, "SIP UDP socket reachable at %s:%d",
	      addr_string(p_pub_addr),
	      (int)pj_sockaddr_get_port(p_pub_addr)));

    return PJ_SUCCESS;
}


/*
 * Create SIP transport.
 */
pj_status_t pjsua_transport_create( pjsip_transport_type_e type,
					    const pjsua_transport_config *cfg,
					    pjsua_transport_id *p_id)
{
	pjsip_transport *tp;
	unsigned id;
	pj_status_t status;
	/*ycw-firewall*/
 	#if defined(PJSIP_HAS_FIREWALL_FILTER)&& PJSIP_HAS_FIREWALL_FILTER!=0
	PJ_FIREWALL_RULE* pFwRule = NULL;	 
	#endif
	PJSUA_LOCK();

	/* Find empty transport slot */
	for (id = 0; id < PJ_ARRAY_SIZE(pjsua_var.tpdata); ++id)
	{
		if (pjsua_var.tpdata[id].data.ptr == NULL)
		{
			break;
		}
	}

	if (id == PJ_ARRAY_SIZE(pjsua_var.tpdata))
	{
		status = PJ_ETOOMANY;
		pjsua_perror(THIS_FILE, "Error creating transport", status);
		goto on_return;
	}

	/*ycw-firewall*/
	#if defined(PJSIP_HAS_FIREWALL_FILTER)&& PJSIP_HAS_FIREWALL_FILTER!=0
	 pFwRule = &pjsua_var.fwRule;
	#endif

	/* Create the transport */
	if (type==PJSIP_TRANSPORT_UDP || type==PJSIP_TRANSPORT_UDP6)
	{
		/*
		 * Create UDP transport (IPv4 or IPv6).
		 */
		pjsua_transport_config config;
		char hostbuf[PJ_INET6_ADDRSTRLEN];
		pj_sock_t sock = PJ_INVALID_SOCKET;
		pj_sockaddr pub_addr;
		pjsip_host_port addr_name;

		/* Supply default config if it's not specified */
		if (cfg == NULL)
		{
		    pjsua_transport_config_default(&config);
		    cfg = &config;
		}

		/* Initialize the public address from the config, if any */
		pj_sockaddr_init(pjsip_transport_type_get_af(type), &pub_addr, 
				 NULL, (pj_uint16_t)cfg->port);
		if (cfg->public_addr.slen)
		{
			status = pj_sockaddr_set_str_addr(pjsip_transport_type_get_af(type),
						      &pub_addr, &cfg->public_addr);
			if (status != PJ_SUCCESS)
			{
				pjsua_perror(THIS_FILE, 
				     "Unable to resolve transport public address", 
				     status);
				goto on_return;
			}
		}

		/* Create the socket and possibly resolve the address with STUN 
	 	*	(only when public address is not specified).
	 	*/
		status = create_sip_udp_sock(pjsip_transport_type_get_af(type),
				     cfg, &sock, &pub_addr);
		if (status != PJ_SUCCESS)
		{
			goto on_return;
		}

		pj_ansi_strcpy(hostbuf, addr_string(&pub_addr));
		addr_name.host = pj_str(hostbuf);
		addr_name.port = pj_sockaddr_get_port(&pub_addr);

		/* Create UDP transport */
		status = pjsip_udp_transport_attach2(pjsua_var.endpt, type, sock,
					     &addr_name, 1, &tp);
		if (status != PJ_SUCCESS)
		{
		   	pjsua_perror(THIS_FILE, "Error creating SIP UDP transport", 
				 status);
		   	pj_sock_close(sock);
		   	goto on_return;
		}

		/*ycw-20120613: bind this socket to the specific network interface*/
		status = pjsip_udp_transport_bindSockToDev(tp, cfg->bound_ifName);
		if (status != PJ_SUCCESS)
		{
			pjsua_perror(THIS_FILE, "Error bind socket to device", status);
			pj_sock_close(sock);
			goto on_return;
		}

		/*ycw-firewall*/
		#if defined(PJSIP_HAS_FIREWALL_FILTER)&& PJSIP_HAS_FIREWALL_FILTER!=0
		/*Create netfilter rule for this listener socket.
		Beacuse CM must configurate --bound-addr option, so, I can use it.
		*/
		CMSIP_PRINT("=========udp listener ip[%.*s], port[%d]\n", 
							cfg->bound_addr.slen, cfg->bound_addr.ptr, cfg->port);
		status = pj_firewall_set_rule_accept(PJ_FIREWALLCFG_ADD, PJ_TRANSPORT_UDP, &cfg->bound_addr,
						NULL, cfg->port, NULL, NULL, -1, pjsua_var.fwType);
		if (status != PJ_SUCCESS)
		{
			pjsua_perror(THIS_FILE, "Error create Netfilter rule", status);
			goto on_return;
		}
		/*save the rule data*/
		memset(pFwRule, 0, sizeof(PJ_FIREWALL_RULE));
		pFwRule->protocol = PJ_TRANSPORT_UDP;
		sprintf(pFwRule->dstBuf, "%.*s", (int)cfg->bound_addr.slen, cfg->bound_addr.ptr);
		pFwRule->destination = pj_str(pFwRule->dstBuf);
		sprintf(pFwRule->dstNetMaskBuf, "%.*s", (int)cfg->bound_addr_netmask.slen, cfg->bound_addr_netmask.ptr);
		pFwRule->dstNetmask = pj_str(pFwRule->dstNetMaskBuf);
		pFwRule->dport = cfg->port;
		pFwRule->sport = -1;
		#endif

		/* Save the transport */
		pjsua_var.tpdata[id].type = type;
		pjsua_var.tpdata[id].local_name = tp->local_name;
		pjsua_var.tpdata[id].data.tp = tp;

#if defined(PJ_HAS_TCP) && PJ_HAS_TCP!=0

	}
	else if (type == PJSIP_TRANSPORT_TCP || type == PJSIP_TRANSPORT_TCP6)
	{
	/*
	 * Create TCP transport.
	 */
	pjsua_transport_config config;
	pjsip_tpfactory *tcp;
	pjsip_tcp_transport_cfg tcp_cfg;

	pjsip_tcp_transport_cfg_default(&tcp_cfg, pj_AF_INET());

	/* Supply default config if it's not specified */
	if (cfg == NULL) {
	    pjsua_transport_config_default(&config);
	    cfg = &config;
	}

	/* Configure bind address */
	if (cfg->port)
	    pj_sockaddr_set_port(&tcp_cfg.bind_addr, (pj_uint16_t)cfg->port);

	if (cfg->bound_addr.slen) {
	    status = pj_sockaddr_set_str_addr(tcp_cfg.af, 
					      &tcp_cfg.bind_addr,
					      &cfg->bound_addr);
	    if (status != PJ_SUCCESS) {
		pjsua_perror(THIS_FILE, 
			     "Unable to resolve transport bound address", 
			     status);
		goto on_return;
	    }
	}

	/* Set published name */
	if (cfg->public_addr.slen)
	    tcp_cfg.addr_name.host = cfg->public_addr;

	/* Copy the QoS settings */
	tcp_cfg.qos_type = cfg->qos_type;
	pj_memcpy(&tcp_cfg.qos_params, &cfg->qos_params, 
		  sizeof(cfg->qos_params));

	/* Create the TCP transport */
	status = pjsip_tcp_transport_start3(pjsua_var.endpt, &tcp_cfg, &tcp);

	if (status != PJ_SUCCESS) {
	    pjsua_perror(THIS_FILE, "Error creating SIP TCP listener", 
			 status);
	    goto on_return;
	}

	/*ycw-firewall*/
 	#if defined(PJSIP_HAS_FIREWALL_FILTER)&& PJSIP_HAS_FIREWALL_FILTER!=0
	/*Create netfilter rule for this listener socket.
	Beacuse CM must configurate --bound-addr option, so, I can use it.
	*/
	CMSIP_PRINT("=========tcp listener ip[%.*s], port[%d]\n", 
						cfg->bound_addr.slen, cfg->bound_addr.ptr, cfg->port);
	status = pj_firewall_set_rule_accept(PJ_FIREWALLCFG_ADD, PJ_TRANSPORT_TCP, &cfg->bound_addr,
					NULL, cfg->port, NULL, NULL, -1, pjsua_var.fwType);
	if (status != PJ_SUCCESS)
	{
		pjsua_perror(THIS_FILE, "Error create Netfilter rule", status);
		goto on_return;
	}
	/*save the rule data*/
	memset(pFwRule, 0, sizeof(PJ_FIREWALL_RULE));
	pFwRule->protocol = PJ_TRANSPORT_TCP;
	sprintf(pFwRule->dstBuf, "%.*s", (int)cfg->bound_addr.slen, cfg->bound_addr.ptr);
	pFwRule->destination = pj_str(pFwRule->dstBuf);
	sprintf(pFwRule->dstNetMaskBuf, "%.*s", (int)cfg->bound_addr_netmask.slen, cfg->bound_addr_netmask.ptr);
	pFwRule->dstNetmask = pj_str(pFwRule->dstNetMaskBuf);
	pFwRule->dport = cfg->port;
	pFwRule->sport = -1;	
	#endif

	/* Save the transport */
	pjsua_var.tpdata[id].type = type;
	pjsua_var.tpdata[id].local_name = tcp->addr_name;
	pjsua_var.tpdata[id].data.factory = tcp;

#endif	/* PJ_HAS_TCP */

#if defined(PJSIP_HAS_TLS_TRANSPORT) && PJSIP_HAS_TLS_TRANSPORT!=0
    } else if (type == PJSIP_TRANSPORT_TLS) {
	/*
	 * Create TLS transport.
	 */
	pjsua_transport_config config;
	pjsip_host_port a_name;
	pjsip_tpfactory *tls;
	pj_sockaddr_in local_addr;

	/* Supply default config if it's not specified */
	if (cfg == NULL) {
	    pjsua_transport_config_default(&config);
	    config.port = 5061;
	    cfg = &config;
	}

	/* Init local address */
	pj_sockaddr_in_init(&local_addr, 0, 0);

	if (cfg->port)
	    local_addr.sin_port = pj_htons((pj_uint16_t)cfg->port);

	if (cfg->bound_addr.slen) {
	    status = pj_sockaddr_in_set_str_addr(&local_addr,&cfg->bound_addr);
	    if (status != PJ_SUCCESS) {
		pjsua_perror(THIS_FILE, 
			     "Unable to resolve transport bound address", 
			     status);
		goto on_return;
	    }
	}

	/* Init published name */
	pj_bzero(&a_name, sizeof(pjsip_host_port));
	if (cfg->public_addr.slen)
	    a_name.host = cfg->public_addr;

	status = pjsip_tls_transport_start(pjsua_var.endpt, 
					   &cfg->tls_setting, 
					   &local_addr, &a_name, 1, &tls);
	if (status != PJ_SUCCESS) {
	    pjsua_perror(THIS_FILE, "Error creating SIP TLS listener", 
			 status);
	    goto on_return;
	}

	/*ycw-firewall*/
	#if defined(PJSIP_HAS_FIREWALL_FILTER)&& PJSIP_HAS_FIREWALL_FILTER!=0
	/*Create netfilter rule for this listener socket.
	Beacuse CM must configurate --bound-addr option, so, I can use it.
	*/
	CMSIP_PRINT("=========tcp listener ip[%.*s], port[%d]\n", 
						cfg->bound_addr.slen, cfg->bound_addr.ptr, cfg->port);
	status = pj_firewall_set_rule_accept(PJ_FIREWALLCFG_ADD, PJ_TRANSPORT_TCP, &cfg->bound_addr,
					NULL, cfg->port, NULL, NULL, -1, pjsua_var.fwType);
	if (status != PJ_SUCCESS)
	{
		pjsua_perror(THIS_FILE, "Error create Netfilter rule", status);
		goto on_return;
	}
	/*save the rule data*/
	memset(pFwRule, 0, sizeof(PJ_FIREWALL_RULE));
	pFwRule->protocol = PJ_TRANSPORT_TCP;
	sprintf(pFwRule->dstBuf, "%.*s", cfg->bound_addr.slen, cfg->bound_addr.ptr);
	pFwRule->destination = pj_str(pFwRule->dstBuf);
	sprintf(pFwRule->dstNetMaskBuf, "%.*s", cfg->bound_addr_netmask.slen, cfg->bound_addr_netmask.ptr);
	pFwRule->dstNetmask = pj_str(pFwRule->dstNetMaskBuf);
	pFwRule->dport = cfg->port;
	pFwRule->sport = -1;	
	#endif

	/* Save the transport */
	pjsua_var.tpdata[id].type = type;
	pjsua_var.tpdata[id].local_name = tls->addr_name;
	pjsua_var.tpdata[id].data.factory = tls;
#endif

    } else {
	status = PJSIP_EUNSUPTRANSPORT;
	pjsua_perror(THIS_FILE, "Error creating transport", status);
	goto on_return;
    }

    /* Set transport state callback */
    /*if (pjsua_var.ua_cfg.cb.on_transport_state) *//*bugfix#1550 trac.pjsip.org */
	{
	pjsip_tp_state_callback tpcb;
	pjsip_tpmgr *tpmgr;

	tpmgr = pjsip_endpt_get_tpmgr(pjsua_var.endpt);
	tpcb = pjsip_tpmgr_get_state_cb(tpmgr);

	if (tpcb != &on_tp_state_callback) {
	    pjsua_var.old_tp_cb = tpcb;
	    pjsip_tpmgr_set_state_cb(tpmgr, &on_tp_state_callback);
	}
    }

    /* Return the ID */
    if (p_id) *p_id = id;

    status = PJ_SUCCESS;

on_return:

    PJSUA_UNLOCK();

    return status;
}


/*
 * Register transport that has been created by application.
 */
pj_status_t pjsua_transport_register( pjsip_transport *tp,
					      pjsua_transport_id *p_id)
{
    unsigned id;
    PJSUA_LOCK();

    /* Find empty transport slot */
    for (id=0; id < PJ_ARRAY_SIZE(pjsua_var.tpdata); ++id) {
	if (pjsua_var.tpdata[id].data.ptr == NULL)
	    break;
    }

    if (id == PJ_ARRAY_SIZE(pjsua_var.tpdata)) {
	pjsua_perror(THIS_FILE, "Error creating transport", PJ_ETOOMANY);
	PJSUA_UNLOCK();
	return PJ_ETOOMANY;
    }

    /* Save the transport */
    pjsua_var.tpdata[id].type = (pjsip_transport_type_e) tp->key.type;
    pjsua_var.tpdata[id].local_name = tp->local_name;
    pjsua_var.tpdata[id].data.tp = tp;

    /* Return the ID */
    if (p_id) *p_id = id;

    PJSUA_UNLOCK();

    return PJ_SUCCESS;
}


/*
 * Enumerate all transports currently created in the system.
 */
pj_status_t pjsua_enum_transports( pjsua_transport_id id[],
					   unsigned *p_count )
{
    unsigned i, count;
    PJSUA_LOCK();

    for (i=0, count=0; i<PJ_ARRAY_SIZE(pjsua_var.tpdata) && count<*p_count; 
	 ++i) 
    {
	if (!pjsua_var.tpdata[i].data.ptr)
	    continue;

	id[count++] = i;
    }

    *p_count = count;

    PJSUA_UNLOCK();

    return PJ_SUCCESS;
}


/*
 * Get information about transports.
 */
pj_status_t pjsua_transport_get_info( pjsua_transport_id id,
					      pjsua_transport_info *info)
{
    pjsua_transport_data *t = &pjsua_var.tpdata[id];
    pj_status_t status;

    pj_bzero(info, sizeof(*info));

    /* Make sure id is in range. */
    PJ_ASSERT_RETURN(id>=0 && id<(int)PJ_ARRAY_SIZE(pjsua_var.tpdata), 
		     PJ_EINVAL);

    /* Make sure that transport exists */
    PJ_ASSERT_RETURN(pjsua_var.tpdata[id].data.ptr != NULL, PJ_EINVAL);
    PJSUA_LOCK();

    if (pjsua_var.tpdata[id].type == PJSIP_TRANSPORT_UDP) {

	pjsip_transport *tp = t->data.tp;

	if (tp == NULL) {
	    PJSUA_UNLOCK();
	    return PJ_EINVALIDOP;
	}
    
	info->id = id;
	info->type = (pjsip_transport_type_e) tp->key.type;
	info->type_name = pj_str(tp->type_name);
	info->info = pj_str(tp->info);
	info->flag = tp->flag;
	info->addr_len = tp->addr_len;
	info->local_addr = tp->local_addr;
	info->local_name = tp->local_name;
	info->usage_count = pj_atomic_get(tp->ref_cnt);

	status = PJ_SUCCESS;

    } else if (pjsua_var.tpdata[id].type == PJSIP_TRANSPORT_TCP) {

	pjsip_tpfactory *factory = t->data.factory;

	if (factory == NULL) {
	    PJSUA_UNLOCK();
	    return PJ_EINVALIDOP;
	}
    
	info->id = id;
	info->type = t->type;
	info->type_name = pj_str("TCP");
	info->info = pj_str("TCP transport");
	info->flag = factory->flag;
	info->addr_len = sizeof(factory->local_addr);
	info->local_addr = factory->local_addr;
	info->local_name = factory->addr_name;
	info->usage_count = 0;

	status = PJ_SUCCESS;

    } else {
	pj_assert(!"Unsupported transport");
	status = PJ_EINVALIDOP;
    }


    PJSUA_UNLOCK();

    return status;
}


/*
 * Disable a transport or re-enable it.
 */
pj_status_t pjsua_transport_set_enable( pjsua_transport_id id,
						pj_bool_t enabled)
{
    /* Make sure id is in range. */
    PJ_ASSERT_RETURN(id>=0 && id<(int)PJ_ARRAY_SIZE(pjsua_var.tpdata), 
		     PJ_EINVAL);

    /* Make sure that transport exists */
    PJ_ASSERT_RETURN(pjsua_var.tpdata[id].data.ptr != NULL, PJ_EINVAL);


    /* To be done!! */
    PJ_TODO(pjsua_transport_set_enable);
    PJ_UNUSED_ARG(enabled);

    return PJ_EINVALIDOP;
}


/*
 * Close the transport.
 */
pj_status_t pjsua_transport_close( pjsua_transport_id id,
					   pj_bool_t force )
{
    pj_status_t status;

    /* Make sure id is in range. */
    PJ_ASSERT_RETURN(id>=0 && id<(int)PJ_ARRAY_SIZE(pjsua_var.tpdata), 
		     PJ_EINVAL);

    /* Make sure that transport exists */
    PJ_ASSERT_RETURN(pjsua_var.tpdata[id].data.ptr != NULL, PJ_EINVAL);

    /* Note: destroy() may not work if there are objects still referencing
     *	     the transport.
     */
    if (force) {
	switch (pjsua_var.tpdata[id].type) {
	case PJSIP_TRANSPORT_UDP:
	    status = pjsip_transport_shutdown(pjsua_var.tpdata[id].data.tp);
	    if (status  != PJ_SUCCESS)
		return status;
	    status = pjsip_transport_destroy(pjsua_var.tpdata[id].data.tp);
	    if (status != PJ_SUCCESS)
		return status;
	    break;

	case PJSIP_TRANSPORT_TLS:
	case PJSIP_TRANSPORT_TCP:
	    /* This will close the TCP listener, but existing TCP/TLS
	     * connections (if any) will still linger 
	     */
	    status = (*pjsua_var.tpdata[id].data.factory->destroy)
			(pjsua_var.tpdata[id].data.factory);
	    if (status != PJ_SUCCESS)
		return status;

	    break;

	default:
	    return PJ_EINVAL;
	}
	
    } else {
	/* If force is not specified, transports will be closed at their
	 * convenient time. However this will leak PJSUA-API transport
	 * descriptors as PJSUA-API wouldn't know when exactly the
	 * transport is closed thus it can't cleanup PJSUA transport
	 * descriptor.
	 */
	switch (pjsua_var.tpdata[id].type) {
	case PJSIP_TRANSPORT_UDP:
	    return pjsip_transport_shutdown(pjsua_var.tpdata[id].data.tp);
	case PJSIP_TRANSPORT_TLS:
	case PJSIP_TRANSPORT_TCP:
	    return (*pjsua_var.tpdata[id].data.factory->destroy)
			(pjsua_var.tpdata[id].data.factory);
	default:
	    return PJ_EINVAL;
	}
    }

    /* Cleanup pjsua data when force is applied */
    if (force) {
	pjsua_var.tpdata[id].type = PJSIP_TRANSPORT_UNSPECIFIED;
	pjsua_var.tpdata[id].data.ptr = NULL;
    }

    return PJ_SUCCESS;
}


/*
 * Add additional headers etc in msg_data specified by application
 * when sending requests.
 */
void pjsua_process_msg_data(pjsip_tx_data *tdata,
			    const pjsua_msg_data *msg_data)
{
    pj_bool_t allow_body;
    const pjsip_hdr *hdr;

    /* Always add User-Agent */
    if (pjsua_var.ua_cfg.user_agent.slen && 
	tdata->msg->type == PJSIP_REQUEST_MSG) 
    {
	const pj_str_t STR_USER_AGENT = { "User-Agent", 10 };
	pjsip_hdr *h;
	h = (pjsip_hdr*)pjsip_generic_string_hdr_create(tdata->pool, 
							&STR_USER_AGENT, 
							&pjsua_var.ua_cfg.user_agent);
	pjsip_msg_add_hdr(tdata->msg, h);
    }

    if (!msg_data)
	return;

    hdr = msg_data->hdr_list.next;
    while (hdr && hdr != &msg_data->hdr_list) {
	pjsip_hdr *new_hdr;

	new_hdr = (pjsip_hdr*) pjsip_hdr_clone(tdata->pool, hdr);
	pjsip_msg_add_hdr(tdata->msg, new_hdr);

	hdr = hdr->next;
    }

    allow_body = (tdata->msg->body == NULL);

    if (allow_body && msg_data->content_type.slen && msg_data->msg_body.slen) {
	pjsip_media_type ctype;
	pjsip_msg_body *body;	

	pjsua_parse_media_type(tdata->pool, &msg_data->content_type, &ctype);
	body = pjsip_msg_body_create(tdata->pool, &ctype.type, &ctype.subtype,
				     &msg_data->msg_body);
	tdata->msg->body = body;
    }

    /* Multipart */
    if (!pj_list_empty(&msg_data->multipart_parts) &&
	msg_data->multipart_ctype.type.slen)
    {
	pjsip_msg_body *bodies;
	pjsip_multipart_part *part;
	pj_str_t *boundary = NULL;

	bodies = pjsip_multipart_create(tdata->pool,
				        &msg_data->multipart_ctype,
				        boundary);
	part = msg_data->multipart_parts.next;
	while (part != &msg_data->multipart_parts) {
	    pjsip_multipart_part *part_copy;

	    part_copy = pjsip_multipart_clone_part(tdata->pool, part);
	    pjsip_multipart_add_part(tdata->pool, bodies, part_copy);
	    part = part->next;
	}

	if (tdata->msg->body) {
	    part = pjsip_multipart_create_part(tdata->pool);
	    part->body = tdata->msg->body;
	    pjsip_multipart_add_part(tdata->pool, bodies, part);

	    tdata->msg->body = NULL;
	}

	tdata->msg->body = bodies;
    }
}


/*
 * Add route_set to outgoing requests
 */
void pjsua_set_msg_route_set( pjsip_tx_data *tdata,
			      const pjsip_route_hdr *route_set )
{
    const pjsip_route_hdr *r;

    r = route_set->next;
    while (r != route_set) {
	pjsip_route_hdr *new_r;

	new_r = (pjsip_route_hdr*) pjsip_hdr_clone(tdata->pool, r);
	pjsip_msg_add_hdr(tdata->msg, (pjsip_hdr*)new_r);

	r = r->next;
    }
}


/*
 * Simple version of MIME type parsing (it doesn't support parameters)
 */
void pjsua_parse_media_type( pj_pool_t *pool,
			     const pj_str_t *mime,
			     pjsip_media_type *media_type)
{
    pj_str_t tmp;
    char *pos;

    pj_bzero(media_type, sizeof(*media_type));

    pj_strdup_with_null(pool, &tmp, mime);

    pos = pj_strchr(&tmp, '/');
    if (pos) {
	media_type->type.ptr = tmp.ptr; 
	media_type->type.slen = (pos-tmp.ptr);
	media_type->subtype.ptr = pos+1; 
	media_type->subtype.slen = tmp.ptr+tmp.slen-pos-1;
    } else {
	media_type->type = tmp;
    }
}


/*
 * Internal function to init transport selector from transport id.
 */
void pjsua_init_tpselector(pjsua_transport_id tp_id,
			   pjsip_tpselector *sel)
{
	pjsua_transport_data *tpdata;
	unsigned flag;

	pj_bzero(sel, sizeof(*sel));
	if (tp_id == PJSUA_INVALID_ID)
	{
		return;
	}

	pj_assert(tp_id >= 0 && tp_id < (int)PJ_ARRAY_SIZE(pjsua_var.tpdata));
	tpdata = &pjsua_var.tpdata[tp_id];

	flag = pjsip_transport_get_flag_from_type(tpdata->type);

	if (flag & PJSIP_TRANSPORT_DATAGRAM)
	{
		sel->type = PJSIP_TPSELECTOR_TRANSPORT;
		sel->u.transport = tpdata->data.tp;
	}
	else
	{
		sel->type = PJSIP_TPSELECTOR_LISTENER;
		sel->u.listener = tpdata->data.factory;
	}
}

#if SUPPORT_STUN
/* Callback upon NAT detection completion */
static void nat_detect_cb(void *user_data, 
			  const pj_stun_nat_detect_result *res)
{
    PJ_UNUSED_ARG(user_data);

    pjsua_var.nat_in_progress = PJ_FALSE;
    pjsua_var.nat_status = res->status;
    pjsua_var.nat_type = res->nat_type;

    if (pjsua_var.ua_cfg.cb.on_nat_detect) {
	(*pjsua_var.ua_cfg.cb.on_nat_detect)(res);
    }
}


/*
 * Detect NAT type.
 */
pj_status_t pjsua_detect_nat_type()
{
    pj_status_t status;

    if (pjsua_var.nat_in_progress)
    {
		return PJ_SUCCESS;
    }

    /* Make sure STUN server resolution has completed */
    status = resolve_stun_server(PJ_TRUE);
    if (status != PJ_SUCCESS)
	 {
		pjsua_var.nat_status = status;
		pjsua_var.nat_type = PJ_STUN_NAT_TYPE_ERR_UNKNOWN;
		return status;
    }

    /* Make sure we have STUN */
    if (pjsua_var.stun_srv.ipv4.sin_family == 0)
	 {
		pjsua_var.nat_status = PJNATH_ESTUNINSERVER;
		return PJNATH_ESTUNINSERVER;
    }
    status = pj_stun_detect_nat_type(&pjsua_var.stun_srv.ipv4, 
				     &pjsua_var.stun_cfg, 
				     NULL, &nat_detect_cb);

    if (status != PJ_SUCCESS)
	 {
		pjsua_var.nat_status = status;
		pjsua_var.nat_type = PJ_STUN_NAT_TYPE_ERR_UNKNOWN;
		return status;
    }

    pjsua_var.nat_in_progress = PJ_TRUE;

    return PJ_SUCCESS;
}


/*
 * Get NAT type.
 */
pj_status_t pjsua_get_nat_type(pj_stun_nat_type *type)
{
    *type = pjsua_var.nat_type;
    return pjsua_var.nat_status;
}
#endif /* SUPPORT_STUN */

/*
 * Verify that valid url is given.
 */
pj_status_t pjsua_verify_url(const char *c_url)
{
	pjsip_uri *p;
	pj_pool_t *pool;
	char *url;
	int len = (c_url ? pj_ansi_strlen(c_url) : 0);

	if (!len) return PJSIP_EINVALIDURI;

	pool = pj_pool_create(&pjsua_var.cp.factory, "check%p", 1024, 0, NULL);
	if (!pool) return PJ_ENOMEM;

	url = (char*) pj_pool_alloc(pool, len+1);
	pj_ansi_strcpy(url, c_url);

	p = pjsip_parse_uri(pool, url, len, 0);

	pj_pool_release(pool);
	return p ? 0 : PJSIP_EINVALIDURI;
}

/*
 * Verify that valid SIP url is given.
 */
pj_status_t pjsua_verify_sip_url(const char *c_url)
{
    pjsip_uri *p;
    pj_pool_t *pool;
    char *url;
    int len = (c_url ? pj_ansi_strlen(c_url) : 0);

    if (!len) return PJSIP_EINVALIDURI;

    pool = pj_pool_create(&pjsua_var.cp.factory, "check%p", 1024, 0, NULL);
    if (!pool) return PJ_ENOMEM;

    url = (char*) pj_pool_alloc(pool, len+1);
    pj_ansi_strcpy(url, c_url);

    p = pjsip_parse_uri(pool, url, len, 0);
    if (!p || (pj_stricmp2(pjsip_uri_get_scheme(p), "sip") != 0 &&
	       pj_stricmp2(pjsip_uri_get_scheme(p), "sips") != 0))
    {
	p = NULL;
    }

    pj_pool_release(pool);
    return p ? 0 : PJSIP_EINVALIDURI;
}

/*
 * Schedule a timer entry. 
 */
pj_status_t pjsua_schedule_timer( pj_timer_entry *entry,
					  const pj_time_val *delay)
{
    return pjsip_endpt_schedule_timer(pjsua_var.endpt, entry, delay);
}

/*
 * Cancel the previously scheduled timer.
 *
 */
void pjsua_cancel_timer(pj_timer_entry *entry)
{
    pjsip_endpt_cancel_timer(pjsua_var.endpt, entry);
}

/** 
 * Normalize route URI (check for ";lr" and append one if it doesn't
 * exist and pjsua_config.force_lr is set.
 */
pj_status_t normalize_route_uri(pj_pool_t *pool, pj_str_t *uri)
{
    pj_str_t tmp_uri;
    pj_pool_t *tmp_pool;
    pjsip_uri *uri_obj;
    pjsip_sip_uri *sip_uri;

    tmp_pool = pjsua_pool_create("tmplr%p", 512, 512);
    if (!tmp_pool)
	return PJ_ENOMEM;

    pj_strdup_with_null(tmp_pool, &tmp_uri, uri);

    uri_obj = pjsip_parse_uri(tmp_pool, tmp_uri.ptr, tmp_uri.slen, 0);
    if (!uri_obj) {
	PJ_LOG(1,(THIS_FILE, "Invalid route URI: %.*s", 
		  (int)uri->slen, uri->ptr));
	pj_pool_release(tmp_pool);
	return PJSIP_EINVALIDURI;
    }

    if (!PJSIP_URI_SCHEME_IS_SIP(uri_obj) && 
	!PJSIP_URI_SCHEME_IS_SIP(uri_obj))
    {
	PJ_LOG(1,(THIS_FILE, "Route URI must be SIP URI: %.*s", 
		  (int)uri->slen, uri->ptr));
	pj_pool_release(tmp_pool);
	return PJSIP_EINVALIDSCHEME;
    }

    sip_uri = (pjsip_sip_uri*) pjsip_uri_get_uri(uri_obj);

    /* Done if force_lr is disabled or if lr parameter is present */
    if (!pjsua_var.ua_cfg.force_lr || sip_uri->lr_param) {
	pj_pool_release(tmp_pool);
	return PJ_SUCCESS;
    }

    /* Set lr param */
    sip_uri->lr_param = 1;

    /* Print the URI */
    tmp_uri.ptr = (char*) pj_pool_alloc(tmp_pool, PJSIP_MAX_URL_SIZE);
    tmp_uri.slen = pjsip_uri_print(PJSIP_URI_IN_ROUTING_HDR, uri_obj, 
				   tmp_uri.ptr, PJSIP_MAX_URL_SIZE);
    if (tmp_uri.slen < 1) {
	PJ_LOG(1,(THIS_FILE, "Route URI is too long: %.*s", 
		  (int)uri->slen, uri->ptr));
	pj_pool_release(tmp_pool);
	return PJSIP_EURITOOLONG;
    }

    /* Clone the URI */
    pj_strdup_with_null(pool, uri, &tmp_uri);

    pj_pool_release(tmp_pool);
    return PJ_SUCCESS;
}

/*
 * This is a utility function to dump the stack states to log, using
 * verbosity level 3.
 */
 #	if 0
void pjsua_dump(pj_bool_t detail)
{
    unsigned old_decor;
    unsigned i;

    PJ_LOG(3,(THIS_FILE, "Start dumping application states:"));

    old_decor = pj_log_get_decor();
    pj_log_set_decor(old_decor & (PJ_LOG_HAS_NEWLINE | PJ_LOG_HAS_CR));

    if (detail)
	pj_dump_config();

    pjsip_endpt_dump(pjsua_get_pjsip_endpt(), detail);

    pjmedia_endpt_dump(pjsua_get_pjmedia_endpt());

    PJ_LOG(3,(THIS_FILE, "Dumping media transports:"));
    for (i=0; i<pjsua_var.ua_cfg.max_calls; ++i) {
	pjsua_call *call = &pjsua_var.calls[i];
	pjmedia_transport_info tpinfo;
	char addr_buf[80];

	/* MSVC complains about tpinfo not being initialized */
	//pj_bzero(&tpinfo, sizeof(tpinfo));

	pjmedia_transport_info_init(&tpinfo);
	pjmedia_transport_get_info(call->med_tp, &tpinfo);

	PJ_LOG(3,(THIS_FILE, " %s: %s",
		  (pjsua_var.media_cfg.enable_ice ? "ICE" : "UDP"),
		  pj_sockaddr_print(&tpinfo.sock_info.rtp_addr_name, addr_buf,
				    sizeof(addr_buf), 3)));
    }

    pjsip_tsx_layer_dump(detail);
    pjsip_ua_dump(detail);

// Dumping complete call states may require a 'large' buffer 
// (about 3KB per call session, including RTCP XR).
#if 0
    /* Dump all invite sessions: */
    PJ_LOG(3,(THIS_FILE, "Dumping invite sessions:"));

    if (pjsua_call_get_count() == 0) {

	PJ_LOG(3,(THIS_FILE, "  - no sessions -"));

    } else {
	unsigned i;

	for (i=0; i<pjsua_var.ua_cfg.max_calls; ++i) {
	    if (pjsua_call_is_active(i)) {
		/* Tricky logging, since call states log string tends to be 
		 * longer than PJ_LOG_MAX_SIZE.
		 */
		char buf[1024 * 3];
		unsigned call_dump_len;
		unsigned part_len;
		unsigned part_idx;
		unsigned log_decor;

		pjsua_call_dump(i, detail, buf, sizeof(buf), "  ");
		call_dump_len = strlen(buf);

		log_decor = pj_log_get_decor();
		pj_log_set_decor(log_decor & ~(PJ_LOG_HAS_NEWLINE | 
					       PJ_LOG_HAS_CR));
		PJ_LOG(3,(THIS_FILE, "\n"));
		pj_log_set_decor(0);

		part_idx = 0;
		part_len = PJ_LOG_MAX_SIZE-80;
		while (part_idx < call_dump_len) {
		    char p_orig, *p;

		    p = &buf[part_idx];
		    if (part_idx + part_len > call_dump_len)
			part_len = call_dump_len - part_idx;
		    p_orig = p[part_len];
		    p[part_len] = '\0';
		    PJ_LOG(3,(THIS_FILE, "%s", p));
		    p[part_len] = p_orig;
		    part_idx += part_len;
		}
		pj_log_set_decor(log_decor);
	    }
	}
    }
#endif

    /* Dump presence status */
    pjsua_pres_dump(detail);

    pj_log_set_decor(old_decor);
    PJ_LOG(3,(THIS_FILE, "Dump complete"));
}
#	endif

/*ycw-pjsip-usbvm*/
void pjsua_init_usbvmConfig(USBVMENDPTCONFIG* pConfig, int index)
{	
	#ifdef CMSIP_DEBUG
	PJ_ASSERT_ON_FAIL(pConfig, return);
	PJ_ASSERT_ON_FAIL(index >=0 && index < PJSUA_MAX_CMENDPT, return);
	#endif
	memcpy(&pjsua_var.usbVmEndptConfig[index],pConfig, sizeof(pjsua_var.usbVmEndptConfig[index]));
	pjsua_var.cmEndpt_cnt++;
}

