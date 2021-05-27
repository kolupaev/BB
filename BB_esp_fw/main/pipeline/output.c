/*
 * downmix.c
 *
 *  Created on: 19. 1. 2021
 *      Author: horinek
 */

#include "output.h"

#include "i2s_stream.h"
#include "downmix.h"

static const char *TAG = "PIPE_OUT";

void pipe_output_init()
{
    INFO("pipe_downmix_init");

    //create PIPE
    audio_pipeline_cfg_t output_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    pipes.output.pipe = audio_pipeline_init(&output_cfg);

    //Config
    i2s_stream_cfg_t i2s_cfg =  {
            .type = AUDIO_STREAM_WRITER,
            .i2s_config = {
                .mode = I2S_MODE_MASTER | I2S_MODE_TX,
                .sample_rate = OUTPUT_SAMPLERATE,
                .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
                .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
                .communication_format = I2S_COMM_FORMAT_STAND_I2S,
                .intr_alloc_flags = ESP_INTR_FLAG_LEVEL2, // | ESP_INTR_FLAG_IRAM,
                .dma_buf_count = 3,
                .dma_buf_len = 300,
                .use_apll = false,
                .tx_desc_auto_clear = true,
                .fixed_mclk = 0
            },
            .i2s_port = I2S_PORT,
            .use_alc = false,
            .volume = 0,
            .out_rb_size = I2S_STREAM_RINGBUFFER_SIZE,
            .task_stack = I2S_STREAM_TASK_STACK,
            .task_core = I2S_STREAM_TASK_CORE,
            .task_prio = I2S_STREAM_TASK_PRIO,
            .stack_in_ext = false,
            .multi_out_num = 0,
            .uninstall_drv = true,
        };

    pipes.output.i2s_writer = i2s_stream_init(&i2s_cfg);

    downmix_cfg_t downmix_cfg = DEFAULT_DOWNMIX_CONFIG();
    downmix_cfg.downmix_info.source_num = NUMBER_OF_SOURCES;
    downmix_cfg.downmix_info.mode = ESP_DOWNMIX_WORK_MODE_SWITCH_ON;

    pipes.output.mix = downmix_init(&downmix_cfg);

    esp_downmix_input_info_t source_info = {
        .samplerate = OUTPUT_SAMPLERATE,
        .channel = OUTPUT_CHANNELS,
        .gain = {0, 0},
        .transit_time = 0,
    };

    esp_downmix_input_info_t source_information[NUMBER_OF_SOURCES] = {0};
    for (uint8_t i = 0; i < NUMBER_OF_SOURCES; i++)
        source_information[i] = source_info;

    source_info_init(pipes.output.mix, source_information);
    downmix_set_output_type(pipes.output.mix, ESP_DOWNMIX_OUTPUT_TYPE_ONE_CHANNEL);
    downmix_set_work_mode(pipes.output.mix, ESP_DOWNMIX_WORK_MODE_SWITCH_ON);

    audio_pipeline_register(pipes.output.pipe, pipes.output.mix, "mix");
    audio_pipeline_register(pipes.output.pipe, pipes.output.i2s_writer, "i2s");

    const char *link[2] = {"mix", "i2s"};
    audio_pipeline_link(pipes.output.pipe, &link[0], 2);

    audio_pipeline_run(pipes.output.pipe);
}
