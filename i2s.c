#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/dma.h"

#include "bsp/board_api.h"
#include "tusb.h"
#include "usb_descriptors.h"

#include "i2s.pio.h"
#include "i2s.h"

static int8_t i2s_buf_length;
static uint8_t enqueue_pos;
static uint8_t dequeue_pos;

static int32_t i2s_buf[BUF_DEPTH][CFG_TUD_AUDIO_FUNC_1_EP_OUT_SW_BUF_SZ / 3];
static uint32_t i2s_sample[BUF_DEPTH];

static int32_t mul_l;
static int32_t mul_r;
static const int32_t db_to_vol[101] = {
	0x20000000,     0x1c8520af,     0x196b230b,     0x16a77dea,     0x1430cd74,     0x11feb33c,     0x1009b9cf,     0xe4b3b63,      0xcbd4b3f,      0xb5aa19b,
    0xa1e89b1,      0x904d1bd,      0x809bcc3,      0x729f5d9,      0x66284d5,      0x5b0c438,      0x5125831,      0x4852697,      0x4074fcb,      0x3972853,
    0x3333333,      0x2da1cde,      0x28ab6b4,      0x243f2fd,      0x204e158,      0x1ccab86,      0x19a9294,      0x16dec56,      0x146211f,      0x122a9c2,
    0x1030dc4,      0xe6e1c6,       0xcdc613,       0xb76562,       0xa373ae,       0x91ad38,       0x81d59e,       0x73b70f,       0x672194,       0x5bea6e,
    0x51eb85,       0x4902e3,       0x411245,       0x39feb2,       0x33b022,       0x2e1127,       0x290ea8,       0x2497a2,       0x209ce9,       0x1d10f9,
    0x19e7c6,       0x171693,       0x1493ce,       0x1256f0,       0x10585e,       0xe9152,        0xcfbc3,        0xb924e,        0xa5028,        0x9310b,
    0x83126,        0x74d16,        0x681d3,        0x5ccab,        0x52b36,        0x49b50,        0x41b10,        0x3a8c3,        0x342e4,        0x2e818,
    0x2972d,        0x24f0e,        0x20ec7,        0x1d57e,        0x1a26f,        0x174ee,        0x14c60,        0x1283b,        0x10804,        0xeb4d,
    0xd1b7,         0xbae8,         0xa695,         0x9477,         0x8452,         0x75ee,         0x691b,         0x5dad,         0x537d,         0x4a68,
    0x4251,         0x3b1b,         0x34ad,         0x2ef3,         0x29d7,         0x254b,         0x213c,         0x1d9f,         0x1a66,         0x1787,
    0x14f8,
};

void i2s_mclk_init(uint32_t audio_clock){
    pio_sm_config sm_config;
    PIO pio = PIO_I2S;
    uint sm = PIO_I2S_SM;
    uint data_pin = PIN_I2S_DATA;
    uint clock_pin_base = PIN_I2S_CLK;
    uint func = GPIO_FUNC_PIO0;
    uint offset;
    float div;

    gpio_set_function(data_pin, func);
    gpio_set_function(clock_pin_base, func);
    gpio_set_function(clock_pin_base + 1, func);
    gpio_set_function(clock_pin_base + 2, func);

    offset = pio_add_program(pio, &i2s_mclk_256_program);
    sm_config = i2s_mclk_256_program_get_default_config(offset);
    
    sm_config_set_out_pins(&sm_config, data_pin, 1);
    sm_config_set_sideset_pins(&sm_config, clock_pin_base);
    sm_config_set_out_shift(&sm_config, false, false, 32);
    sm_config_set_fifo_join(&sm_config, PIO_FIFO_JOIN_TX);

    sm_config_set_set_pins(&sm_config, data_pin, 1);

    i2s_buf_length = 0;
    enqueue_pos = 0;
    dequeue_pos = 0;

    div = (float)clock_get_hz(clk_sys) / (float)(audio_clock * 512);
    sm_config_set_clkdiv(&sm_config, div);

    pio_sm_init(pio, sm, offset, &sm_config);

    uint pin_mask = (1u << data_pin) | (7u << clock_pin_base);
    pio_sm_set_pindirs_with_mask(pio, sm, pin_mask, pin_mask);
    pio_sm_set_pins(pio, sm, 1);

    pio_sm_exec(pio, sm, pio_encode_jmp(offset));
    pio_sm_clear_fifos(pio, sm);
    pio_sm_set_enabled(pio, sm, true);
}

void i2s_mclk_change_clock(uint32_t audio_clock){
    irq_set_enabled(DMA_IRQ_0, false);

    i2s_buf_length = 0;
    enqueue_pos = 0;
    dequeue_pos = 0;

    i2s_mclk_clock_set(audio_clock);

    irq_set_enabled(DMA_IRQ_0, true);
}

void i2s_mclk_clock_set(uint32_t audio_clock){
    //MCLKは256fs
    //pio_clock = MCLK * 2
    float div;
    div = (float)clock_get_hz(clk_sys) / (float)(audio_clock * 512);
    pio_sm_set_clkdiv(PIO_I2S, PIO_I2S_SM, div);
}

void i2s_mclk_dma_init(void){
    uint ch = DMA_I2S_CN;
    PIO pio = PIO_I2S;
    uint sm = PIO_I2S_SM;

    dma_channel_config conf = dma_channel_get_default_config(ch);
    
    channel_config_set_read_increment(&conf, true);
    channel_config_set_write_increment(&conf, false);
    channel_config_set_transfer_data_size(&conf, DMA_SIZE_32);
    channel_config_set_dreq(&conf, pio_get_dreq(pio, sm, true));
    
    dma_channel_configure(
        ch,
        &conf,
        &PIO_I2S->txf[PIO_I2S_SM],
        NULL,
        0,
        false
    );

    dma_channel_set_irq0_enabled(ch, true);
	irq_set_exclusive_handler(DMA_IRQ_0, i2s_handler);
    irq_set_priority(DMA_IRQ_0, 0);
    irq_set_enabled(DMA_IRQ_0, true);
}


//i2sのバッファにusb受信データを積む
bool enqueue(uint8_t* in, int sample, uint8_t resolution){
    int i, j;
	if (i2s_buf_length < BUF_DEPTH){
        j = 0;
        if (resolution == 16){
            int16_t *d = (int16_t*)in;
            sample /= 2;
            for (i = 0; i < sample / 2; i++){
                i2s_buf[enqueue_pos][j++] = volume_set(*d++ << 16, mul_l);
                i2s_buf[enqueue_pos][j++] = volume_set(*d++ << 16, mul_r);
            }
            i2s_sample[enqueue_pos] = sample;
        }
        else if (resolution == 24){
            uint8_t *d = in;
            int32_t e;
            sample /= 3;
            for (i = 0; i < sample / 2; i++){
                e = 0;
                e |= *d++ << 8;
                e |= *d++ << 16;
                e |= *d++ << 24;
                i2s_buf[enqueue_pos][j++] = volume_set(e, mul_l);
                e = 0;
                e |= *d++ << 8;
                e |= *d++ << 16;
                e |= *d++ << 24;
                i2s_buf[enqueue_pos][j++] = volume_set(e, mul_r);
            }
            i2s_sample[enqueue_pos] = sample;
        }
        else if (resolution == 32){
            int32_t *d = (int32_t*)in;
            sample /= 4;
            for (i = 0; i < sample / 2; i++){
                i2s_buf[enqueue_pos][j++] = volume_set(*d++, mul_l);
                i2s_buf[enqueue_pos][j++] = volume_set(*d++, mul_r);
            }
            i2s_sample[enqueue_pos] = sample;
        }
		
		enqueue_pos++;
		if (enqueue_pos >= BUF_DEPTH) enqueue_pos = 0;

		irq_set_enabled(DMA_IRQ_0, false);
		i2s_buf_length++;
		irq_set_enabled(DMA_IRQ_0, true);
		return true;
	}
	else return false;
}

void __isr __time_critical_func(i2s_handler)(){
	static bool mute;
	static int32_t mute_buff[1000*2] = {0};
	static uint32_t mute_len = sizeof(mute_buff) / sizeof(int32_t);
	
	if (i2s_buf_length == 0){
        mute = true;
    }
	else if (i2s_buf_length >= I2S_START_LEVEL && mute == true){
        mute = false;
    }

	if (!mute){
		dma_channel_transfer_from_buffer_now(DMA_I2S_CN, i2s_buf[dequeue_pos], i2s_sample[dequeue_pos]);

		dequeue_pos++;
		if (dequeue_pos >= BUF_DEPTH) dequeue_pos = 0;
		
		i2s_buf_length--;
	}
	else{
		dma_channel_transfer_from_buffer_now(DMA_I2S_CN, mute_buff, mute_len);
	}
    
   	dma_hw->ints0 = 1u << DMA_I2S_CN;
}

bool dequeue(int32_t** buff, int* sample){
    if (get_buf_length()){
        *buff = i2s_buf[dequeue_pos];
        *sample = i2s_sample[dequeue_pos];
        dequeue_pos++;
        if (dequeue_pos >= BUF_DEPTH) dequeue_pos = 0;

        i2s_buf_length--;

        return true;
    }
    else return false;
}

int8_t get_buf_length(void){
    int8_t d;
	d = i2s_buf_length;
    return d;
}

int32_t volume_set(int32_t d, int32_t mul){
    int32_t data;
	data = (int32_t)(((int64_t)d * mul) >> 29u);
    return data;
}

void volume_change(int16_t v, int8_t ch){
    if (ch == 1){
        mul_l = db_to_vol[-v >> 8];
    }
    else if (ch == 2){
        mul_r = db_to_vol[-v >> 8];
    }
}