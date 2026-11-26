#include "project.h"

#include <string.h>

#include "common/files.h"
#include "platform/filesystem.h"

void rz_build_cmake(const RzProject *project, const RzTarget *target, const RzOptions *options, const char *stage) {
    const char *cmake = rz_tool("cmake", NULL);
    const char *source = rz_join(project->root, target->source);
    const char *build = rz_join(project->root, rz_format("build/.cmake/%s/%s/%s", rz_platform(), options->profile, target->name));
    const char *inputs = rz_join(build, "razum-inputs");
    rz_stage_native_inputs(project, target, stage, inputs);
    const char *configuration = strcmp(options->profile, "release") == 0 ? "Release" : "Debug";
    PointerList configure = {0};
    rz_add_argument(&configure, cmake);
    if (target->preset != NULL) {
        rz_add_argument(&configure, "--preset");
        rz_add_argument(&configure, target->preset);
    }
    rz_add_argument(&configure, "-S");
    rz_add_argument(&configure, source);
    rz_add_argument(&configure, "-B");
    rz_add_argument(&configure, build);
    rz_add_argument(&configure, rz_format("-DCMAKE_BUILD_TYPE=%s", configuration));
    rz_add_argument(&configure, options->offline ? "-DFETCHCONTENT_FULLY_DISCONNECTED=ON" : "-DFETCHCONTENT_FULLY_DISCONNECTED=OFF");
    rz_add_argument(&configure, rz_format("-DRAZUM_ARTIFACT_DIR:PATH=%s", inputs));
    rz_add_argument(&configure, rz_format("-DRAZUM_COMPILER:FILEPATH=%s", rz_tool("rzc", "RZC")));
    rz_run(configure, source);

    PointerList arguments = {0};
    const char *const command[] = {cmake, "--build", build, "--config", configuration, "--target",
                                   target->cmake_target != NULL ? target->cmake_target : target->name, NULL};
    for (int index = 0; command[index] != NULL; index++) {
        rz_add_argument(&arguments, command[index]);
    }
    rz_run(arguments, source);
    size_t size = 0;
    const char *artifact = rz_join(build, target->artifact);
    const char *contents = read_file(artifact, &size);
    const char *destination = rz_join(stage, rz_target_filename(target));
    if (contents == NULL || !write_file(destination, contents, size) || !make_executable(destination)) {
        rz_error("target '%s': cannot stage CMake artifact %s", target->name, artifact);
    }
}
