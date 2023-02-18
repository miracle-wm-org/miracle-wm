#include <cstdlib>
#include <cstdio>

#include <miral/runner.h>
#include <miral/minimal_window_manager.h>
#include <miral/set_window_management_policy.h>

int main(int argc, char const* argv[]) {
    MirRunner runner{argc, argv};

    return runner.run_with(
        {
            set_window_management_policy<MinimalWindowManager>()
        });
    return EXIT_SUCCESS;
}