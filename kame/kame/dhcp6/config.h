/*	$KAME: config.h,v 1.21 2003/01/22 07:24:15 jinmei Exp $	*/

/*
 * Copyright (C) 2002 WIDE Project.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/* per-interface information */
struct dhcp6_if {
	struct dhcp6_if *next;

	int outsock;

	/* timer for the interface */
	struct dhcp6_timer *timer;

	/* event queue */
	TAILQ_HEAD(, dhcp6_event) event_list;	

	/* static parameters of the interface */
	char *ifname;
	unsigned int ifid;
	u_int32_t linkid;	/* to send link-local packets */

	/* configuration parameters */
	u_long send_flags;
	u_long allow_flags;
#define DHCIFF_INFO_ONLY 0x1
#define DHCIFF_RAPID_COMMIT 0x2

	int server_pref;	/* server preference (server only) */

	struct dhcp6_list iapd_list;
	struct dhcp6_list reqopt_list;

	struct dhcp6_serverinfo *current_server;
	struct dhcp6_serverinfo *servers;
};

struct dhcp6_event {
	TAILQ_ENTRY(dhcp6_event) link;

	struct dhcp6_if *ifp;
	struct dhcp6_timer *timer;

	struct duid serverid;

	struct timeval tv_start; /* timestamp when the 1st msg is sent */

	/* internal timer parameters */
	long retrans;
	long init_retrans;
	long max_retrans_cnt;
	long max_retrans_time;
	long max_retrans_dur;
	int timeouts;		/* number of timeouts */

	u_int32_t xid;		/* current transaction ID */
	int state;

	TAILQ_HEAD(, dhcp6_eventdata) data_list;
};

typedef enum { DHCP6_EVDATA_IAPD } dhcp6_eventdata_t;

struct dhcp6_eventdata {
	TAILQ_ENTRY(dhcp6_eventdata) link;

	struct dhcp6_event *event;
	dhcp6_eventdata_t type;
	void *data;

	void (*destructor) __P((struct dhcp6_eventdata *));
	void *privdata;
};

struct dhcp6_serverinfo {
	struct dhcp6_serverinfo *next;

	/* option information provided in the advertisement */
	struct dhcp6_optinfo optinfo;

	int pref;		/* preference */
	int active;		/* bool; if this server is active or not */
	/* TODO: remember available information from the server */
};

/* client status code */
enum {DHCP6S_INIT, DHCP6S_SOLICIT, DHCP6S_INFOREQ, DHCP6S_REQUEST,
      DHCP6S_RENEW, DHCP6S_REBIND, DHCP6S_RELEASE, DHCP6S_IDLE};

struct prefix_ifconf {
	TAILQ_ENTRY(prefix_ifconf) link;

	char *ifname;		/* interface name such as ne0 */
	int sla_len;		/* SLA ID length in bits */
	u_int32_t sla_id;	/* need more than 32bits? */
	int ifid_len;		/* interface ID length in bits */
	int ifid_type;		/* EUI-64 and manual (unused?) */
	char ifid[16];		/* Interface ID, up to 128bits */
};
#define IFID_LEN_DEFAULT 64
#define SLA_LEN_DEFAULT 16

struct ia_conf {
	struct ia_conf *next;
	int type;
	u_int32_t iaid;

	/* type dependent values follow */
};
typedef enum {IATYPE_PD} iatype_t;

TAILQ_HEAD(pifc_list, prefix_ifconf);
struct iapd_conf {
	struct ia_conf iapd_ia;

	/* type dependent values follow */
	struct pifc_list iapd_pif_list;
};
#define iapd_next iapd_ia.next
#define iapd_type iapd_ia.type
#define iapd_id iapd_ia.iaid

/* per-host configuration */
struct host_conf {
	struct host_conf *next;

	char *name;		/* host name to identify the host */
	struct duid duid;	/* DUID for the host */
	/* delegated prefixes for the host */
	/* struct dhcp6_list prefix_list; */
	struct dhcp6_list prefix_list;

	/* bindings of delegated prefixes */
	struct dhcp6_list prefix_binding_list;
};

/* structures and definitions used in the config file parser */
struct cf_namelist {
	struct cf_namelist *next;
	char *name;
	int line;		/* the line number of the config file */
	struct cf_list *params;
};

struct cf_list {
	struct cf_list *next;
	struct cf_list *tail;
	int type;
	int line;		/* the line number of the config file */

	/* type dependent values: */
	long long num;
	struct cf_list *list;
	void *ptr;
};

enum {DECL_SEND, DECL_ALLOW, DECL_INFO_ONLY, DECL_REQUEST, DECL_DUID,
      DECL_PREFIX, DECL_PREFERENCE,
      IFPARAM_SLA_ID, IFPARAM_SLA_LEN,
      DHCPOPT_RAPID_COMMIT, DHCPOPT_PREFIX_DELEGATION, DHCPOPT_DNS,
      DHCPOPT_IA_PD,
      ADDRESS_LIST_ENT,
      IACONF_PIF };

typedef enum {DHCP6_MODE_SERVER, DHCP6_MODE_CLIENT, DHCP6_MODE_RELAY }
dhcp6_mode_t;

extern const dhcp6_mode_t dhcp6_mode;

extern struct dhcp6_if *dhcp6_if;
extern struct dhcp6_ifconf *dhcp6_iflist;
extern struct prefix_ifconf *prefix_ifconflist;
extern struct dhcp6_list dnslist;

extern void ifinit __P((char *));
extern int configure_interface __P((struct cf_namelist *));
extern int configure_host __P((struct cf_namelist *));
extern int configure_ia __P((struct cf_namelist *, iatype_t));
extern int configure_global_option __P((void));
extern void configure_cleanup __P((void));
extern void configure_commit __P((void));
extern int cfparse __P((char *));
extern struct dhcp6_if *find_ifconfbyname __P((char *));
extern struct dhcp6_if *find_ifconfbyid __P((unsigned int));
extern struct prefix_ifconf *find_prefixifconf __P((char *));
extern struct host_conf *find_hostconf __P((struct duid *));
extern struct dhcp6_prefix *find_prefix6 __P((struct dhcp6_list *,
					      struct dhcp6_prefix *));
extern struct ia_conf *find_iaconf __P((int, u_int32_t));
