#include <stdbool.h>

#include "esp_log.h"

#include "eos_port.h"

static const char *TAG = "eos_audio";

bool eos_speaker_detect(void)
{
    return false;
}

bool eos_microphone_detect(void)
{
    return false;
}

eos_audio_state_t eos_audio_get_state(void)
{
    return EOS_AUDIO_STATE_UNAVAILABLE;
}

int eos_audio_play_file(const char *file_path)
{
    (void)file_path;
    return -1;
}

int eos_audio_stop(void)
{
    return -1;
}

void eos_port_audio_init(void)
{
    ESP_LOGI(TAG, "Audio unavailable");
}
