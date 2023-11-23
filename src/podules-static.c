#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "arc.h"
#include "config.h"
#include "podules.h"

typedef const podule_header_t* (*podule_probe_t)(const podule_callbacks_t *callbacks, char *path);
#define PODULE_PROBE_PROTOTYPE(name) const podule_header_t *name##_podule_probe(const podule_callbacks_t *callbacks, char *path);

#ifdef PODULE_ultimatecdrom
PODULE_PROBE_PROTOTYPE(ultimatecdrom)
#endif

static void init(char* name, podule_probe_t probe)
{
    char path[256];
    const podule_header_t *header;

    snprintf(path, 256, "podules/%s/", name);
    header = probe(&podule_callbacks_def, path);

    rpclog("probing %s\n", name);
    if (!header) {
        rpclog("podule_probe %s failed\n", name);
        return;
    }
    uint32_t valid_flags = podule_validate_and_get_valid_flags(header);
    if (!valid_flags)
    {
        rpclog("podule_probe failed validation %s\n", name);
        return;
    }

    uint32_t flags;
    do
    {
        flags = header->flags;
        if (flags & ~valid_flags)
        {
            rpclog("podule_probe: podule header fails flags validation\n");
            return;
        }
        podule_add(header);
        header++;
    } while (flags & PODULE_FLAGS_NEXT);
}

void opendlls()
{
#ifdef PODULE_ultimatecdrom
    init("ultimatecdrom", ultimatecdrom_podule_probe);
#endif
}
