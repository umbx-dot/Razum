#include <assert.h>
#include <stdint.h>
#include <windows.h>

#ifdef LINK_LIFECYCLE
#include "lifecycle.h"
#else
typedef struct {
    uint32_t attaches;
    uint32_t detaches;
    uint32_t module_seen;
    uint32_t loaded_dynamically;
} DllState;
#endif

static uint32_t observed_reason = 99;

static void observe_detach(uint32_t reason) {
    observed_reason = reason;
}

#ifdef LINK_LIFECYCLE
static int32_t twice(int32_t value) {
    return 2 * value;
}
#else
static DWORD WINAPI thread_proc(void *parameter) {
    (void)parameter;
    return 0;
}
#endif

int main(void) {
#ifdef LINK_LIFECYCLE
    assert(dll_state.module_seen == 1);
    assert(dll_state.loaded_dynamically == 0);
    assert(apply_callback(twice, -21) == -42);
    detach_observer = observe_detach;
#else
    HMODULE dll = LoadLibraryA("lifecycle.dll");
    assert(dll != NULL);
    assert(GetProcAddress(dll, "DllMain") == NULL);
    DllState *state = (DllState *)(uintptr_t)GetProcAddress(dll, "dll_state");
    void (**observer)(uint32_t) = (void (**)(uint32_t))(uintptr_t)GetProcAddress(dll, "detach_observer");
    assert(state != NULL && observer != NULL);
    assert(state->module_seen == 1 && state->loaded_dynamically == 1);
    const uint32_t attaches = state->attaches;
    const uint32_t detaches = state->detaches;
    *observer = observe_detach;
    HANDLE thread = CreateThread(NULL, 0, thread_proc, NULL, 0, NULL);
    assert(thread != NULL && WaitForSingleObject(thread, 5000) == WAIT_OBJECT_0);
    assert(CloseHandle(thread));
    assert(state->attaches == attaches + 1 && state->detaches == detaches + 1);
    assert(FreeLibrary(dll));
    assert(observed_reason == DLL_PROCESS_DETACH);
#endif
    assert(LoadLibraryA("reject_load.dll") == NULL);
    assert(GetLastError() == ERROR_DLL_INIT_FAILED);
    return 0;
}
