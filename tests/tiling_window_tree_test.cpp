#include <gtest/gtest.h>
#include "tiling_window_tree.h"
#include "stub_tiling_interface.h"
#include "miracle_config.h"
#include <miral/window_management_options.h>

using namespace miracle;

class TilingWindowTreeTest : public testing::Test
{
public:
    TilingWindowTreeTest()
        : tree(nullptr, tiling_interface, )
    {
    }

    void SetUp() override
    {
    }

    void TearDown() override
    {
    }

    test::StubTilingInterface tiling_interface;
    std::shared_ptr<MiracleConfig> config;
    TilingWindowTree tree;
};