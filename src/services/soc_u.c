/*
 * Copyright (C) 2014 - plutoo
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#ifdef _WIN32

// Target Vista or newer for inet_ntop

#define WINVER 0x0600
#define _WIN32_WINNT 0x0600

#include <winsock2.h>
#include <ws2tcpip.h>
#include <BaseTsd.h>

#ifndef __MINGW32__
typedef SSIZE_T ssize_t;
#endif

static WSADATA wsaData;
#define SOCKET_FAILED(s) ((s) == (SOCKET)NULL)

#define WSAEAGAIN    WSAEWOULDBLOCK

#define SHUT_RD   SD_RECEIVE
#define SHUT_WR   SD_SEND
#define SHUT_RDWR SD_BOTH

#define ERRNO(x)  WSA##x
#define GET_ERRNO WSAGetLastError()

#else

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>

typedef int SOCKET;
#define SOCKET_FAILED(s) ((s) < 0)
#define closesocket(s)   close(s)

#define ERRNO(x)  x
#define GET_ERRNO errno

#endif

#include <stdint.h>

#include "util.h"
#include "handles.h"
#include "mem.h"
#include "arm11.h"
#include "service_macros.h"

static u32 soc_shared_size = 0;
static u32 soc_shared_mem_handle = 0;

static void init_error_map(void);
static int translate_error(int error);
static int load_sockaddr(struct sockaddr *saddr, socklen_t *addrlen, uint32_t src);
static int save_sockaddr(struct sockaddr *saddr, socklen_t addrlen, uint32_t dst, socklen_t dstlen);

SERVICE_START(soc_u)

SERVICE_CMD(0x00010044)   //InitializeSockets
{
    DEBUG("InitializeSockets %08x %08x\n", CMD(1), CMD(5));
#ifdef _WIN32
    WSAStartup(MAKEWORD(1, 1), &wsaData);
#endif
    init_error_map();
    soc_shared_size       = CMD(1);
    soc_shared_mem_handle = CMD(5);
    RESP(1, 0);
    return 0;
}

SERVICE_CMD(0x00160000)   //Gethostid
{
    DEBUG("gethostid\n", CMD(1), CMD(2), CMD(3));

    struct addrinfo hints, *info;
    char   host[80];
    if (gethostname(host, sizeof(host)) != 0) {
        RESP(2, translate_error(GET_ERRNO));
        RESP(1, 0);
        return 0;
    }

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;

    int rc = getaddrinfo(host, NULL, &hints, &info);
    if (rc != 0) {
        int err;
        switch(rc) {
        case EAI_AGAIN:
            err = EAGAIN; break;

        case EAI_MEMORY:
            err = ENOMEM; break;

#ifdef EAI_SYSTEM
        case EAI_SYSTEM:
            err = errno;  break;
#endif

        default:
            err = ENOENT; break;
        }

        RESP(2, translate_error(err));
        RESP(1, 0);
        return 0;
    }

    if(info->ai_addr == NULL || info->ai_addr->sa_family != AF_INET) {
        RESP(2, translate_error(ENOENT));
        RESP(1, 0);
        freeaddrinfo(info);
        return 0;
    }

    struct in_addr *addr = &((struct sockaddr_in*)info->ai_addr)->sin_addr;
    char addrstr[INET_ADDRSTRLEN];
    if(inet_ntop(AF_INET, addr, addrstr, sizeof(addrstr)) == NULL) {
        RESP(2, translate_error(ENOENT));
        RESP(1, 0);
        freeaddrinfo(info);
        return 0;
    }
    DEBUG("hostid=%s\n", addrstr);
#ifdef _WIN32
    RESP(2, addr->S_un.S_addr);
#else
    RESP(2, addr->s_addr);
#endif
    RESP(1, 0);
    freeaddrinfo(info);
    return 0;
}

SERVICE_CMD(0x000200c2) //socket
{
    DEBUG("socket %d %d %d\n", CMD(1), CMD(2), CMD(3));

    u32 handle = handle_New(HANDLE_TYPE_SOCKET, 0);
    handleinfo* h = handle_Get(handle);
    if (h == NULL) {
        DEBUG("failed to get newly created Handle\n");
        RESP(2, translate_error(ENOMEM));
        RESP(1, 0);
        return 0;
    }

    SOCKET s = socket(CMD(1), CMD(2), CMD(3));
    if (SOCKET_FAILED(s)) {
        DEBUG("failed to get newly created SOCKET\n");
        RESP(2, translate_error(GET_ERRNO));
        RESP(1, 0);
        return 0;
    }
    h->misc_ptr[0] = (void*)(uintptr_t)s;

    RESP(2, handle - HANDLES_BASE); //bit(31) must not be set
    RESP(1, 0);
    return 0;
}

SERVICE_CMD(0x00050084) //bind
{
    DEBUG("bind %08X %08X %08X\n", CMD(1), CMD(2), CMD(6));

    struct sockaddr_storage addr;
    struct sockaddr         *saddr = (struct sockaddr*)&addr;
    socklen_t               addrlen = CMD(2);
    SOCKET                  s;

    handleinfo* h = handle_Get(CMD(1) + HANDLES_BASE); //bit(31) must not be set
    if (h == NULL) {
        DEBUG("failed to get Handle\n");
        RESP(2, translate_error(ERRNO(EBADF)));
        RESP(1, 0);
        return 0;
    }

    memset(&addr, 0, sizeof(addr));
    if (load_sockaddr(saddr, &addrlen, CMD(6)) != 0)
        return 0;

    s = (uintptr_t)h->misc_ptr[0];
    int rc = bind(s, saddr, addrlen);
    if (rc != 0) {
        RESP(2, translate_error(GET_ERRNO));
        RESP(1, 0);
        return 0;
    }

    RESP(2, 0);
    RESP(1, 0);
    return 0;
}

SERVICE_CMD(0x00030082) //listen
{
    DEBUG("listen %08X %08X\n", CMD(1), CMD(2));

    SOCKET s;

    handleinfo* h = handle_Get(CMD(1) + HANDLES_BASE); //bit(31) must not be set
    if (h == NULL) {
        DEBUG("failed to get Handle\n");
        RESP(2, translate_error(ERRNO(EBADF)));
        RESP(1, 0);
        return 0;
    }

    s = (uintptr_t)h->misc_ptr[0];
    int rc = listen(s, CMD(2));
    if (rc != 0) {
        RESP(2, translate_error(GET_ERRNO));
        RESP(1, 0);
        return 0;
    }

    RESP(2, 0);
    RESP(1, 0);
    return 0;
}

SERVICE_CMD(0x00060084) //connect
{
    DEBUG("connect %08X %08X %08X\n", CMD(1), CMD(2), CMD(6));

    struct sockaddr_storage addr;
    struct sockaddr         *saddr = (struct sockaddr*)&addr;
    socklen_t               addrlen = CMD(2);
    SOCKET                  s;

    handleinfo* h = handle_Get(CMD(1) + HANDLES_BASE); //bit(31) must not be set
    if (h == NULL) {
        DEBUG("failed to get Handle\n");
        RESP(2, translate_error(ERRNO(EBADF)));
        RESP(1, 0);
        return 0;
    }

    memset(&addr, 0, sizeof(addr));
    if (load_sockaddr(saddr, &addrlen, CMD(6)) != 0)
        return 0;

    s = (uintptr_t)h->misc_ptr[0];
    int rc = connect(s, saddr, addrlen);
    if (rc != 0) {
        RESP(2, translate_error(GET_ERRNO));
        RESP(1, 0);
        return 0;
    }

    RESP(2, 0);
    RESP(1, 0);
    return 0;
}

SERVICE_CMD(0x000B0042) //close
{
    DEBUG("close %08X --todo--\n", CMD(1));

    SOCKET s;

    handleinfo* h = handle_Get(CMD(1) + HANDLES_BASE); //bit(31) must not be set
    if (h == NULL) {
        DEBUG("failed to get Handle\n");
        RESP(2, translate_error(ERRNO(EBADF)));
        RESP(1, 0);
        return 0;
    }

    s = (uintptr_t)h->misc_ptr[0];
    int rc = closesocket(s);
    if (rc != 0) {
        RESP(2, translate_error(GET_ERRNO));
        RESP(1, 0);
        return 0;
    }

    RESP(2, 0);
    RESP(1, 0);

    /* TODO cleanup h */
    return 0;
}

SERVICE_CMD(0x00040082) //accept
{
    DEBUG("accept %08X %08X %08X\n", CMD(1), CMD(2), EXTENDED_CMD(1));

    struct sockaddr_storage addr;
    struct sockaddr         *saddr = (struct sockaddr*)&addr;
    socklen_t               addrlen = sizeof(addr);
    SOCKET                  s;

    handleinfo* h = handle_Get(CMD(1) + HANDLES_BASE); //bit(31) must not be set
    if (h == NULL) {
        DEBUG("failed to get Handle\n");
        RESP(2, translate_error(ERRNO(EBADF)));
        RESP(1, 0);
        return 0;
    }

    u32 handle = handle_New(HANDLE_TYPE_SOCKET, 0);
    handleinfo* ha = handle_Get(handle);
    if (ha == NULL) {
        DEBUG("failed to get newly created Handle\n");
        RESP(2, translate_error(ENOMEM));
        RESP(1, 0);
        return 0;
    }

    s = (uintptr_t)h->misc_ptr[0];
    s = accept(s, saddr, &addrlen);
    if (SOCKET_FAILED(s)) {
        DEBUG("failed to get newly created SOCKET\n");
        RESP(2, translate_error(GET_ERRNO));
        RESP(1, 0);
        return 0;
    }
    ha->misc_ptr[0] = (void*)(uintptr_t)s;

    if (save_sockaddr(saddr, addrlen, EXTENDED_CMD(1), CMD(2)) != 0) {
        shutdown(s, SHUT_RDWR);
        closesocket(s);
        /* TODO cleanup ha */
        return 0;
    }

    RESP(2, handle - HANDLES_BASE); //bit(31) must not be set
    RESP(1, 0);
    return 0;
}

SERVICE_CMD(0x00090106) //sendto_other
{
    DEBUG("sendto_other %08X %08X %08X %08X %08X %08X\n", CMD(1), CMD(2), CMD(3), CMD(4), CMD(8), CMD(10));

    struct sockaddr_storage addr;
    struct sockaddr         *saddr = (struct sockaddr*)&addr;
    socklen_t               addrlen = CMD(4);
    SOCKET                  s;

    handleinfo* h = handle_Get(CMD(1) + HANDLES_BASE); //bit(31) must not be set
    if (h == NULL) {
        DEBUG("failed to get Handle\n");
        RESP(2, translate_error(ERRNO(EBADF)));
        RESP(1, 0);
        return 0;
    }

    u8 *buffer = (u8*)malloc(CMD(2));
    if (buffer == NULL) {
        ERROR("Not enough mem.\n");
        RESP(2, translate_error(ENOMEM));
        RESP(1, 0);
        return 0;
    }

    if (mem_Read(buffer, CMD(10), CMD(2)) != 0) {
        ERROR("mem_Read failed.\n");
        RESP(2, translate_error(ERRNO(EFAULT)));
        RESP(1, 0);
        free(buffer);
        return 0;
    }

    memset(&addr, 0, sizeof(addr));
    if (addrlen > 0 && load_sockaddr(saddr, &addrlen, CMD(8)) != 0) {
        free(buffer);
        return 0;
    }

    s = (uintptr_t)h->misc_ptr[0];
    ssize_t rc;
    if (addrlen > 0)
        rc = sendto(s, buffer, CMD(2), CMD(3), saddr, addrlen);
    else
        rc = send(s, buffer, CMD(2), CMD(3));
    if (rc < 0) {
        RESP(2, translate_error(GET_ERRNO));
        RESP(1, 0);
        free(buffer);
        return 0;
    }

    RESP(2, 0);
    RESP(1, (uint32_t)rc);
    free(buffer);
    return 0;
}

SERVICE_CMD(0x000a0106) //sendto
{
    DEBUG("sendto %08X %08X %08X %08X %08X %08X\n", CMD(1), CMD(2), CMD(3), CMD(4), CMD(8), CMD(10));

    struct sockaddr_storage addr;
    struct sockaddr         *saddr = (struct sockaddr*)&addr;
    socklen_t               addrlen = CMD(4);
    SOCKET                  s;

    handleinfo* h = handle_Get(CMD(1) + HANDLES_BASE); //bit(31) must not be set
    if (h == NULL) {
        DEBUG("failed to get Handle\n");
        RESP(2, translate_error(ERRNO(EBADF)));
        RESP(1, 0);
        return 0;
    }

    u8 *buffer = (u8*)malloc(CMD(2));
    if (buffer == NULL) {
        ERROR("Not enough mem.\n");
        RESP(2, translate_error(ENOMEM));
        RESP(1, 0);
        return 0;
    }

    if (mem_Read(buffer, CMD(8), CMD(2)) != 0) {
        ERROR("mem_Read failed.\n");
        RESP(2, translate_error(ERRNO(EFAULT)));
        RESP(1, 0);
        free(buffer);
        return 0;
    }

    memset(&addr, 0, sizeof(addr));
    if (addrlen > 0 && load_sockaddr(saddr, &addrlen, CMD(10)) != 0) {
        free(buffer);
        return 0;
    }

    s = (uintptr_t)h->misc_ptr[0];
    ssize_t rc;
    if (addrlen > 0)
        rc = sendto(s, buffer, CMD(2), CMD(3), saddr, addrlen);
    else
        rc = send(s, buffer, CMD(2), CMD(3));
    if (rc < 0) {
        RESP(2, translate_error(GET_ERRNO));
        RESP(1, 0);
        free(buffer);
        return 0;
    }

    RESP(2, 0);
    RESP(1, (uint32_t)rc);
    free(buffer);
    return 0;
}

SERVICE_CMD(0x00070104) //recvfrom_other
{
    DEBUG("recvfrom_other %08X %08X %08X %08X %08X %08X\n", CMD(1), CMD(2), CMD(3), CMD(4), CMD(8), EXTENDED_CMD(1));

    struct sockaddr_storage addr;
    struct sockaddr         *saddr = (struct sockaddr*)&addr;
    socklen_t               addrlen = CMD(4);
    SOCKET                  s;

    handleinfo* h = handle_Get(CMD(1) + HANDLES_BASE); //bit(31) must not be set
    if (h == NULL) {
        DEBUG("failed to get Handle\n");
        RESP(2, translate_error(ERRNO(EBADF)));
        RESP(1, 0);
        return 0;
    }

    u8 *buffer = (u8*)malloc(CMD(2));
    if (buffer == NULL) {
        ERROR("Not enough mem.\n");
        RESP(2, translate_error(ENOMEM));
        RESP(1, 0);
        return 0;
    }

    memset(&addr, 0, sizeof(addr));
    if (addrlen > 0 && load_sockaddr(saddr, &addrlen, EXTENDED_CMD(1)) != 0) {
        free(buffer);
        return 0;
    }

    s = (uintptr_t)h->misc_ptr[0];
    ssize_t rc;
    if (addrlen > 0)
        rc = recvfrom(s, buffer, CMD(2), CMD(3), saddr, &addrlen);
    else
        rc = recv(s, buffer, CMD(2), CMD(3));
    if (rc < 0) {
        ERROR("recvfrom failed.\n");
        RESP(2, translate_error(GET_ERRNO));
        RESP(1, 0);
        free(buffer);
        return 0;
    }

    if (mem_Write(buffer, CMD(8), (uint32_t)rc) != 0) {
        ERROR("mem_Write failed.\n");
        RESP(2, translate_error(ERRNO(EFAULT)));
        RESP(1, 0);
        free(buffer);
        return 0;
    }

    if (save_sockaddr(saddr, addrlen, EXTENDED_CMD(1), CMD(4)) != 0) {
        free(buffer);
        return 0;
    }

    RESP(2, 0);
    RESP(1, (uint32_t)rc);
    free(buffer);
    return 0;
}

SERVICE_CMD(0x00080102) //recvfrom
{
    DEBUG("recvfrom %08X %08X %08X %08X %08X %08X\n", CMD(1), CMD(2), CMD(3), CMD(4), EXTENDED_CMD(1), EXTENDED_CMD(3));

    struct sockaddr_storage addr;
    struct sockaddr         *saddr = (struct sockaddr*)&addr;
    socklen_t               addrlen = CMD(4);
    SOCKET                  s;

    handleinfo* h = handle_Get(CMD(1) + HANDLES_BASE); //bit(31) must not be set
    if (h == NULL) {
        DEBUG("failed to get Handle\n");
        RESP(2, translate_error(ERRNO(EBADF)));
        RESP(1, 0);
        return 0;
    }

    u8 *buffer = (u8*)malloc(CMD(2));
    if (buffer == NULL) {
        ERROR("Not enough mem.\n");
        RESP(2, translate_error(ENOMEM));
        RESP(1, 0);
        return 0;
    }

    if (addrlen > 0 && load_sockaddr(saddr, &addrlen, EXTENDED_CMD(3)) != 0) {
        free(buffer);
        return 0;
    }

    s = (uintptr_t)h->misc_ptr[0];
    ssize_t rc;
    if (addrlen > 0)
        rc = recvfrom(s, buffer, CMD(2), CMD(3), saddr, &addrlen);
    else
        rc = recv(s, buffer, CMD(2), CMD(3));
    if (rc < 0) {
        ERROR("recvfrom failed.\n");
        RESP(2, translate_error(GET_ERRNO));
        RESP(1, 0);
        free(buffer);
        return 0;
    }

    if (mem_Write(buffer, CMD(1), (uint32_t)rc) != 0) {
        ERROR("mem_Write failed.\n");
        RESP(2, translate_error(ERRNO(EFAULT)));
        RESP(1, 0);
        free(buffer);
        return 0;
    }

    if (save_sockaddr(saddr, addrlen, EXTENDED_CMD(3), CMD(4)) != 0) {
        free(buffer);
        return 0;
    }

    RESP(2, 0);
    RESP(1, (uint32_t)rc);
    free(buffer);
    return 0;

}
SERVICE_CMD(0x001A00C0)   //ShutdownSockets
{
    DEBUG("ShutdownSockets");
    RESP(1, 0); //result code
    return 0;
}

SERVICE_CMD(0x00190000)   //GetNetworkOpt
{
    DEBUG("GetNetworkOpt %08x %08x %08x --STUB--", CMD(1), CMD(2), CMD(3));
    RESP(3, 6); //Output optlen 
    RESP(2, 0); //POSIX
    RESP(1, 0); //result code
    return 0;
}

SERVICE_CMD(0x100100C0)   // ???, seems to crash if the result is 0
{
	u32 unk1 = CMD(1);
	u32 unk2 = CMD(2);
	u32 unk3 = CMD(3);
	u32 unk4 = CMD(4);
	u32 unk5 = CMD(5);
	u32 unk6 = CMD(6);
	u32 unk7 = CMD(7);
	DEBUG("100100C0 %08x %08x %08x %08x %08x %02x %08x\n", unk1, unk2, unk3, unk4, unk5, unk6, unk7);

	RESP(1, -1); // Result
	return 0;
}

SERVICE_CMD(0x10030142)   // ???
{
	u32 unk1 = CMD(1);
	u32 unk2 = CMD(2);
	u32 unk3 = CMD(3);
	u32 unk4 = CMD(4);
	u32 unk5 = CMD(5);
	u32 unk6 = CMD(6);
	u32 unk7 = CMD(7);
	DEBUG("10030142 %08x %08x %08x %08x %08x %02x %08x\n", unk1, unk2, unk3, unk4, unk5, unk6, unk7);

	RESP(1, 0); // Result
	return 0;
}

SERVICE_CMD(0x10050084)   // ???
{
	u32 unk1 = CMD(1);
	u32 unk2 = CMD(2);
	u32 unk3 = CMD(3);
	u32 unk4 = CMD(4);
	u32 unk5 = CMD(5);
	u32 unk6 = CMD(6);
	u32 unk7 = CMD(7);
	DEBUG("10050084 %08x %08x %08x %08x %08x %02x %08x\n", unk1, unk2, unk3, unk4, unk5, unk6, unk7);

	RESP(1, 0); // Result
	return 0;
}

SERVICE_CMD(0x10070102)   // ???
{
	u32 unk1 = CMD(1);
	u32 unk2 = CMD(2);
	u32 unk3 = CMD(3);
	u32 unk4 = CMD(4);
	u32 unk5 = CMD(5);
	u32 unk6 = CMD(6);
	u32 unk7 = CMD(7);
	DEBUG("10070102 %08x %08x %08x %08x %08x %02x %08x\n", unk1, unk2, unk3, unk4, unk5, unk6, unk7);

	RESP(1, 0); // Result
	return 0;
}

SERVICE_END()


static int load_sockaddr(struct sockaddr *saddr, socklen_t *addrlen, uint32_t src)
{
    uint8_t addrbuf[0x1C];

    if (*addrlen < 2) {
        RESP(2, translate_error(ERRNO(EINVAL)));
        RESP(1, 0);
        return -1;
    }

    if (mem_Read(addrbuf, src, 2) != 0) {
        RESP(2, translate_error(ERRNO(EFAULT)));
        RESP(1, 0);
        return -1;
    }

    if (addrbuf[1] == 2) {
        /* AF_INET */
        if (mem_Read(addrbuf, src, 0x8) != 0) {
            RESP(2, translate_error(ERRNO(EFAULT)));
            RESP(1, 0);
            return -1;
        }
        saddr->sa_family = AF_INET;
        memcpy(saddr->sa_data, &addrbuf[2], 0x6);
        *addrlen = sizeof(struct sockaddr_in);
    }
    else {
        DEBUG("unknown protocol family %u\n", addrbuf[1]);
        RESP(2, translate_error(ERRNO(EINVAL)));
        RESP(1, 0);
        return -1;
    }

    return 0;
}

static int save_sockaddr(struct sockaddr *saddr, socklen_t addrlen, uint32_t dst, socklen_t dstlen)
{
    uint8_t addrbuf[0x1C];

    memcpy(&addrbuf[2], saddr->sa_data, addrlen-2);
    addrbuf[0] = addrlen;
    addrbuf[1] = (uint8_t)saddr->sa_family; /* I hope this data truncation doesn't break anything */

    if (mem_Write(addrbuf, dst, dstlen < addrlen ? dstlen : addrlen) != 0) {
      RESP(2, translate_error(ERRNO(EFAULT)));
      RESP(1, 0);
      return -1;
    }

    return 0;
}

typedef struct error_map_t {
    int from;
    int to;
} error_map_t;

static error_map_t error_map[] = {
    { 0,                       0, },
    { E2BIG,                   1, },
    { ERRNO(EACCES),           2, },
    { ERRNO(EADDRINUSE),       3, },
    { ERRNO(EADDRNOTAVAIL),    4, },
    { ERRNO(EAFNOSUPPORT),     5, },
    { ERRNO(EAGAIN),           6, },
    { ERRNO(EALREADY),         7, },
    { ERRNO(EBADF),            8, },
    { EBADMSG,                 9, },
    { EBUSY,                  10, },
    { ECANCELED,              11, },
    { ECHILD,                 12, },
    { ERRNO(ECONNABORTED),    13, },
    { ERRNO(ECONNREFUSED),    14, },
    { ERRNO(ECONNRESET),      15, },
    { EDEADLK,                16, },
    { ERRNO(EDESTADDRREQ),    17, },
    { EDOM,                   18, },
    { ERRNO(EDQUOT),          19, },
    { EEXIST,                 20, },
    { ERRNO(EFAULT),          21, },
    { EFBIG,                  22, },
    { ERRNO(EHOSTUNREACH),    23, },
    { EIDRM,                  24, },
    { EILSEQ,                 25, },
    { ERRNO(EINPROGRESS),     26, },
    { ERRNO(EINTR),           27, },
    { ERRNO(EINVAL),          28, },
    { EIO,                    29, },
    { ERRNO(EISCONN),         30, },
    { EISDIR,                 31, },
    { ERRNO(ELOOP),           32, },
    { ERRNO(EMFILE),          33, },
    { EMLINK,                 34, },
    { ERRNO(EMSGSIZE),        35, },
#ifndef _WIN32
    { ERRNO(EMULTIHOP),       36, },
#endif
    { ERRNO(ENAMETOOLONG),    37, },
    { ERRNO(ENETDOWN),        38, },
    { ERRNO(ENETRESET),       39, },
    { ERRNO(ENETUNREACH),     40, },
    { ENFILE,                 41, },
    { ERRNO(ENOBUFS),         42, },
    { ENODATA,                43, },
    { ENODEV,                 44, },
    { ENOENT,                 45, },
    { ENOEXEC,                46, },
    { ENOLCK,                 47, },
    { ENOLINK,                48, },
    { ENOMEM,                 49, },
    { ENOMSG,                 50, },
    { ERRNO(ENOPROTOOPT),     51, },
    { ENOSPC,                 52, },
    { ENOSR,                  53, },
    { ENOSTR,                 54, },
    { ENOSYS,                 55, },
    { ERRNO(ENOTCONN),        56, },
    { ENOTDIR,                57, },
    { ERRNO(ENOTEMPTY),       58, },
    { ERRNO(ENOTSOCK),        59, },
    { ENOTSUP,                60, },
    { ENOTTY,                 61, },
    { ENXIO,                  62, },
    { ERRNO(EOPNOTSUPP),      63, },
    { EOVERFLOW,              64, },
    { EPERM,                  65, },
    { EPIPE,                  66, },
    { EPROTO,                 67, },
    { ERRNO(EPROTONOSUPPORT), 68, },
    { ERRNO(EPROTOTYPE),      69, },
    { ERANGE,                 70, },
    { EROFS,                  71, },
    { ESPIPE,                 72, },
    { ESRCH,                  73, },
    { ERRNO(ESTALE),          74, },
    { ETIME,                  75, },
    { ERRNO(ETIMEDOUT),       76, },
};
static const size_t num_error_map = sizeof(error_map)/sizeof(error_map[0]);

static int error_map_cmp(const void *p1, const void *p2)
{
    error_map_t *l = (error_map_t*)p1;
    error_map_t *r = (error_map_t*)p2;

    if (l->from < r->from)
        return -1;
    if (l->from > r->from)
        return 1;

    return 0;
}

static void init_error_map(void)
{
    qsort(error_map, num_error_map, sizeof(error_map_t), error_map_cmp);
}

static int translate_error(int error)
{
    error_map_t key;
    key.from = error;

    void *result = bsearch(&key, error_map, num_error_map, sizeof(error_map_t), error_map_cmp);
    if (result != NULL)
        return -((error_map_t*)result)->to;
    return error;
}
