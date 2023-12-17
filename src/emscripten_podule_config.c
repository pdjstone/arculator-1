#include "podules.h"
#include <stddef.h>

#include "arc.h"

void *podule_config_get_current(void *window_p, int id)
{
    rpclog("podule_config_get_current: window_p=%p id=%d\n", window_p, id);
    return NULL;
}

void podule_config_set_current(void *window_p, int id, void *val)
{
    rpclog("podule_config_set_current: window_p=%p id=%d val=%p\n", window_p, id, val);
}

int podule_config_file_selector(void *window_p, const char *title, const char *default_path, const char *default_fn, const char *default_ext, const char *wildcard, char *dest, int dest_len, int flags)
{
    rpclog("podule_config_file_selector: window_p=%p title=%s default_path=%s default_fn=%s default_ext=%s wildcard=%s dest=%p dest_len=%d flags=%d\n", window_p, title, default_path, default_fn, default_ext, wildcard, dest, dest_len, flags);
    return 0;
}

int podule_config_open(void *window_p, podule_config_t *config, const char *prefix)
{
    rpclog("podule_config_open: window_p=%p config=%p prefix=%s\n", window_p, config, prefix);
    return 0;
}
