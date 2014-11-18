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

#include "util.h"
#include "handles.h"
#include "mem.h"
#include "arm11.h"
#include "service_macros.h"

#ifdef _WIN32
#include <winsock.h>

WSADATA wsaData;
#endif
u32 soc_shared_size = 0;
u32 soc_shared_mem_handle = 0;

SERVICE_START(soc_u);
#ifdef _WIN32
SERVICE_CMD(0x00010044)   //InitializeSockets
{
    DEBUG("InitializeSockets %08x %08x\n", CMD(1), CMD(5));
    WSAStartup(MAKEWORD(1, 1), &wsaData);
    soc_shared_size = CMD(1);
    soc_shared_mem_handle = CMD(5);
    RESP(1, 0); // Result
    return 0;
}
SERVICE_CMD(0x00160000)   //Gethostid
{
    char ac[80];
    gethostname(ac, sizeof(ac));
    struct hostent *phe = gethostbyname(ac);
    struct in_addr addr;
    memcpy(&addr, phe->h_addr_list[0], sizeof(struct in_addr));

    DEBUG("Gethostid %s name: %s\n",inet_ntoa(addr) ,ac );
    RESP(2, addr.S_un.S_addr); // Result
    RESP(1, 0); // Result
    return 0;
}
SERVICE_CMD(0x000200c2) //socket
{
    DEBUG("socket %d %d %d\n", CMD(1), CMD(2), CMD(3));
    u32 handleorigin = arm11_R(0);
    u32 type = arm11_R(1);
    u32 handle = handle_New(HANDLE_TYPE_SOCKET, 0);

    handleinfo* h = handle_Get(handle);
    if (h == NULL) {
        DEBUG("failed to get newly created Handle\n");
        PAUSE();
        return -1;
    }
    SOCKET s = socket(CMD(1), CMD(2), CMD(3));
    if (s == NULL) {
        DEBUG("failed to get newly created SOCKET\n");
        PAUSE();
        return -1;
    }
    h->misc_ptr[0] = malloc(sizeof(SOCKET));
    memcpy(h->misc_ptr[0], &s, sizeof(SOCKET));

    RESP(2, handle - HANDLES_BASE); //bit(31) must not be set
    RESP(1, 0); // Result
    return 0;
}
SERVICE_CMD(0x00050084) //bind
{
    struct sockaddr saServer;

    u8* b = malloc(CMD(2));
    if (b == NULL) {
        ERROR("Not enough mem.\n");
        return -1;
    }

    if (mem_Read(b, CMD(6), CMD(2)) != 0) {
        ERROR("mem_Read failed.\n");
        free(b);
        return -1;
    }
    DEBUG("bind %08X %08X %08X\n", CMD(1), CMD(2), CMD(6));
    handleinfo* h = handle_Get(CMD(1) + HANDLES_BASE); //bit(31) must not be set
    if (h == NULL) {
        DEBUG("failed to get Handle\n");
        PAUSE();
        return -1;
    }
    struct sockaddr serv_addr;
    memset((char *)&serv_addr, 0, sizeof(serv_addr));
    if (CMD(2) == 0x8)
    {
        memcpy(serv_addr.sa_data, &b[2], 0x6);
    }
    else if (CMD(2) == 0x1c)
    {
        memcpy(serv_addr.sa_data, &b[2], 14);
    }
    else
    {
        DEBUG("unknown len\n");
    }
    /*serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(5001);*/
    serv_addr.sa_family = b[1];

    RESP(2, (u32)bind(*(SOCKET*)(h->misc_ptr[0]), &serv_addr, sizeof(serv_addr)));
    RESP(1, 0); // Result
    free(b);
    return 0;
}
SERVICE_CMD(0x00030082) //listen
{

    DEBUG("listen %08X %08X\n", CMD(1), CMD(2));
    handleinfo* h = handle_Get(CMD(1) + HANDLES_BASE); //bit(31) must not be set
    if (h == NULL) {
        DEBUG("failed to get Handle\n");
        PAUSE();
        return -1;
    }

    RESP(2, (u32)listen(*(SOCKET*)(h->misc_ptr[0]), CMD(2)));
    RESP(1, 0); // Result
    return 0;
}
SERVICE_CMD(0x000B0042) //close
{

    DEBUG("close %08X --todo--\n", CMD(1));
    handleinfo* h = handle_Get(CMD(1) + HANDLES_BASE); //bit(31) must not be set
    if (h == NULL) {
        DEBUG("failed to get Handle\n");
        PAUSE();
        return -1;
    }

    RESP(2, (u32)closesocket(*(SOCKET*)(h->misc_ptr[0])));
    RESP(1, 0); // Result
    return 0;
}
SERVICE_CMD(0x00040082) //accept
{
    struct sockaddr_in cli_addr;
    DEBUG("accept %08X %08X %08X\n", CMD(1), CMD(2), EXTENDED_CMD(1));
    handleinfo* h = handle_Get(CMD(1) + HANDLES_BASE); //bit(31) must not be set
    if (h == NULL) {
        DEBUG("failed to get Handle\n");
        PAUSE();
        return -1;
    }
    int clilen = sizeof(cli_addr);



    u32 handle = handle_New(HANDLE_TYPE_SOCKET, 0);
    handleinfo* ha = handle_Get(handle);
    if (ha == NULL) {
        DEBUG("failed to get newly created Handle\n");
        PAUSE();
        return -1;
    }
    SOCKET s = accept(*(SOCKET*)(h->misc_ptr[0]), (struct sockaddr *)&cli_addr, &clilen);
    if (s == NULL) {
        DEBUG("failed to get newly created SOCKET\n");
        PAUSE();
        return -1;
    }
    ha->misc_ptr[0] = malloc(sizeof(SOCKET));
    memcpy(ha->misc_ptr[0], &s, sizeof(SOCKET));

    mem_Write(&cli_addr, EXTENDED_CMD(1), CMD(2) > sizeof(cli_addr) ? sizeof(cli_addr) : CMD(2));

    RESP(2, handle - HANDLES_BASE); //bit(31) must not be set
    RESP(1, 0); // Result
    return 0;
}
SERVICE_CMD(0x000a0106) //sendto
{
    struct sockaddr saServer;

    u8* a = malloc(CMD(4));
    if (a == NULL) {
        ERROR("Not enough mem.\n");
        return -1;
    }

    if (mem_Read(a, CMD(10), CMD(4)) != 0) {
        ERROR("mem_Read failed.\n");
        free(a);
        return -1;
    }

    u8* b = malloc(CMD(2));
    if (b == NULL) {
        ERROR("Not enough mem.\n");
        return -1;
    }

    if (mem_Read(b, CMD(8), CMD(2)) != 0) {
        ERROR("mem_Read failed.\n");
        free(b);
        return -1;
    }
    DEBUG("sendto %08X %08X %08X %08X %08X %08X\n", CMD(1), CMD(2), CMD(3), CMD(4), CMD(8), CMD(10));
    handleinfo* h = handle_Get(CMD(1) + HANDLES_BASE); //bit(31) must not be set
    if (h == NULL) {
        DEBUG("failed to get Handle\n");
        PAUSE();
        return -1;
    }
    struct sockaddr serv_addr;
    memset((char *)&serv_addr, 0, sizeof(serv_addr));
    if (CMD(4) == 0x8)
    {
        serv_addr.sa_family = a[1];
        memcpy(serv_addr.sa_data, &a[2], 0x6);
    }
    else if (CMD(4) == 0x1c)
    {
        serv_addr.sa_family = a[1];
        memcpy(serv_addr.sa_data, &a[2], 14);
    }
    else
    {
        DEBUG("unknown len\n");
    }

    RESP(2, (u32)sendto(*(SOCKET*)(h->misc_ptr[0]), b, CMD(2), CMD(3), &serv_addr, sizeof(serv_addr)));
    RESP(1, 0); // Result
    free(b);
    return 0;
}

SERVICE_CMD(0x00080102) //recvfrom
{
    struct sockaddr saServer;

    u8* a = malloc(CMD(4));
    if (a == NULL) {
        ERROR("Not enough mem.\n");
        return -1;
    }

    if (mem_Read(a, EXTENDED_CMD(3), CMD(4)) != 0) {
        ERROR("mem_Read failed.\n");
        free(a);
        return -1;
    }

    u8* b = malloc(CMD(2));
    if (b == NULL) {
        ERROR("Not enough mem.\n");
        return -1;
    }
    DEBUG("recvfrom %08X %08X %08X %08X %08X %08X\n", CMD(1), CMD(2), CMD(3), CMD(4), EXTENDED_CMD(1), EXTENDED_CMD(3));
    handleinfo* h = handle_Get(CMD(1) + HANDLES_BASE); //bit(31) must not be set
    if (h == NULL) {
        DEBUG("failed to get Handle\n");
        PAUSE();
        return -1;
    }
    struct sockaddr serv_addr;
    memset((char *)&serv_addr, 0, sizeof(serv_addr));
    if (CMD(4) == 0x8)
    {
        serv_addr.sa_family = a[1];
        memcpy(serv_addr.sa_data, &a[2], 0x6);
    }
    else if (CMD(4) == 0x1c)
    {
        serv_addr.sa_family = a[1];
        memcpy(serv_addr.sa_data, &a[2], 14);
    }
    else
    {
        DEBUG("unknown len\n");
    }

    RESP(3, (u32)recvfrom(*(SOCKET*)(h->misc_ptr[0]), b, CMD(2), CMD(3), &serv_addr, sizeof(serv_addr)));
    
    if (mem_Write(b, EXTENDED_CMD(1), CMD(2)) != 0) { //cmd 2 is not yet overwritten
        ERROR("mem_Write failed.\n");
        free(b);
        return -1;
    }

    RESP(2, 0); // POSIX return value
    RESP(1, 0); // Result
    free(b);
    return 0;
}




#endif
SERVICE_END();