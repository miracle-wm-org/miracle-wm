#include "tiling_window_tree.h"
#include <gtest/gtest.h>
#include <miral/window_management_options.h>

using namespace miracle;

class TreeTest : public testing::Test
{
public:
    TreeTest()
    {
    }

    void SetUp() override
    {
    }

    void TearDown() override
    {
    }

    std::shared_ptr<TilingWindowTree> tree;
};