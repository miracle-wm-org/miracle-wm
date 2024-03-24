#include <gtest/gtest.h>
#include "tree.h"
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

    std::shared_ptr<Tree> tree;
};