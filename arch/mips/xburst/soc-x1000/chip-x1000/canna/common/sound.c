
#include <mach/jzsnd.h>
#include "board_base.h"

#ifdef CONFIG_SOUND_OSS_CORE
struct snd_codec_data codec_data = {

};
#endif

#ifdef CONFIG_SND_ASOC_JZ_INCODEC
static struct ingenic_snd_codec_data snd_alsa_platform_data = {
	.gpio_spk_en = {.gpio = GPIO_SPEAKER_EN, .active_level = GPIO_SPEAKER_EN_LEVEL},
	.gpio_linein_sw = {.gpio = GPIO_LINEIN_DETECT, .active_level = GPIO_LINEIN_INSERT_LEVEL},
};

struct platform_device snd_alsa_device = {
	.name = "ingenic-alsa",
	.dev = {
		.platform_data = &snd_alsa_platform_data,
	},
};
#endif
