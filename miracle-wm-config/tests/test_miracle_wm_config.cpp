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

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <miracle/miracle-wm-config.h>

class KeymapConfigurationTest : public testing::Test
{
};

TEST_F(KeymapConfigurationTest, KeymapToString)
{
    miracle::KeymapConfiguration config;
    config.language = "en";
    config.variant = "qwerty";
    config.options.emplace_back("a");
    config.options.emplace_back("b");
    EXPECT_EQ(config.to_string(), "en+qwerty+a+b");
}

TEST_F(KeymapConfigurationTest, KeymapLanguageOnly)
{
    miracle::KeymapConfiguration config;
    config.language = "en";
    EXPECT_EQ(config.to_string(), "en");
}