#include "util.h"
#include "color.h"

void color_decode(unsigned char* input, const TextureFormat format, Color *output)
{
    switch(format) {
    case RGBA8:
    {
        output->r = input[3];
        output->g = input[2];
        output->b = input[1];
        output->a = input[0];
        break;
    }

    case RGB8:
    {
        output->r = input[2];
        output->g = input[1];
        output->b = input[0];
        output->a = 255;
        break;
    }

    case RGBA5551:
    {
        u16 val = input[0] | input[1] << 8;
        output->r = ((val >> 11) & 0x1F) * 8;
        output->g = ((val >> 6) & 0x1F) * 8;
        output->b = ((val >> 1) & 0x1F) * 8;
        output->a = (val & 1 ) * 255;
        break;
    }

    case RGB565:
    {
        u16 val = input[0] | input[1] << 8;
        output->r = ((val >> 11) & 0x1F) * 8;
        output->g = ((val >> 5) & 0x3F) * 4;
        output->b = ((val >> 0) & 0x1F) * 8;
        output->a = 255;
        break;
    }

    case RGBA4:
    {
        u16 val = input[0] | input[1] << 8;
        output->r = ((val >> 12) & 0xF) * 16;
        output->g = ((val >> 8) & 0xF) * 16;
        output->b = ((val >> 4) & 0xF) * 16;
        output->a = (val & 0xF) * 16;
        break;
    }

    case IA8:
    {
        output->r = input[0];
        output->g = input[0];
        output->b = input[0];
        output->a = input[1];
        break;
    }

    case I8:
    {
        output->r = input[0];
        output->g = input[0];
        output->b = input[0];
        output->a = 255;
        break;
    }

    case A8:
    {
        output->r = 0;
        output->g = 0;
        output->b = 0;
        output->a = input[0];
    }

    case IA4:
    {
        output->r = ((input[0] >> 4) & 0xF) * 16;
        output->g = ((input[0] >> 4) & 0xF) * 16;
        output->b = ((input[0] >> 4) & 0xF) * 16;
        output->a = (input[0] & 0xF) * 16;
        break;
    }

    /*case I4:
      {
      const u8* source_ptr = source + coarse_x * block_height / 2 + coarse_y * stride + texel_index_within_tile / 2;

      // TODO: Order?
      u8 i = (coarse_x % 2) ? ((*source_ptr) & 0xF) : (((*source_ptr) & 0xF0) >> 4);
      i |= i << 4;

      ret.v[0] = i;
      ret.v[1] = i;
      ret.v[2] = i;
      ret.v[3] = 255;
      break;
      }

      case A4:
      {
      const u8* source_ptr = source + coarse_x * block_height / 2 + coarse_y * stride + texel_index_within_tile / 2;

      // TODO: Order?
      u8 a = (coarse_x % 2) ? ((*source_ptr) & 0xF) : (((*source_ptr) & 0xF0) >> 4);
      a |= a << 4;

      // TODO: Better control this...
      if(disable_alpha) {
      ret.v[0] = a;
      ret.v[1] = a;
      ret.v[2] = a;
      ret.v[3] = 255;
      }
      else {
      ret.v[0] = 0;
      ret.v[1] = 0;
      ret.v[2] = 0;
      ret.v[3] = a;
      }
      break;
      }*/

    default:
        DEBUG("Unknown texture format: 0x%08X\n", (u32)format);
        break;
    }
}

void color_encode(Color *input, const TextureFormat format, unsigned char* output)
{
    switch(format) {
    case RGBA8:
    {
        output[3] = input->r;
        output[2] = input->g;
        output[1] = input->b;
        output[0] = input->a;
        break;
    }

    case RGB8:
    {
        output[0] = input->b;
        output[1] = input->g;
        output[2] = input->r;
        break;
    }

    case RGBA5551:
    {
        u16 val = 0;

        val |= ((input->r / 8) & 0x1F) << 11;
        val |= ((input->g / 8) & 0x1F) << 6;
        val |= ((input->b / 8) & 0x1F) << 1;
        val |= (input->a != 0) ? 1 : 0;

        output[0] = val & 0xFF;
        output[1] = (val >> 8) & 0xFF;
        break;
    }

    case RGB565:
    {
        u16 val = 0;

        val |= ((input->r / 8) & 0x1F) << 11;
        val |= ((input->g / 4) & 0x3F) << 5;
        val |= ((input->b / 8) & 0x1F) << 0;

        output[0] = val & 0xFF;
        output[1] = (val >> 8) & 0xFF;
        break;
    }

    case RGBA4:
    {
        u16 val = 0;

        val |= ((input->r / 16) & 0xF) << 12;
        val |= ((input->g / 16) & 0xF) << 8;
        val |= ((input->b / 16) & 0xF) << 4;
        val |= ((input->a / 16) & 0xF) << 0;

        output[0] = val & 0xFF;
        output[1] = (val >> 8) & 0xFF;

        break;
    }

    case IA8:
    {
        output[0] = input->r;
        output[1] = input->a;
        break;
    }

    case I8:
    {
        output[0] = input->r;
        break;
    }

    case A8:
    {
        output[0] = input->a;
    }

    case IA4:
    {
        u8 val = 0;
        val |= ((input->r / 16) & 0xF) << 4;
        val |= ((input->a / 16) & 0xF) << 0;
        output[0] = val;
        break;
    }

    /*case I4:
      {
      const u8* source_ptr = source + coarse_x * block_height / 2 + coarse_y * stride + texel_index_within_tile / 2;

      // TODO: Order?
      u8 i = (coarse_x % 2) ? ((*source_ptr) & 0xF) : (((*source_ptr) & 0xF0) >> 4);
      i |= i << 4;

      ret.v[0] = i;
      ret.v[1] = i;
      ret.v[2] = i;
      ret.v[3] = 255;
      break;
      }

      case A4:
      {
      const u8* source_ptr = source + coarse_x * block_height / 2 + coarse_y * stride + texel_index_within_tile / 2;

      // TODO: Order?
      u8 a = (coarse_x % 2) ? ((*source_ptr) & 0xF) : (((*source_ptr) & 0xF0) >> 4);
      a |= a << 4;

      // TODO: Better control this...
      if(disable_alpha) {
      ret.v[0] = a;
      ret.v[1] = a;
      ret.v[2] = a;
      ret.v[3] = 255;
      }
      else {
      ret.v[0] = 0;
      ret.v[1] = 0;
      ret.v[2] = 0;
      ret.v[3] = a;
      }
      break;
      }*/

    default:
        DEBUG("Unknown texture format: 0x%08X\n", (u32)format);
        break;
    }
}
