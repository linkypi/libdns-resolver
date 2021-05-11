#pragma once

#ifndef _MYEV_DNS_H_
#define _MYEV_DNS_H_

#pragma warning(disable:4996)

#include <event2/event-config.h>

/* Compatibility for possible missing IPv6 declarations */
#include "ipv6-internal.h"

#include <sys/types.h>

#ifdef EVENT__HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <getopt.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#include <event2/event.h>
#include <event2/dns.h>
#include <event2/dns_struct.h>
#include <event2/util.h>

#ifdef EVENT__HAVE_NETINET_IN6_H
#include <netinet/in6.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <map>

#define u32 ev_uint32_t
#define u8 ev_uint8_t

_declspec(dllexport) void resolve_dns(char* host_name);

typedef void (*DnsCallback) (int,int,int,int,char*,const char**);

extern "C" {
	__declspec(dllexport) void resolve(char* host, DnsCallback func);
}
#endif