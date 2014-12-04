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

void ctr_set_counter(ctr_aes_context* ctx,
    u8 ctr[16]);

void ctr_add_counter(ctr_aes_context* ctx,
    u32 carry);

void ctr_init_counter(ctr_aes_context* ctx,
    u8 key[16],
    u8 ctr[16]);

void ctr_crypt_counter_block(ctr_aes_context* ctx,
    u8 input[16],
    u8 output[16]);

void ctr_crypt_counter(ctr_aes_context* ctx,
    u8* input,
    u8* output,
    u32 size);
void ncch_get_counter(ctr_ncchheader* h, u8 counter[16], u8 type);

int ncch_extract_prepare(ctr_aes_context* ctx, ctr_ncchheader* h, u32 type, u8* key);

int programid_is_system(u8 programid[8]);