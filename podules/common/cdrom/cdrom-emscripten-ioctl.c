#include "cdrom.h"
#include "podule_api.h"

void ioctl_reset()
{
}

void ioctl_set_drive(const char *path)
{
}

int ioctl_open(char d)
{
    return 0;
}

void ioctl_close(void)
{
}

void ioctl_audio_callback(int16_t *output, int len)
{
}

podule_config_selection_t *cdrom_devices_config(void)
{
    return NULL;
}
