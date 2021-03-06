/*	$OpenBSD: show.c,v 1.13 2008/05/08 07:19:42 claudio Exp $	*/
/*	$NetBSD: show.c,v 1.1 1996/11/15 18:01:41 gwr Exp $	*/

/*
 * Copyright (c) 1983, 1988, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/mbuf.h>
#include <sys/sysctl.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_types.h>
#include <net/pfkeyv2.h>
#include <net/route.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>
#include <netinet/ip_ipsp.h>
#include <arpa/inet.h>

#include <err.h>
#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "netstat.h"

char	*any_ntoa(const struct sockaddr *);
char	*link_print(struct sockaddr *);

#define ROUNDUP(a) \
	((a) > 0 ? (1 + (((a) - 1) | (sizeof(long) - 1))) : sizeof(long))
#define ADVANCE(x, n) (x += ROUNDUP((n)->sa_len))

#define PFKEYV2_CHUNK sizeof(u_int64_t)

/*
 * Definitions for showing gateway flags.
 */
struct bits {
	int	b_mask;
	char	b_val;
};
static const struct bits bits[] = {
	{ RTF_UP,	'U' },
	{ RTF_GATEWAY,	'G' },
	{ RTF_HOST,	'H' },
	{ RTF_REJECT,	'R' },
	{ RTF_BLACKHOLE, 'B' },
	{ RTF_DYNAMIC,	'D' },
	{ RTF_MODIFIED,	'M' },
	{ RTF_DONE,	'd' }, /* Completed -- for routing messages only */
	{ RTF_MASK,	'm' }, /* Mask Present -- for routing messages only */
	{ RTF_CLONING,	'C' },
	{ RTF_XRESOLVE,	'X' },
	{ RTF_LLINFO,	'L' },
	{ RTF_STATIC,	'S' },
	{ RTF_PROTO1,	'1' },
	{ RTF_PROTO2,	'2' },
	{ RTF_PROTO3,	'3' },
	{ RTF_CLONED,	'c' },
	{ RTF_JUMBO,	'J' },
	{ 0 }
};

void	 p_rtentry(struct rt_msghdr *);
void	 p_pfkentry(struct sadb_msg *);
void	 pr_family(int);
void	 p_encap(struct sockaddr *, struct sockaddr *, int);
void	 p_protocol(struct sadb_protocol *, struct sockaddr *, struct
	     sadb_protocol *, int);
void	 p_sockaddr(struct sockaddr *, struct sockaddr *, int, int);
void	 p_flags(int, char *);
char	*routename4(in_addr_t);
char	*routename6(struct sockaddr_in6 *);
void	 index_pfk(struct sadb_msg *, void **);

/*
 * Print routing tables.
 */
void
p_rttables(int af, u_int tableid)
{
	struct rt_msghdr *rtm;
	struct sadb_msg *msg;
	char *buf = NULL, *next, *lim = NULL;
	size_t needed;
	int mib[7];
	struct sockaddr *sa;

	mib[0] = CTL_NET;
	mib[1] = PF_ROUTE;
	mib[2] = 0;
	mib[3] = af;
	mib[4] = NET_RT_DUMP;
	mib[5] = 0;
	mib[6] = tableid;

	if (sysctl(mib, 7, NULL, &needed, NULL, 0) < 0)
		err(1, "route-sysctl-estimate");
	if (needed > 0) {
		if ((buf = malloc(needed)) == 0)
			err(1, NULL);
		if (sysctl(mib, 7, buf, &needed, NULL, 0) < 0)
			err(1, "sysctl of routing table");
		lim = buf + needed;
	}

	printf("Routing tables\n");

	if (buf) {
		for (next = buf; next < lim; next += rtm->rtm_msglen) {
			rtm = (struct rt_msghdr *)next;
			if (rtm->rtm_version != RTM_VERSION)
				continue;
			sa = (struct sockaddr *)(rtm + 1);
			if (af != AF_UNSPEC && sa->sa_family != af)
				continue;
			p_rtentry(rtm);
		}
		free(buf);
		buf = NULL;
	}

	if (af != 0 && af != PF_KEY)
		return;

	mib[0] = CTL_NET;
	mib[1] = PF_KEY;
	mib[2] = PF_KEY_V2;
	mib[3] = NET_KEY_SPD_DUMP;
	mib[4] = mib[5] = 0;

	if (sysctl(mib, 4, NULL, &needed, NULL, 0) == -1) {
		if (errno == ENOPROTOOPT)
			return;
		err(1, "spd-sysctl-estimate");
	}
	if (needed > 0) {
		if ((buf = malloc(needed)) == 0)
			err(1, NULL);
		if (sysctl(mib, 4, buf, &needed, NULL, 0) == -1)
			err(1,"sysctl of spd");
		lim = buf + needed;
	}

	if (buf) {
		printf("\nEncap:\n");

		for (next = buf; next < lim; next += msg->sadb_msg_len *
		    PFKEYV2_CHUNK) {
			msg = (struct sadb_msg *)next;
			if (msg->sadb_msg_len == 0)
				break;
			p_pfkentry(msg);
		}
		free(buf);
		buf = NULL;
	}
}

/*
 * column widths; each followed by one space
 * width of destination/gateway column
 * strlen("fe80::aaaa:bbbb:cccc:dddd@gif0") == 30, strlen("/128") == 4
 */
#define	WID_GW(af)	((af) == AF_INET6 ? (nflag ? 30 : 18) : 18)

int
WID_DST(int af)
{

	if (nflag)
		switch (af) {
		case AF_INET6:
			return 34;
		default:
			return 18;
		}
	else
		switch (af) {
 		default:
			return 18;
		}
}

/*
 * Print header for routing table columns.
 */
void
pr_rthdr(int af, int Aflag)
{
	if (Aflag)
		printf("%-*.*s ", PLEN, PLEN, "Address");
	switch (af) {
	case PF_KEY:
		printf("%-18s %-5s %-18s %-5s %-5s %-22s\n",
		    "Source", "Port", "Destination",
		    "Port", "Proto", "SA(Address/Proto/Type/Direction)");
		break;
	default:
		printf("%-*.*s %-*.*s %-6.6s %5.5s %8.8s %5.5s  %4.4s %s\n",
		    WID_DST(af), WID_DST(af), "Destination",
		    WID_GW(af), WID_GW(af), "Gateway",
		    "Flags", "Refs", "Use", "Mtu", "Prio", "Iface");
		break;
	}
}

static void
get_rtaddrs(int addrs, struct sockaddr *sa, struct sockaddr **rti_info)
{
	int	i;

	for (i = 0; i < RTAX_MAX; i++) {
		if (addrs & (1 << i)) {
			rti_info[i] = sa;
			sa = (struct sockaddr *)((char *)(sa) +
			    ROUNDUP(sa->sa_len));
		} else
			rti_info[i] = NULL;
	}
}

/*
 * Print a routing table entry.
 */
void
p_rtentry(struct rt_msghdr *rtm)
{
	static int	 old_af = -1;
	struct sockaddr	*sa = (struct sockaddr *)(rtm + 1);
	struct sockaddr	*mask, *rti_info[RTAX_MAX];
	char		 ifbuf[IF_NAMESIZE];

	if (sa->sa_family == AF_KEY)
		return;

	get_rtaddrs(rtm->rtm_addrs, sa, rti_info);
	if (Fflag && rti_info[RTAX_GATEWAY]->sa_family != sa->sa_family) {
		return;
	}
	if (old_af != sa->sa_family) {
		old_af = sa->sa_family;
		pr_family(sa->sa_family);
		pr_rthdr(sa->sa_family, 0);
	}

	mask = rti_info[RTAX_NETMASK];
	if ((sa = rti_info[RTAX_DST]) == NULL)
		return;

	p_sockaddr(sa, mask, rtm->rtm_flags, WID_DST(sa->sa_family));
	p_sockaddr(rti_info[RTAX_GATEWAY], NULL, RTF_HOST,
	    WID_GW(sa->sa_family));
	p_flags(rtm->rtm_flags, "%-6.6s ");
	printf("%5u %8llu ", rtm->rtm_rmx.rmx_refcnt,
	    rtm->rtm_rmx.rmx_pksent);
	if (rtm->rtm_rmx.rmx_mtu)
		printf("%5u ", rtm->rtm_rmx.rmx_mtu);
	else
		printf("%5s ", "-");
	putchar((rtm->rtm_rmx.rmx_locks & RTV_MTU) ? 'L' : ' ');
	printf("  %2d %.16s", rtm->rtm_priority,
	    if_indextoname(rtm->rtm_index, ifbuf));
	putchar('\n');
}

/*
 * Print a pfkey/encap entry.
 */
void
p_pfkentry(struct sadb_msg *msg)
{
	static int		 old = 0;
	struct sadb_address	*saddr;
	struct sadb_protocol	*sap, *saft;
	struct sockaddr		*sa, *mask;
	void			*headers[SADB_EXT_MAX + 1];

	if (!old) {
		pr_rthdr(PF_KEY, 0);
		old++;
	}

	bzero(headers, sizeof(headers));
	index_pfk(msg, headers);

	/* These are always set */
	saddr = headers[SADB_X_EXT_SRC_FLOW];
	sa = (struct sockaddr *)(saddr + 1);
	saddr = headers[SADB_X_EXT_SRC_MASK];
	mask = (struct sockaddr *)(saddr + 1);
	p_encap(sa, mask, WID_DST(sa->sa_family));

	/* These are always set, too. */
	saddr = headers[SADB_X_EXT_DST_FLOW];
	sa = (struct sockaddr *)(saddr + 1);
	saddr = headers[SADB_X_EXT_DST_MASK];
	mask = (struct sockaddr *)(saddr + 1);
	p_encap(sa, mask, WID_DST(sa->sa_family));

	/* Bypass and deny flows do not set SADB_EXT_ADDRESS_DST! */
	sap = headers[SADB_X_EXT_PROTOCOL];
	saft = headers[SADB_X_EXT_FLOW_TYPE];
	saddr = headers[SADB_EXT_ADDRESS_DST];
	if (saddr)
		sa = (struct sockaddr *)(saddr + 1);
	else
		sa = NULL;
	p_protocol(sap, sa, saft, msg->sadb_msg_satype);

	printf("\n");
}

/*
 * Print address family header before a section of the routing table.
 */
void
pr_family(int af)
{
	char *afname;

	switch (af) {
	case AF_INET:
		afname = "Internet";
		break;
	case AF_INET6:
		afname = "Internet6";
		break;
	case PF_KEY:
		afname = "Encap";
		break;
	case AF_APPLETALK:
		afname = "AppleTalk";
		break;
	default:
		afname = NULL;
		break;
	}
	if (afname)
		printf("\n%s:\n", afname);
	else
		printf("\nProtocol Family %d:\n", af);
}

void
p_addr(struct sockaddr *sa, struct sockaddr *mask, int flags)
{
	p_sockaddr(sa, mask, flags, WID_DST(sa->sa_family));
}

void
p_gwaddr(struct sockaddr *sa, int af)
{
	p_sockaddr(sa, 0, RTF_HOST, WID_GW(af));
}

void
p_encap(struct sockaddr *sa, struct sockaddr *mask, int width)
{
	char		*cp;
	unsigned short	 port = 0;

	if (mask)
		cp = netname(sa, mask);
	else
		cp = routename(sa);
	switch (sa->sa_family) {
	case AF_INET:
		port = ntohs(((struct sockaddr_in *)sa)->sin_port);
		break;
	case AF_INET6:
		port = ntohs(((struct sockaddr_in6 *)sa)->sin6_port);
		break;
	}
	if (width < 0)
		printf("%s", cp);
	else {
		if (nflag)
			printf("%-*s %-5u ", width, cp, port);
		else
			printf("%-*.*s %-5u ", width, width, cp, port);
	}
}

void
p_protocol(struct sadb_protocol *sap, struct sockaddr *sa, struct sadb_protocol
    *saft, int proto)
{
	printf("%-6u", sap->sadb_protocol_proto);

	if (sa)
		p_sockaddr(sa, NULL, 0, -1);
	else
		printf("none");

	switch (proto) {
	case SADB_SATYPE_ESP:
		printf("/esp");
		break;
	case SADB_SATYPE_AH:
		printf("/ah");
		break;
	case SADB_X_SATYPE_IPCOMP:
		printf("/ipcomp");
		break;
	case SADB_X_SATYPE_IPIP:
		printf("/ipip");
		break;
	default:
		printf("/<unknown>");
	}

	switch(saft->sadb_protocol_proto) {
	case SADB_X_FLOW_TYPE_USE:
		printf("/use");
		break;
	case SADB_X_FLOW_TYPE_REQUIRE:
		printf("/require");
		break;
	case SADB_X_FLOW_TYPE_ACQUIRE:
		printf("/acquire");
		break;
	case SADB_X_FLOW_TYPE_DENY:
		printf("/deny");
		break;
	case SADB_X_FLOW_TYPE_BYPASS:
		printf("/bypass");
		break;
	case SADB_X_FLOW_TYPE_DONTACQ:
		printf("/dontacq");
		break;
	default:
		printf("/<unknown type>");
	}

	switch(saft->sadb_protocol_direction) {
	case IPSP_DIRECTION_IN:
		printf("/in");
		break;
	case IPSP_DIRECTION_OUT:
		printf("/out");
		break;
	default:
		printf("/<unknown>");
	}
}

void
p_sockaddr(struct sockaddr *sa, struct sockaddr *mask, int flags, int width)
{
	char *cp;

	switch (sa->sa_family) {
	case AF_INET6:
	    {
		struct sockaddr_in6 *sa6 = (struct sockaddr_in6 *)sa;
		struct in6_addr *in6 = &sa6->sin6_addr;

		/*
		 * XXX: This is a special workaround for KAME kernels.
		 * sin6_scope_id field of SA should be set in the future.
		 */
		if (IN6_IS_ADDR_LINKLOCAL(in6) ||
		    IN6_IS_ADDR_MC_LINKLOCAL(in6) ||
		    IN6_IS_ADDR_MC_INTFACELOCAL(in6)) {
			/* XXX: override is ok? */
			sa6->sin6_scope_id = (u_int32_t)ntohs(*(u_short *)
			    &in6->s6_addr[2]);
			*(u_short *)&in6->s6_addr[2] = 0;
		}
		if (flags & RTF_HOST)
			cp = routename((struct sockaddr *)sa6);
		else
			cp = netname((struct sockaddr *)sa6, mask);
		break;
	    }
	default:
		if ((flags & RTF_HOST) || mask == NULL)
			cp = routename(sa);
		else
			cp = netname(sa, mask);
		break;
	}
	if (width < 0)
		printf("%s", cp);
	else {
		if (nflag)
			printf("%-*s ", width, cp);
		else
			printf("%-*.*s ", width, width, cp);
	}
}

void
p_flags(int f, char *format)
{
	char name[33], *flags;
	const struct bits *p = bits;

	for (flags = name; p->b_mask && flags < &name[sizeof(name) - 2]; p++)
		if (p->b_mask & f)
			*flags++ = p->b_val;
	*flags = '\0';
	printf(format, name);
}

static char line[MAXHOSTNAMELEN];
static char domain[MAXHOSTNAMELEN];

char *
routename(struct sockaddr *sa)
{
	char *cp = NULL;
	static int first = 1;

	if (first) {
		first = 0;
		if (gethostname(domain, sizeof(domain)) == 0 &&
		    (cp = strchr(domain, '.')))
			(void)strlcpy(domain, cp + 1, sizeof(domain));
		else
			domain[0] = '\0';
		cp = NULL;
	}

	if (sa->sa_len == 0) {
		(void)strlcpy(line, "default", sizeof(line));
		return (line);
	}

	switch (sa->sa_family) {
	case AF_INET:
		return
		    (routename4(((struct sockaddr_in *)sa)->sin_addr.s_addr));

	case AF_INET6:
	    {
		struct sockaddr_in6 sin6;

		memset(&sin6, 0, sizeof(sin6));
		memcpy(&sin6, sa, sa->sa_len);
		sin6.sin6_len = sizeof(struct sockaddr_in6);
		sin6.sin6_family = AF_INET6;
		if (sa->sa_len == sizeof(struct sockaddr_in6) &&
		    (IN6_IS_ADDR_LINKLOCAL(&sin6.sin6_addr) ||
		     IN6_IS_ADDR_MC_LINKLOCAL(&sin6.sin6_addr) ||
		     IN6_IS_ADDR_MC_INTFACELOCAL(&sin6.sin6_addr)) &&
		    sin6.sin6_scope_id == 0) {
			sin6.sin6_scope_id =
			    ntohs(*(u_int16_t *)&sin6.sin6_addr.s6_addr[2]);
			sin6.sin6_addr.s6_addr[2] = 0;
			sin6.sin6_addr.s6_addr[3] = 0;
		}
		return (routename6(&sin6));
	    }

	case AF_LINK:
		return (link_print(sa));
	case AF_UNSPEC:
		if (sa->sa_len == sizeof(struct sockaddr_rtlabel)) {
			static char name[RTLABEL_LEN];
			struct sockaddr_rtlabel *sr;

			sr = (struct sockaddr_rtlabel *)sa;
			(void)strlcpy(name, sr->sr_label, sizeof(name));
			return (name);
		}
		/* FALLTHROUGH */
	default:
		(void)snprintf(line, sizeof(line), "(%d) %s",
		    sa->sa_family, any_ntoa(sa));
		break;
	}
	return (line);
}

char *
routename4(in_addr_t in)
{
	char		*cp = NULL;
	struct in_addr	 ina;
	struct hostent	*hp;

	if (in == INADDR_ANY)
		cp = "default";
	if (!cp && !nflag) {
		if ((hp = gethostbyaddr((char *)&in,
		    sizeof(in), AF_INET)) != NULL) {
			if ((cp = strchr(hp->h_name, '.')) &&
			    !strcmp(cp + 1, domain))
				*cp = '\0';
			cp = hp->h_name;
		}
	}
	ina.s_addr = in;
	strlcpy(line, cp ? cp : inet_ntoa(ina), sizeof(line));

	return (line);
}

char *
routename6(struct sockaddr_in6 *sin6)
{
	int	 niflags = 0;

	if (nflag)
		niflags |= NI_NUMERICHOST;
	else
		niflags |= NI_NOFQDN;

	if (getnameinfo((struct sockaddr *)sin6, sin6->sin6_len,
	    line, sizeof(line), NULL, 0, niflags) != 0)
		strncpy(line, "invalid", sizeof(line));

	return (line);
}

/*
 * Return the name of the network whose address is given.
 * The address is assumed to be that of a net or subnet, not a host.
 */
char *
netname4(in_addr_t in, in_addr_t mask)
{
	char *cp = NULL;
	struct netent *np = NULL;
	int mbits;

	in = ntohl(in);
	mask = ntohl(mask);
	if (!nflag && in != INADDR_ANY) {
		if ((np = getnetbyaddr(in, AF_INET)) != NULL)
			cp = np->n_name;
	}
	if (in == INADDR_ANY)
		cp = "default";
	mbits = mask ? 33 - ffs(mask) : 0;
	if (cp)
		strlcpy(line, cp, sizeof(line));
#define C(x)	((x) & 0xff)
	else if (mbits < 9)
		snprintf(line, sizeof(line), "%u/%d", C(in >> 24), mbits);
	else if (mbits < 17)
		snprintf(line, sizeof(line), "%u.%u/%d",
		    C(in >> 24) , C(in >> 16), mbits);
	else if (mbits < 25)
		snprintf(line, sizeof(line), "%u.%u.%u/%d",
		    C(in >> 24), C(in >> 16), C(in >> 8), mbits);
	else
		snprintf(line, sizeof(line), "%u.%u.%u.%u/%d", C(in >> 24),
		    C(in >> 16), C(in >> 8), C(in), mbits);
#undef C
	return (line);
}

char *
netname6(struct sockaddr_in6 *sa6, struct sockaddr_in6 *mask)
{
	struct sockaddr_in6 sin6;
	u_char *p;
	int masklen, final = 0, illegal = 0;
	int i, lim, flag, error;
	char hbuf[NI_MAXHOST];

	sin6 = *sa6;

	flag = 0;
	masklen = 0;
	if (mask) {
		lim = mask->sin6_len - offsetof(struct sockaddr_in6, sin6_addr);
		lim = lim < (int)sizeof(struct in6_addr) ?
		    lim : sizeof(struct in6_addr);
		for (p = (u_char *)&mask->sin6_addr, i = 0; i < lim; p++) {
			if (final && *p) {
				illegal++;
				sin6.sin6_addr.s6_addr[i++] = 0x00;
				continue;
			}

			switch (*p & 0xff) {
			case 0xff:
				masklen += 8;
				break;
			case 0xfe:
				masklen += 7;
				final++;
				break;
			case 0xfc:
				masklen += 6;
				final++;
				break;
			case 0xf8:
				masklen += 5;
				final++;
				break;
			case 0xf0:
				masklen += 4;
				final++;
				break;
			case 0xe0:
				masklen += 3;
				final++;
				break;
			case 0xc0:
				masklen += 2;
				final++;
				break;
			case 0x80:
				masklen += 1;
				final++;
				break;
			case 0x00:
				final++;
				break;
			default:
				final++;
				illegal++;
				break;
			}

			if (!illegal)
				sin6.sin6_addr.s6_addr[i++] &= *p;
			else
				sin6.sin6_addr.s6_addr[i++] = 0x00;
		}
		while (i < sizeof(struct in6_addr))
			sin6.sin6_addr.s6_addr[i++] = 0x00;
	} else
		masklen = 128;

	if (masklen == 0 && IN6_IS_ADDR_UNSPECIFIED(&sin6.sin6_addr))
		return ("default");

	if (illegal)
		warnx("illegal prefixlen");

	if (nflag)
		flag |= NI_NUMERICHOST;
	error = getnameinfo((struct sockaddr *)&sin6, sin6.sin6_len,
	    hbuf, sizeof(hbuf), NULL, 0, flag);
	if (error)
		snprintf(hbuf, sizeof(hbuf), "invalid");

	snprintf(line, sizeof(line), "%s/%d", hbuf, masklen);
	return (line);
}

/*
 * Return the name of the network whose address is given.
 * The address is assumed to be that of a net or subnet, not a host.
 */
char *
netname(struct sockaddr *sa, struct sockaddr *mask)
{
	switch (sa->sa_family) {

	case AF_INET:
		return netname4(((struct sockaddr_in *)sa)->sin_addr.s_addr,
		    ((struct sockaddr_in *)mask)->sin_addr.s_addr);
	case AF_INET6:
		return netname6((struct sockaddr_in6 *)sa,
		    (struct sockaddr_in6 *)mask);
	case AF_LINK:
		return (link_print(sa));
	default:
		snprintf(line, sizeof(line), "af %d: %s",
		    sa->sa_family, any_ntoa(sa));
		break;
	}
	return (line);
}

static const char hexlist[] = "0123456789abcdef";

char *
any_ntoa(const struct sockaddr *sa)
{
	static char obuf[240];
	const char *in = sa->sa_data;
	char *out = obuf;
	int len = sa->sa_len - offsetof(struct sockaddr, sa_data);

	*out++ = 'Q';
	do {
		*out++ = hexlist[(*in >> 4) & 15];
		*out++ = hexlist[(*in++)    & 15];
		*out++ = '.';
	} while (--len > 0 && (out + 3) < &obuf[sizeof(obuf) - 1]);
	out[-1] = '\0';
	return (obuf);
}

char *
link_print(struct sockaddr *sa)
{
	struct sockaddr_dl	*sdl = (struct sockaddr_dl *)sa;
	u_char			*lla = (u_char *)sdl->sdl_data + sdl->sdl_nlen;

	if (sdl->sdl_nlen == 0 && sdl->sdl_alen == 0 &&
	    sdl->sdl_slen == 0) {
		(void)snprintf(line, sizeof(line), "link#%d", sdl->sdl_index);
		return (line);
	}
	switch (sdl->sdl_type) {
	case IFT_ETHER:
	case IFT_CARP:
		return (ether_ntoa((struct ether_addr *)lla));
	default:
		return (link_ntoa(sdl));
	}
}

void
index_pfk(struct sadb_msg *msg, void **headers)
{
	struct sadb_ext	*ext;

	for (ext = (struct sadb_ext *)(msg + 1);
	    (size_t)((u_int8_t *)ext - (u_int8_t *)msg) <
	    msg->sadb_msg_len * PFKEYV2_CHUNK && ext->sadb_ext_len > 0;
	    ext = (struct sadb_ext *)((u_int8_t *)ext +
	    ext->sadb_ext_len * PFKEYV2_CHUNK)) {
		switch (ext->sadb_ext_type) {
		case SADB_EXT_ADDRESS_SRC:
			headers[SADB_EXT_ADDRESS_SRC] = (void *)ext;
			break;
		case SADB_EXT_ADDRESS_DST:
			headers[SADB_EXT_ADDRESS_DST] = (void *)ext;
			break;
		case SADB_X_EXT_PROTOCOL:
			headers[SADB_X_EXT_PROTOCOL] = (void *)ext;
			break;
		case SADB_X_EXT_SRC_FLOW:
			headers[SADB_X_EXT_SRC_FLOW] = (void *)ext;
			break;
		case SADB_X_EXT_DST_FLOW:
			headers[SADB_X_EXT_DST_FLOW] = (void *)ext;
			break;
		case SADB_X_EXT_SRC_MASK:
			headers[SADB_X_EXT_SRC_MASK] = (void *)ext;
			break;
		case SADB_X_EXT_DST_MASK:
			headers[SADB_X_EXT_DST_MASK] = (void *)ext;
			break;
		case SADB_X_EXT_FLOW_TYPE:
			headers[SADB_X_EXT_FLOW_TYPE] = (void *)ext;
		default:
			/* Ignore. */
			break;
		}
	}
}
