/*	$KAME: control.h,v 1.3 2004/06/12 11:00:59 jinmei Exp $	*/

/*
 * Copyright (C) 2004 WIDE Project.
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

#define DEFAULT_CONTROL_ADDR "::1" /* default IPv6 address for
				    * control socket */
#define DEFAULT_CONTROL_PORT "5546" /* default TCP port for control socket */

#define DHCP6CTL_VERSION 0

#define DHCP6CTL_COMMAND_RELOAD 1
#define DHCP6CTL_COMMAND_REMOVE 2

#define DHCP6CTL_BINDING 1
#define DHCP6CTL_BINDING_IA 1

#define DHCP6CTL_IA_PD 1

/*
 * Packet formats of command protocol
 */
struct dhcp6ctl {
	u_int16_t command;
	u_int16_t len;
	u_int16_t version;
	u_int16_t reserved;
} __attribute__ ((__packed__));

struct dhcp6ctl_iaspec {
	u_int32_t type;
	u_int32_t id;
	u_int32_t duidlen;
	/* variable length of DUID follows */
} __attribute__ ((__packed__));
