#include "platform/process.h"

#include <process.h>

long process_id(void) {
    return (long)_getpid();
}
