/**
Copyright (C) 2024  Matthew Kosarek

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
**/

#include "i3_command.h"
#include <gtest/gtest.h>

using namespace miracle;

class I3CommandTest : public testing::Test
{
};

TEST_F(I3CommandTest, TestClassParsing)
{
    std::string v = "[class=\"XYZ\"]";
    int ptr;
    auto scope = I3Scope::parse(v, ptr);
    ASSERT_EQ(scope[0].type, I3ScopeType::class_);
    ASSERT_EQ(scope[0].regex.value(), "XYZ");
}

TEST_F(I3CommandTest, TestAllParsing)
{
    std::string v = "[all]";
    int ptr;
    auto scope = I3Scope::parse(v, ptr);
    ASSERT_EQ(scope[0].type, I3ScopeType::all);
}

TEST_F(I3CommandTest, TestMultipleParsing)
{
    std::string v = "[class=\"Firefox\" window_role=\"About\"]";
    int ptr;
    auto scope = I3Scope::parse(v, ptr);
    ASSERT_EQ(scope[0].type, I3ScopeType::class_);
    ASSERT_EQ(scope[0].regex.value(), "Firefox");
    ASSERT_EQ(scope[1].type, I3ScopeType::window_role);
    ASSERT_EQ(scope[1].regex.value(), "About");
}

TEST_F(I3CommandTest, TestComplexClassParsing)
{
    std::string v = "[class=\"^(?i)(?!firefox)(?!gnome-terminal).*\"]";
    int ptr;
    auto scope = I3Scope::parse(v, ptr);
    ASSERT_EQ(scope[0].type, I3ScopeType::class_);
    ASSERT_EQ(scope[0].regex.value(), "^(?i)(?!firefox)(?!gnome-terminal).*");
}

TEST_F(I3CommandTest, TestTilingParsing)
{
    std::string v = "[tiling ]";
    int ptr;
    auto scope = I3Scope::parse(v, ptr);
    ASSERT_EQ(scope[0].type, I3ScopeType::tiling);
}

TEST_F(I3CommandTest, TestFloatingParsing)
{
    std::string v = "[floating ]";
    int ptr;
    auto scope = I3Scope::parse(v, ptr);
    ASSERT_EQ(scope[0].type, I3ScopeType::floating);
}

TEST_F(I3CommandTest, CanParseSingleI3Command)
{
    std::string v = "exec gedit";
    auto commands = I3ScopedCommandList::parse(v);
    ASSERT_EQ(commands[0].commands.size(), 1);
    ASSERT_EQ(commands[0].commands[0].type, I3CommandType::exec);
    ASSERT_EQ(commands[0].commands[0].arguments[0], "gedit");
}

TEST_F(I3CommandTest, CanParseExecCommandWithNoStartupId)
{
    std::string v = "exec --no-startup-id gedit";
    auto commands = I3ScopedCommandList::parse(v);
    ASSERT_EQ(commands[0].commands.size(), 1);
    ASSERT_EQ(commands[0].commands[0].type, I3CommandType::exec);
    ASSERT_EQ(commands[0].commands[0].arguments[0], "--no-startup-id");
    ASSERT_EQ(commands[0].commands[0].arguments[1], "gedit");
}