#include "platform/process.h"

#include <unistd.h>

long process_id(void) {
    return (long)getpid();
}
