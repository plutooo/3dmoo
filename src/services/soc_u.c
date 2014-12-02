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

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include "util.h"
#include "handles.h"
#include "mem.h"
#include "arm11.h"
#include "service_macros.h"

#ifdef _WIN32
#include <winsock.h>
WSADATA wsaData;
#define SOCKET_FAILED(s) (s == NULL)
#else
typedef int SOCKET;
#define SOCKET_FAILED(s) (s < 0)
#define closesocket(s)   close(s)
#endif
static u32 soc_shared_size = 0;
static u32 soc_shared_mem_handle = 0;

static int translate_error(int error);

SERVICE_START(soc_u)

SERVICE_CMD(0x00010044)   //InitializeSockets
{
    DEBUG("InitializeSockets %08x %08x\n", CMD(1), CMD(5));
#ifdef _WIN32
    WSAStartup(MAKEWORD(1, 1), &wsaData);
#endif
    soc_shared_size       = CMD(1);
    soc_shared_mem_handle = CMD(5);
    RESP(1, 0);
    return 0;
}

SERVICE_CMD(0x00160000)   //Gethostid
{
    DEBUG("gethostid\n", CMD(1), CMD(2), CMD(3));

    char ac[80];
    if (gethostname(ac, sizeof(ac)) != 0) {
        RESP(2, translate_error(errno));
        RESP(1, 0);
        return 0;
    }

    struct hostent *phe = gethostbyname(ac);
    if (phe == NULL) {
        /* TODO better error */
        RESP(2, translate_error(ENOENT));
        RESP(1, 0);
        return 0;
    }


    struct in_addr *addr = (struct in_addr*)phe->h_addr_list[0];
    ERROR("hostid=%s\n", inet_ntoa(*addr));
#ifdef _WIN32
    RESP(2, addr->S_un.S_addr);
#else
    RESP(2, addr->s_addr);
#endif
    RESP(1, 0);
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
        RESP(2, translate_error(errno));
        RESP(1, 0);
        return 0;
    }
    h->misc_ptr[0] = (void*)(long)s;

    RESP(2, handle - HANDLES_BASE); //bit(31) must not be set
    RESP(1, 0);
    return 0;
}

SERVICE_CMD(0x00050084) //bind
{
    DEBUG("bind %08X %08X %08X\n", CMD(1), CMD(2), CMD(6));

    uint8_t                 addrbuf[0x1C];
    struct sockaddr_storage addr;
    struct sockaddr         *saddr = (struct sockaddr*)&addr;
    socklen_t               addrlen = CMD(2);

    handleinfo* h = handle_Get(CMD(1) + HANDLES_BASE); //bit(31) must not be set
    if (h == NULL) {
        DEBUG("failed to get Handle\n");
        RESP(2, translate_error(EBADF));
        RESP(1, 0);
        return 0;
    }

    memset(&addr, 0, sizeof(addr));
    if (addrlen == 0x8) {
        if (mem_Read(addrbuf, CMD(6), addrlen) != 0) {
            RESP(2, translate_error(EFAULT));
            RESP(1, 0);
            return 0;
        }
        saddr->sa_family = addrbuf[1];
        memcpy(saddr->sa_data, &addrbuf[2], 0x6);
    }
    else if (addrlen == 0x1C) {
        if (mem_Read((uint8_t*)&addr, CMD(6), addrlen) != 0) {
            RESP(2, translate_error(EFAULT));
            RESP(1, 0);
            return 0;
        }
        /* this size of 0xE seems bogus */
        saddr->sa_family = addrbuf[1];
        memcpy(saddr->sa_data, &addrbuf[2], 0xE);
        addrlen = 0x10;
    }
    else {
        DEBUG("unknown len\n");
        RESP(2, translate_error(EINVAL));
        RESP(1, 0);
        return 0;
    }

    SOCKET s  = (long)h->misc_ptr[0];
    int    rc = bind(s, saddr, addrlen);
    if (rc != 0) {
        RESP(2, translate_error(errno));
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

    handleinfo* h = handle_Get(CMD(1) + HANDLES_BASE); //bit(31) must not be set
    if (h == NULL) {
        DEBUG("failed to get Handle\n");
        RESP(2, translate_error(EBADF));
        RESP(1, 0);
        return 0;
    }

    SOCKET s  = (long)h->misc_ptr[0];
    int    rc = listen(s, CMD(2));
    if (rc != 0) {
        RESP(2, translate_error(errno));
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

    handleinfo* h = handle_Get(CMD(1) + HANDLES_BASE); //bit(31) must not be set
    if (h == NULL) {
        DEBUG("failed to get Handle\n");
        RESP(2, translate_error(EBADF));
        RESP(1, 0);
        return 0;
    }

    SOCKET s = (long)h->misc_ptr[0];
    int    rc = closesocket(s);
    if (rc != 0) {
        RESP(2, translate_error(errno));
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

    handleinfo* h = handle_Get(CMD(1) + HANDLES_BASE); //bit(31) must not be set
    if (h == NULL) {
        DEBUG("failed to get Handle\n");
        RESP(2, translate_error(EBADF));
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

    SOCKET s = (long)h->misc_ptr[0];
    s = accept(s, saddr, &addrlen);
    if (SOCKET_FAILED(s)) {
        DEBUG("failed to get newly created SOCKET\n");
        RESP(2, translate_error(errno));
        RESP(1, 0);
        return 0;
    }
    ha->misc_ptr[0] = (void*)(long)s;

    if (mem_Write((uint8_t*)saddr, EXTENDED_CMD(1), CMD(2) > addrlen ? addrlen : CMD(2)) != 0) {
      RESP(2, translate_error(EFAULT));
      RESP(1, 0);
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

    uint8_t                 addrbuf[0x1C];
    struct sockaddr_storage addr;
    struct sockaddr         *saddr = (struct sockaddr*)&addr;
    socklen_t               addrlen = CMD(4);

    handleinfo* h = handle_Get(CMD(1) + HANDLES_BASE); //bit(31) must not be set
    if (h == NULL) {
        DEBUG("failed to get Handle\n");
        RESP(2, translate_error(EBADF));
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
        RESP(2, translate_error(EFAULT));
        RESP(1, 0);
        free(buffer);
        return 0;
    }

    if (addrlen == 0x8) {
        if (mem_Read(addrbuf, CMD(8), addrlen) != 0) {
            RESP(2, translate_error(EFAULT));
            RESP(1, 0);
            free(buffer);
            return 0;
        }
        saddr->sa_family = addrbuf[1];
        memcpy(saddr->sa_data, &addrbuf[2], 0x6);
    }
    else if (addrlen == 0x1C) {
        if (mem_Read((uint8_t*)&addr, CMD(8), addrlen) != 0) {
            RESP(2, translate_error(EFAULT));
            RESP(1, 0);
            free(buffer);
            return 0;
        }
        /* this size of 0xE seems bogus */
        saddr->sa_family = addrbuf[1];
        memcpy(saddr->sa_data, &addrbuf[2], 0xE);
        addrlen = 0x10;
    }
    else {
        DEBUG("unknown len\n");
        RESP(2, translate_error(EINVAL));
        RESP(1, 0);
        free(buffer);
        return 0;
    }

    // TODO detect that we are doing send()
    SOCKET  s  = (long)h->misc_ptr[0];
    ssize_t rc = sendto(s, buffer, CMD(2), CMD(3), saddr, addrlen);
    if (rc < 0) {
        RESP(2, translate_error(errno));
        RESP(1, 0);
        free(buffer);
        return 0;
    }

    RESP(2, 0);
    RESP(1, rc);
    free(buffer);
    return 0;
}

SERVICE_CMD(0x000a0106) //sendto
{
    DEBUG("sendto %08X %08X %08X %08X %08X %08X\n", CMD(1), CMD(2), CMD(3), CMD(4), CMD(8), CMD(10));

    uint8_t                 addrbuf[0x1C];
    struct sockaddr_storage addr;
    struct sockaddr         *saddr = (struct sockaddr*)&addr;
    socklen_t               addrlen = CMD(4);

    handleinfo* h = handle_Get(CMD(1) + HANDLES_BASE); //bit(31) must not be set
    if (h == NULL) {
        DEBUG("failed to get Handle\n");
        RESP(2, translate_error(EBADF));
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
        RESP(2, translate_error(EFAULT));
        RESP(1, 0);
        free(buffer);
        return 0;
    }

    if (addrlen == 0x8) {
        if (mem_Read(addrbuf, CMD(10), addrlen) != 0) {
            RESP(2, translate_error(EFAULT));
            RESP(1, 0);
            free(buffer);
            return 0;
        }
        saddr->sa_family = addrbuf[1];
        memcpy(saddr->sa_data, &addrbuf[2], 0x6);
    }
    else if (addrlen == 0x1C) {
        if (mem_Read((uint8_t*)&addr, CMD(10), addrlen) != 0) {
            RESP(2, translate_error(EFAULT));
            RESP(1, 0);
            free(buffer);
            return 0;
        }
        /* this size of 0xE seems bogus */
        saddr->sa_family = addrbuf[1];
        memcpy(saddr->sa_data, &addrbuf[2], 0xE);
        addrlen = 0x10;
    }
    else {
        DEBUG("unknown len\n");
        RESP(2, translate_error(EINVAL));
        RESP(1, 0);
        free(buffer);
        return 0;
    }

    // TODO detect that we are doing send()
    SOCKET  s  = (long)h->misc_ptr[0];
    ssize_t rc = sendto(s, buffer, CMD(2), CMD(3), saddr, addrlen);
    if (rc < 0) {
        RESP(2, translate_error(errno));
        RESP(1, 0);
        free(buffer);
        return 0;
    }

    RESP(2, 0);
    RESP(1, rc);
    free(buffer);
    return 0;
}

SERVICE_CMD(0x00070104) //recvfrom_other
{
    DEBUG("recvfrom_other %08X %08X %08X %08X %08X %08X\n", CMD(1), CMD(2), CMD(3), CMD(4), CMD(8), EXTENDED_CMD(1));

    uint8_t                 addrbuf[0x1C];
    struct sockaddr_storage addr;
    struct sockaddr         *saddr = (struct sockaddr*)&addr;
    socklen_t               addrlen = CMD(4);

    handleinfo* h = handle_Get(CMD(1) + HANDLES_BASE); //bit(31) must not be set
    if (h == NULL) {
        DEBUG("failed to get Handle\n");
        RESP(2, translate_error(EBADF));
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

    if (addrlen == 0x8) {
        if (mem_Read(addrbuf, EXTENDED_CMD(1), addrlen) != 0) {
            RESP(2, translate_error(EFAULT));
            RESP(1, 0);
            free(buffer);
            return 0;
        }
        saddr->sa_family = addrbuf[1];
        memcpy(saddr->sa_data, &addrbuf[2], 0x6);
    }
    else if (addrlen == 0x1C) {
        if (mem_Read(addrbuf, EXTENDED_CMD(1), addrlen) != 0) {
            RESP(2, translate_error(EFAULT));
            RESP(1, 0);
            free(buffer);
            return 0;
        }
        /* this size of 0xE seems bogus */
        saddr->sa_family = addrbuf[1];
        memcpy(saddr->sa_data, &addrbuf[2], 0xE);
        addrlen = 0x10;
    }
    else {
        DEBUG("unknown len\n");
        RESP(2, translate_error(errno));
        RESP(1, 0);
        free(buffer);
        return 0;
    }

    // TODO detect that we are doing recv()
    SOCKET s = (long)h->misc_ptr[0];
    ssize_t rc = recvfrom(s, buffer, CMD(2), CMD(3), saddr, &addrlen);
    if (rc < 0) {
        ERROR("recvfrom failed.\n");
        RESP(2, translate_error(errno));
        RESP(1, 0);
        free(buffer);
        return 0;
    }

    if (mem_Write(buffer, CMD(8), rc) != 0) {
        ERROR("mem_Write failed.\n");
        RESP(2, translate_error(EFAULT));
        RESP(1, 0);
        free(buffer);
        return 0;
    }

    memcpy(&addrbuf[2], saddr->sa_data, addrlen-2);
    addrbuf[0] = addrlen;
    addrbuf[1] = saddr->sa_family;

    if (mem_Write(addrbuf, EXTENDED_CMD(1), CMD(4) < addrlen ? CMD(4) : addrlen) != 0) {
      RESP(2, translate_error(EFAULT));
      RESP(1, 0);
      free(buffer);
      return 0;
    }

    RESP(2, 0);
    RESP(1, rc);
    free(buffer);
    return 0;
}

SERVICE_CMD(0x00080102) //recvfrom
{
    DEBUG("recvfrom %08X %08X %08X %08X %08X %08X\n", CMD(1), CMD(2), CMD(3), CMD(4), EXTENDED_CMD(1), EXTENDED_CMD(3));

    uint8_t                 addrbuf[0x1C];
    struct sockaddr_storage addr;
    struct sockaddr         *saddr = (struct sockaddr*)&addr;
    socklen_t               addrlen = CMD(4);

    handleinfo* h = handle_Get(CMD(1) + HANDLES_BASE); //bit(31) must not be set
    if (h == NULL) {
        DEBUG("failed to get Handle\n");
        RESP(2, translate_error(EBADF));
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

    if (addrlen == 0x8) {
        if (mem_Read(addrbuf, EXTENDED_CMD(3), addrlen) != 0) {
            RESP(2, translate_error(EFAULT));
            RESP(1, 0);
            free(buffer);
            return 0;
        }
        saddr->sa_family = addrbuf[1];
        memcpy(saddr->sa_data, &addrbuf[2], 0x6);
    }
    else if (addrlen == 0x1C) {
        if (mem_Read(addrbuf, EXTENDED_CMD(3), addrlen) != 0) {
            RESP(2, translate_error(EFAULT));
            RESP(1, 0);
            free(buffer);
            return 0;
        }
        /* this size of 0xE seems bogus */
        saddr->sa_family = addrbuf[1];
        memcpy(saddr->sa_data, &addrbuf[2], 0xE);
        addrlen = 0x10;
    }
    else {
        DEBUG("unknown len\n");
        RESP(2, translate_error(errno));
        RESP(1, 0);
        free(buffer);
        return 0;
    }

    // TODO detect that we are doing recv()
    SOCKET s = (long)h->misc_ptr[0];
    ssize_t rc = recvfrom(s, buffer, CMD(2), CMD(3), saddr, &addrlen);
    if (rc < 0) {
        ERROR("recvfrom failed.\n");
        RESP(2, translate_error(errno));
        RESP(1, 0);
        free(buffer);
        return 0;
    }

    if (mem_Write(buffer, CMD(1), rc) != 0) {
        ERROR("mem_Write failed.\n");
        RESP(2, translate_error(EFAULT));
        RESP(1, 0);
        free(buffer);
        return 0;
    }

    memcpy(&addrbuf[2], saddr->sa_data, addrlen-2);
    addrbuf[0] = addrlen;
    addrbuf[1] = saddr->sa_family;

    if (mem_Write(addrbuf, EXTENDED_CMD(3), CMD(4) < addrlen ? CMD(4) : addrlen) != 0) {
      RESP(2, translate_error(EFAULT));
      RESP(1, 0);
      free(buffer);
      return 0;
    }

    RESP(2, 0);
    RESP(1, rc);
    free(buffer);
    return 0;

}

SERVICE_END()

typedef struct error_map_t {
    int from;
    int to;
} error_map_t;

static error_map_t error_map[] = {
    { 0,                0, },
    { E2BIG,            1, },
    { EACCES,           2, },
    { EADDRINUSE,       3, },
    { EADDRNOTAVAIL,    4, },
    { EAFNOSUPPORT,     5, },
    { EAGAIN,           6, },
    { EALREADY,         7, },
    { EBADF,            8, },
    { EBADMSG,          9, },
    { EBUSY,           10, },
    { ECANCELED,       11, },
    { ECHILD,          12, },
    { ECONNABORTED,    13, },
    { ECONNREFUSED,    14, },
    { ECONNRESET,      15, },
    { EDEADLK,         16, },
    { EDESTADDRREQ,    17, },
    { EDOM,            18, },
    { EDQUOT,          19, },
    { EEXIST,          20, },
    { EFAULT,          21, },
    { EFBIG,           22, },
    { EHOSTUNREACH,    23, },
    { EIDRM,           24, },
    { EILSEQ,          25, },
    { EINPROGRESS,     26, },
    { EINTR,           27, },
    { EINVAL,          28, },
    { EIO,             29, },
    { EISCONN,         30, },
    { EISDIR,          31, },
    { ELOOP,           32, },
    { EMFILE,          33, },
    { EMLINK,          34, },
    { EMSGSIZE,        35, },
    { EMULTIHOP,       36, },
    { ENAMETOOLONG,    37, },
    { ENETDOWN,        38, },
    { ENETRESET,       39, },
    { ENETUNREACH,     40, },
    { ENFILE,          41, },
    { ENOBUFS,         42, },
    { ENODATA,         43, },
    { ENODEV,          44, },
    { ENOENT,          45, },
    { ENOEXEC,         46, },
    { ENOLCK,          47, },
    { ENOLINK,         48, },
    { ENOMEM,          49, },
    { ENOMSG,          50, },
    { ENOPROTOOPT,     51, },
    { ENOSPC,          52, },
    { ENOSR,           53, },
    { ENOSTR,          54, },
    { ENOSYS,          55, },
    { ENOTCONN,        56, },
    { ENOTDIR,         57, },
    { ENOTEMPTY,       58, },
    { ENOTSOCK,        59, },
    { ENOTSUP,         60, },
    { ENOTTY,          61, },
    { ENXIO,           62, },
    { EOPNOTSUPP,      63, },
    { EOVERFLOW,       64, },
    { EPERM,           65, },
    { EPIPE,           66, },
    { EPROTO,          67, },
    { EPROTONOSUPPORT, 68, },
    { EPROTOTYPE,      69, },
    { ERANGE,          70, },
    { EROFS,           71, },
    { ESPIPE,          72, },
    { ESRCH,           73, },
    { ESTALE,          74, },
    { ETIME,           75, },
    { ETIMEDOUT,       76, },
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

__attribute__((constructor))
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
