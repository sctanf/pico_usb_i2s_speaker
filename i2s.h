#ifndef I2S_H
#define I2S_H

#define BUF_DEPTH   20
#define I2S_START_LEVEL 5

#define PIO_I2S         pio0
#define PIO_I2S_SM      0
#define DMA_I2S_CN      0
#define PIN_I2S_DATA    2
#define PIN_I2S_CLK     3

void i2s_mclk_init(uint32_t audio_clock);
void i2s_mclk_change_clock(uint32_t audio_clock);
void i2s_mclk_clock_set(uint32_t audio_clock);
void i2s_mclk_dma_init(void);
bool enqueue(uint8_t* in, int sample, uint32_t resolution);
void i2s_handler(void);
bool dequeue(int32_t** buff, int* sample);
int8_t get_buf_length(void);
int32_t volume_set(int32_t d, int32_t mul);
void volume_change(int16_t v, int8_t ch);

#endif