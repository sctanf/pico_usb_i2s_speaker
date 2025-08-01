#ifndef PICO_AMP_EQ_H
#define PICO_AMP_EQ_H

#include <stdio.h>

#include "pico/stdlib.h"

#include "dsp.h"
#include "i2s.h"

// dsp audio buffers
dspfx buf0[192];
dspfx buf1[192];
dspfx buf2[192];
dspfx out_buf[192];

// equalizer filters
biquad(eq_bq_0)
biquad(eq_bq_00) // TODO:
biquad(eq_bq_1)
biquad(eq_bq_2)
biquad(eq_bq_3)
biquad(eq_bq_4)
biquad(eq_bq_5)
biquad(eq_bq_6)
biquad(eq_bq_7)
biquad(eq_bq_8)
biquad(eq_bq_9)
biquad(eq_bq_10)
biquad(eq_bq_11)
biquad(eq_bq_12)
biquad(eq_bq_13)
biquad(eq_bq_14)
biquad(eq_bq_15)
biquad(eq_bq_16)
biquad(eq_bq_17)
biquad(eq_bq_18)

//#define PASSTHRU_ENABLE

#define EQ_ENABLE
#define BASS_ENABLE

#define USE_EQ 0

#include "eq_configs.h"
#include "eq_default_config.h"

#ifdef PASSTHRU_ENABLE
#ifdef EQ_ENABLE
#undef EQ_ENABLE
#endif
#ifdef BASS_ENABLE
#undef BASS_ENABLE
#endif
#endif

int32_t actual_vol = 0;
#define VOL_STEP 300000

dspfx limit[192]; // sample store
int limit_index = 0;
int32_t limit_vol = 0;
//#define LIMIT_MUL ((dspfx)(0.04*(double)(1<<30))) // 0.04x > -28dB limit relative to input (-28+18 > Stops limiting at -10dB)
//#define BASS_MUL floatfx(1./8.) // Bass peak filter is 18dB > 8x
int64_t targ = 0;
#define BASS_STEP 10000
#define BASS_STEP_DOWN 600000
#define BASS_STEP_THRESHOLD ((dspfx)(0.05*(double)(1<<30))) // 1% (i dunno)
#define BASS_STEP_DELAY_US 500*1000

uint cur_alt = 1;

uint64_t bass_step_time = 0;

/**
 * @brief Stores uint8_t data sent from USB in the i2s buffer.
 *
 * @param in Data to store
 * @param sample Number of bytes to store
 * @param resolution Sample bit depth (16, 24, 32)
 * @return true Success
 * @return false Failure (buffer full)
 */
static void __not_in_flash_func(eq_process)(uint8_t* buffer, int sample, uint8_t resolution) {
    uint64_t now_time = time_us_64();

    int16_t count;
//    int32_t vol_mul = audio_state.mute ? 0 : audio_state.vol_mul;
    int32_t vol_mul = 0x40000000; // TODO: max volume

#ifdef PASSTHRU_ENABLE
#define HEADROOM 0
#else
#define HEADROOM 1
#endif
    switch (resolution)
    {
    case 32: // 32bit
        count = sample / 8;
        {
            int32_t *in = (int32_t *) buffer;
            for (int i = 0; i < count * 2; i++)
                buf0[i] = in[i] >> HEADROOM; // divide by 2, allow some headroom in case the filters need it
        }
        break;
    case 24: // 24bit
        count = sample / 6;
        {
            uint8_t *in = (uint8_t *) buffer;
            for (int i = 0; i < count * 2; i += 2) {
                int j = i * 3;
                buf0[i] = (in[j] << 8 | in[j+1] << 16 | (int8_t)in[j+2] << 24) >> HEADROOM; // divide by 2, allow some headroom in case the filters need it
                buf0[i+1] = (in[j+3] << 8 | in[j+4] << 16 | (int8_t)in[j+5] << 24) >> HEADROOM; // divide by 2, allow some headroom in case the filters need it
            }
        }
        break;
    case 16: // 16bit
        count = sample / 4;
        {
            int16_t *in = (int16_t *) buffer;
            for (int i = 0; i < count * 2; i++)
                buf0[i] = (in[i] << 16) >> HEADROOM; // divide by 2, allow some headroom in case the filters need it
        }
        break;
    default:
        count = 0;
        break;
    }

    // main filters
#ifdef EQ_ENABLE
    process_biquad(&eq_bq_1, biquadconstsfx(EQ_I_0), count, buf0, buf1);
    process_biquad(&eq_bq_2, biquadconstsfx(EQ_I_1), count, buf1, buf0);
#ifdef EQ_I_2
    process_biquad(&eq_bq_3, biquadconstsfx(EQ_I_2), count, buf0, buf1);
    process_biquad(&eq_bq_4, biquadconstsfx(EQ_I_3), count, buf1, buf0);
#endif
#ifdef EQ_I_4
    process_biquad(&eq_bq_5, biquadconstsfx(EQ_I_4), count, buf0, buf1);
#endif
#ifdef EQ_I_5
    process_biquad(&eq_bq_6, biquadconstsfx(EQ_I_5), count, buf1, buf0);
#endif
#ifdef EQ_I_6
    process_biquad(&eq_bq_7, biquadconstsfx(EQ_I_6), count, buf0, buf1);
    process_biquad(&eq_bq_8, biquadconstsfx(EQ_I_7), count, buf1, buf0);
#endif
#ifdef EQ_I_8
    process_biquad(&eq_bq_9, biquadconstsfx(EQ_I_8), count, buf0, buf1);
    process_biquad(&eq_bq_10, biquadconstsfx(EQ_I_9), count, buf1, buf0);
#endif
#ifdef EQ_I_10
    process_biquad(&eq_bq_11, biquadconstsfx(EQ_I_10), count, buf0, buf1);
    process_biquad(&eq_bq_12, biquadconstsfx(EQ_I_11), count, buf1, buf0);
#endif
#ifdef EQ_I_12
    process_biquad(&eq_bq_13, biquadconstsfx(EQ_I_12), count, buf0, buf1);
    process_biquad(&eq_bq_14, biquadconstsfx(EQ_I_13), count, buf1, buf0);
#endif
#ifdef EQ_I_14
    process_biquad(&eq_bq_15, biquadconstsfx(EQ_I_14), count, buf0, buf1);
    process_biquad(&eq_bq_16, biquadconstsfx(EQ_I_15), count, buf1, buf0);
#endif
#ifdef EQ_I_16
    process_biquad(&eq_bq_17, biquadconstsfx(EQ_I_16), count, buf0, buf1);
    process_biquad(&eq_bq_18, biquadconstsfx(EQ_I_17), count, buf1, buf0);
#endif
#endif

    // volume
    dspfx volume_mul[96] = {0};
    for (int i = 0; i < count; i++) {
        if (actual_vol - VOL_STEP > vol_mul)
            actual_vol -= VOL_STEP;
        else if (actual_vol < vol_mul - VOL_STEP)
            actual_vol += VOL_STEP;
        else
            actual_vol = vol_mul;
        volume_mul[i] = actual_vol;
        buf0[i*2] = mulfx2(LAST_EQ_BUF[i*2], actual_vol);
        buf0[i*2+1] = mulfx2(LAST_EQ_BUF[i*2+1], actual_vol);
    }

    // amp
#ifdef POWER_LIMIT
    for (int i = 0; i < count * 2; i++)
        buf0[i] = buf0[i] >> POWER_LIMIT;
#endif

    // bass filters
#ifdef BASS_ENABLE
    for (int i = 0; i < count * 2; i++)
        buf0[i] = buf0[i] >> 3; // divide by 8, headroom for bass eq

    process_biquad(&eq_bq_0, biquadconstsfx(EQ_BASS), count, buf0, buf1);
#ifdef EQ_BASS_2
    process_biquad(&eq_bq_00, biquadconstsfx(EQ_BASS_2), count, buf1, buf2);
    #define BASS_BUF buf2
#else
    #define BASS_BUF buf1
#endif

    limit[limit_index] = fxabs(BASS_BUF[0]);
    if (limit[limit_index]>0) 
        limit_index++;
    limit_index %= 192;
    limit[limit_index] = fxabs(BASS_BUF[1]);
    if (limit[limit_index]>0)
        limit_index++;
    limit_index %= 192;

    dspfx max = 0;
    for (int i = 0; i < 192; i++)
        if (limit[i] > max) max = limit[i];

    if (max < LIMIT_MUL)
        max = LIMIT_MUL;
    int64_t actualtarg = (int64_t)(LIMIT_MUL - mulfx(max, BASS_MUL)) * (int64_t)(1<<30) / (int64_t)(max - mulfx(max, BASS_MUL)); // target mix to limit bass
    if (actualtarg < 0) // if the mix goes negative, ignore it
        actualtarg = 0;

    for (int i = 0; i < count * 2; i += 2) {
        if (targ > actualtarg - BASS_STEP_THRESHOLD)
            bass_step_time = now_time + BASS_STEP_DELAY_US;
        if (targ > actualtarg || now_time > bass_step_time) {
            if (targ - BASS_STEP_DOWN > actualtarg)
                targ -= BASS_STEP_DOWN;
            else if (targ < actualtarg - BASS_STEP)
                targ += BASS_STEP;
            else
                targ = actualtarg;
        }
        buf0[i] = (mulfx2(buf0[i], (1<<30) - targ) + mulfx2(BASS_BUF[i], targ)) << 3;
        buf0[i+1] = (mulfx2(buf0[i+1], (1<<30) - targ) + mulfx2(BASS_BUF[i+1], targ)) << 3;
    }
#endif

    // limiter
#ifndef PASSTHRU_ENABLE
#ifdef POWER_LIMIT
// it seems like the bass filters are putting the signal level above slightly, compensate by raising limiter
#define LIMIT_MAX ((LIMIT_MUL << 3) + ((dspfx)(0.05*(double)(1<<30))))
#else
#define LIMIT_MAX ((1 << 30) - 1)
#endif
    dspfx limit_max = ((1 << 30) - 1);
    dspfx limit_min;
    for (int i = 0; i < count; i++) {
        if (LIMIT_MAX > 0) {
            limit_max = volume_mul[i] > LIMIT_MAX ? volume_mul[i] : LIMIT_MAX;
        }
        limit_min = -LIMIT_MAX - 1;
        if (buf0[i*2] > limit_max)
            buf0[i*2] = limit_max;
        if (buf0[i*2+1] < limit_min)
            buf0[i*2+1] = limit_min;
    }
#endif

    for (int i = 0; i < count * 2; i++)
        out_buf[i] = buf0[i] << HEADROOM;

    i2s_enqueue((uint8_t *)out_buf, count * 8, 32);
}

#endif