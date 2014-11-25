#include <stdlib.h>

#include "util.h"
#include "arm11.h"
#include "mem.h"
#include "fs.h"

#include "threads.h"
#include "polarssl/aes.h"
#include "loader.h"

//from ctrtool

u32 getle32(const u8* p)
{
    return (p[0] << 0) | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);
}

u32 getle16(const u8* p)
{
    return (p[0] << 0) | (p[1] << 8);
}
void ctr_set_counter(ctr_aes_context* ctx,
    u8 ctr[16])
{
    memcpy(ctx->ctr, ctr, 16);
}

void ctr_add_counter(ctr_aes_context* ctx,
    u32 carry)
{
    u32 counter[4];
    u32 sum;
    int i;

    for (i = 0; i<4; i++)
        counter[i] = (ctx->ctr[i * 4 + 0] << 24) | (ctx->ctr[i * 4 + 1] << 16) | (ctx->ctr[i * 4 + 2] << 8) | (ctx->ctr[i * 4 + 3] << 0);

    for (i = 3; i >= 0; i--)
    {
        sum = counter[i] + carry;

        if (sum < counter[i])
            carry = 1;
        else
            carry = 0;

        counter[i] = sum;
    }

    for (i = 0; i<4; i++)
    {
        ctx->ctr[i * 4 + 0] = counter[i] >> 24;
        ctx->ctr[i * 4 + 1] = counter[i] >> 16;
        ctx->ctr[i * 4 + 2] = counter[i] >> 8;
        ctx->ctr[i * 4 + 3] = counter[i] >> 0;
    }
}

void ctr_init_counter(ctr_aes_context* ctx,
    u8 key[16],
    u8 ctr[16])
{
    aes_setkey_enc(&ctx->aes, key, 128);
    ctr_set_counter(ctx, ctr);
}

void ctr_crypt_counter_block(ctr_aes_context* ctx,
    u8 input[16],
    u8 output[16])
{
    int i;
    u8 stream[16];


    aes_crypt_ecb(&ctx->aes, AES_ENCRYPT, ctx->ctr, stream);


    if (input)
    {
        for (i = 0; i<16; i++)
        {
            output[i] = stream[i] ^ input[i];
        }
    }
    else
    {
        for (i = 0; i<16; i++)
            output[i] = stream[i];
    }

    ctr_add_counter(ctx, 1);
}

void ctr_crypt_counter(ctr_aes_context* ctx,
    u8* input,
    u8* output,
    u32 size)
{
    u8 stream[16];
    u32 i;

    while (size >= 16)
    {
        ctr_crypt_counter_block(ctx, input, output);

        if (input)
            input += 16;
        if (output)
            output += 16;

        size -= 16;
    }

    if (size)
    {
        memset(stream, 0, 16);
        ctr_crypt_counter_block(ctx, stream, stream);

        if (input)
        {
            for (i = 0; i<size; i++)
                output[i] = input[i] ^ stream[i];
        }
        else
        {
            memcpy(output, stream, size);
        }
    }
}




void ncch_get_counter(ctr_ncchheader* h, u8 counter[16], u8 type)
{
#ifdef NYUU_DEC
    Nyuu_getcounter(h, counter, type);
#else
    u32 version = getle16(h->version);
    u32 mediaunitsize = 0x200;
    u8* partitionid = h->partitionid;
    u32 i;
    u32 x = 0;

    memset(counter, 0, 16);

    if (version == 2 || version == 0)
    {
        for (i = 0; i<8; i++)
            counter[i] = partitionid[7 - i];
        counter[8] = type;
    }
    else if (version == 1)
    {
        if (type == NCCHTYPE_EXHEADER)
            x = 0x200;
        else if (type == NCCHTYPE_EXEFS)
            x = getle32(h->exefsoffset) * mediaunitsize;
        else if (type == NCCHTYPE_ROMFS)
            x = getle32(h->romfsoffset) * mediaunitsize;

        for (i = 0; i<8; i++)
            counter[i] = partitionid[i];
        for (i = 0; i<4; i++)
            counter[12 + i] = x >> ((3 - i) * 8);
    }
#endif
}

int ncch_extract_prepare(ctr_aes_context* ctx, ctr_ncchheader* h, u32 type, u8* key)
{
    u32 offset = 0;
    u32 size = 0;
    u8 counter[16];

    ncch_get_counter(h, counter, type);
    ctr_init_counter(ctx, key, counter);

    return 1;
}

int programid_is_system(u8 programid[8])
{
    u32 hiprogramid = getle32(programid + 4);

    if (((hiprogramid >> 14) == 0x10) && (hiprogramid & 0x10))
        return 1;
    else
        return 0;
}
