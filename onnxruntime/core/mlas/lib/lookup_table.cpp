#include "mlasi.h"

#if defined(MLAS_NEON64_INTRINSICS)

static
MLAS_FORCEINLINE
uint8x16x4_t
QLinearLoadTableU8x64(const uint8_t* table)
{
    uint8x16x4_t lut;
    lut.val[0] = vld1q_u8(table);
    lut.val[1] = vld1q_u8(table + 16);
    lut.val[2] = vld1q_u8(table + 32);
    lut.val[3] = vld1q_u8(table + 48);
    return lut;
}

void
MLASCALL
MlasQLinearLookup(
    const uint8_t* x,
    const uint8_t* table,
    uint8_t* y,
    size_t n
    )
{
    const uint8x16x4_t lut0 = QLinearLoadTableU8x64(table);
    const uint8x16x4_t lut1 = QLinearLoadTableU8x64(table + 64);
    const uint8x16x4_t lut2 = QLinearLoadTableU8x64(table + 128);
    const uint8x16x4_t lut3 = QLinearLoadTableU8x64(table + 192);

    const uint8x16_t v64 = vdupq_n_u8(64);
    const uint8x16_t v128 = vdupq_n_u8(128);

    for (; n >= 16; n -=16) {
        uint8x16_t vindex0 = vld1q_u8(x);
        x += 16;

        uint8x16_t vresult = vqtbl4q_u8(lut0, vindex0);
        uint8x16_t vindex2 = vsubq_u8(vindex0, v128);
        vresult = vqtbx4q_u8(vresult, lut2, vindex2);
        vresult = vqtbx4q_u8(vresult, lut1, vsubq_u8(vindex0, v64));
        vresult = vqtbx4q_u8(vresult, lut3, vsubq_u8(vindex2, v64));
        vst1q_u8_ex(y, vresult, 8);
        y += 16;
    }

    if (n > 0) {
        uint8_t tail[16];
        memcpy(tail, x, n);

        uint8x16_t vindex0 = vld1q_u8(tail);
        uint8x16_t vresult = vqtbl4q_u8(lut0, vindex0);
        uint8x16_t vindex2 = vsubq_u8(vindex0, v128);
        vresult = vqtbx4q_u8(vresult, lut2, vindex2);
        vresult = vqtbx4q_u8(vresult, lut1, vsubq_u8(vindex0, v64));
        vresult = vqtbx4q_u8(vresult, lut3, vsubq_u8(vindex2, v64));

        uint8x8_t u8x8 = vget_low_u8(vresult);
        if (n & 8) {
            vst1_u8_ex(y, u8x8, 8);
            y += 8;
            u8x8 = vget_high_u8(vresult);
        }
        if (n & 4) {
            vst1_lane_u32_ex((uint32_t*)y, vreinterpret_u32_u8(u8x8), 0, 8);
            y += 4;
            u8x8 = vext_u8(u8x8, u8x8, 4);
        }
        if (n & 2) {
            vst1_lane_u16_ex((uint16_t*)y, vreinterpret_u16_u8(u8x8), 0, 8);
            y += 2;
            u8x8 = vext_u8(u8x8, u8x8, 2);
        }
        if (n & 1) {
            vst1_lane_u8(y, u8x8, 0);
        }
    }
}

void
MLASCALL
MlasQLinearLookup(
    size_t M,
    size_t N,
    const uint8_t* X,
    size_t ldx,
    const uint8_t* table,
    uint8_t* Y,
    size_t ldy
    )
{
    const uint8x16x4_t lut0 = QLinearLoadTableU8x64(table);
    const uint8x16x4_t lut1 = QLinearLoadTableU8x64(table + 64);
    const uint8x16x4_t lut2 = QLinearLoadTableU8x64(table + 128);
    const uint8x16x4_t lut3 = QLinearLoadTableU8x64(table + 192);

    for (; M > 0; M--) {
        const uint8_t* x = X;
        uint8_t* y = Y;
        size_t n = N;
        for (; n >= 16; n -=16) {
            uint8x16_t vindex0 = vld1q_u8(x);
            uint8x16_t vresult = vqtbl4q_u8(lut0, vindex0);
            x += 16;
            vresult = vqtbx4q_u8(vresult, lut1, vsubq_u8(vindex0, vdupq_n_u8(64)));
            vresult = vqtbx4q_u8(vresult, lut2, vsubq_u8(vindex0, vdupq_n_u8(128)));
            vresult = vqtbx4q_u8(vresult, lut3, vsubq_u8(vindex0, vdupq_n_u8(192)));

            // uint8x16_t vindex2 = vsubq_u8(vindex0, v128);
            // uint8x16_t vec0 = vqtbl4q_u8(lut0, vindex0);
            // uint8x16_t vec2 = vqtbl4q_u8(lut2, vindex2);
            // uint8x16_t vec1 = vqtbl4q_u8(lut1, vsubq_u8(vindex0, v64));
            // uint8x16_t vec3 = vqtbl4q_u8(lut3, vsubq_u8(vindex2, v64));
            // vec0 = vorrq_u8(vec0, vec2);
            // vec1 = vorrq_u8(vec1, vec3);
            // vec0 = vorrq_u8(vec0, vec1);
            vst1q_u8_ex(y, vresult, 8);
            y += 16;
        }

        if (n > 0) {
            uint8_t tail[16];
            memcpy(tail, x, n);

            uint8x16_t vindex0 = vld1q_u8(tail);
            uint8x16_t vresult = vqtbl4q_u8(lut0, vindex0);
            vresult = vqtbx4q_u8(vresult, lut1, vsubq_u8(vindex0, vdupq_n_u8(64)));
            vresult = vqtbx4q_u8(vresult, lut2, vsubq_u8(vindex0, vdupq_n_u8(128)));
            vresult = vqtbx4q_u8(vresult, lut3, vsubq_u8(vindex0, vdupq_n_u8(192)));

            uint8x8_t u8x8 = vget_low_u8(vresult);
            if (n & 8) {
                vst1_u8_ex(y, u8x8, 8);
                y += 8;
                u8x8 = vget_high_u8(vresult);
            }
            if (n & 4) {
                vst1_lane_u32_ex((uint32_t*)y, vreinterpret_u32_u8(u8x8), 0, 8);
                y += 4;
                u8x8 = vext_u8(u8x8, u8x8, 4);
            }
            if (n & 2) {
                vst1_lane_u16_ex((uint16_t*)y, vreinterpret_u16_u8(u8x8), 0, 8);
                y += 2;
                u8x8 = vext_u8(u8x8, u8x8, 2);
            }
            if (n & 1) {
                vst1_lane_u8(y, u8x8, 0);
            }
        }

        X += ldx;
        Y += ldy;
    }
}

#else


void
MLASCALL
MlasQLinearLookup(
    const uint8_t* x,
    const uint8_t* table,
    uint8_t* y,
    size_t n
    )
{
  for (; n >= 4; n -= 4) {
    const size_t x_value0 = x[0];
    const size_t x_value1 = x[1];
    const size_t x_value2 = x[2];
    const size_t x_value3 = x[3];
    x += 4;
    const uint8_t table_value0 = table[x_value0];
    const uint8_t table_value1 = table[x_value1];
    const uint8_t table_value2 = table[x_value2];
    const uint8_t table_value3 = table[x_value3];

    y[0] = table_value0;
    y[1] = table_value1;
    y[2] = table_value2;
    y[3] = table_value3;
    y += 4;
  }
  for (; n != 0; --n) {
    const size_t x_value0 = *x++;
    const uint8_t table_value0 = table[x_value0];
    *y++ = table_value0;
  }
}

void
MLASCALL
MlasQLinearLookup(
    size_t m,
    size_t n,
    const uint8_t* x,
    size_t ldx,
    const uint8_t* table,
    uint8_t* y,
    size_t ldy
    )
{
    for (; m > 0; m--) {
        MlasQLinearLookup(x, table, y, n);
        x += ldx;
        y += ldy;
    }
}

#endif
