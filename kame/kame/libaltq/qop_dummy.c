/*
 * Copyright (C) 1999
 *	Sony Computer Science Laboratories, Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY SONY CSL AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL SONY CSL OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $Id: qop_dummy.c,v 1.1 2000/01/18 07:29:06 kjc Exp $
 */

#include <sys/param.h>
#include <sys/socket.h>
#include <net/if.h>
#include <stdio.h>
#include <syslog.h>

#include <altq/altq.h>
#include "altq_qop.h"

int null_interface_parser(const char *ifname, int argc, char **argv);
int null_class_parser(const char *ifname, const char *class_name,
		      const char *parent_name, int argc, char **argv);
static int nop_attach(struct ifinfo *ifinfo);
static int nop_detach(struct ifinfo *ifinfo);
static int nop_clear(struct ifinfo *ifinfo);
static int nop_enable(struct ifinfo *ifinfo);
static int nop_disable(struct ifinfo *ifinfo);
static int nop_acc_enable(struct ifinfo *ifinfo);
static int nop_acc_disable(struct ifinfo *ifinfo);
static int nop_add_class(struct classinfo *clinfo);
static int nop_modify_class(struct classinfo *clinfo, void *arg);
static int nop_delete_class(struct classinfo *clinfo);
static int nop_add_filter(struct fltrinfo *fltrinfo);
static int nop_delete_filter(struct fltrinfo *fltrinfo);

struct qdisc_ops nop_qdisc = {
	ALTQT_NONE,
	"nop",
	nop_attach,
	nop_detach,
	nop_clear,
	nop_enable,
	nop_disable,
	nop_acc_enable,
	nop_acc_disable,
	nop_add_class,
	nop_modify_class,
	nop_delete_class,
	nop_add_filter,
	nop_delete_filter,
};

/*
 * parser interface for null interface
 */
int
null_interface_parser(const char *ifname, int argc, char **argv)
{
	if (argc > 0) {
		LOG(LOG_ERR, 0,
		    "No discipline (e.g. cbq) specified in %s, line %d\n",
		    altqconfigfile, line_no);
		return (0);
	}
	/*
	 * add a dummy interface since traffic conditioner might need it.
	 */
	if (qop_add_if(NULL, ifname, 0, &nop_qdisc, NULL) != 0)
		return (0);
	return (1);
}

int
null_class_parser(const char *ifname, const char *class_name,
		  const char *parent_name, int argc, char **argv)
{
	LOG(LOG_ERR, 0,
	    "class cannot be defined without a queueing discipline in %s, line %d\n",
	    altqconfigfile, line_no);
	return (0);
}

static int nop_attach(struct ifinfo *ifinfo)
{
	return (0);
}

static int nop_detach(struct ifinfo *ifinfo)
{
	return (0);
}

static int nop_clear(struct ifinfo *ifinfo)
{
	return (0);
}

static int nop_enable(struct ifinfo *ifinfo)
{
	return (0);
}

static int nop_disable(struct ifinfo *ifinfo)
{
	return (0);
}

static int nop_acc_enable(struct ifinfo *ifinfo)
{
	return (0);
}

static int nop_acc_disable(struct ifinfo *ifinfo)
{
	return (0);
}

static int nop_add_class(struct classinfo *clinfo)
{
	return (0);
}

static int nop_modify_class(struct classinfo *clinfo, void *arg)
{
	return (0);
}

static int nop_delete_class(struct classinfo *clinfo)
{
	return (0);
}

static int nop_add_filter(struct fltrinfo *fltrinfo)
{
	return (0);
}

static int nop_delete_filter(struct fltrinfo *fltrinfo)
{
	return (0);
}
