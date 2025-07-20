#ifndef DRV_DMEM_H
#define	DRV_DMEM_H

#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>

#define DMEM_DEBUG

#ifdef	__cplusplus
extern "C" {
#endif

#if defined(DMEM_DEBUG)

    void *dmem_create(size_t size);
    void *dmem_extend(void *ptr, size_t newSize);
    void dmem_release(void *ptr);

#else

#define dmem_create malloc
#define dmem_extend realloc
#define dmem_release free

#endif

    void *dmem_create_w_data(uint8_t *data, size_t size);

#ifdef	__cplusplus
}
#endif

#endif	/* DRV_DMEM_H */

