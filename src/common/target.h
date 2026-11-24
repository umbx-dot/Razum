#pragma once

typedef enum {
    TARGET_WINDOWS,
    TARGET_LINUX,
} Target;

Target host_target(void);
