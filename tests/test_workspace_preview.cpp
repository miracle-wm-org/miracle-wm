/**
Copyright (C) 2025  Matthew Kosarek

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

#include "workspace_preview.h"

#include "mock_workspace.h"

#include <gtest/gtest.h>

using namespace miracle;
using namespace testing;

namespace
{
/// A workspace that reports it was revealed, and so has to be concealed again.
std::shared_ptr<NiceMock<test::MockWorkspace>> revealable()
{
    auto workspace = std::make_shared<NiceMock<test::MockWorkspace>>();
    ON_CALL(*workspace, begin_preview()).WillByDefault(Return(true));
    return workspace;
}

/// A workspace that is already in the scene, and so is left alone.
std::shared_ptr<NiceMock<test::MockWorkspace>> already_visible()
{
    auto workspace = std::make_shared<NiceMock<test::MockWorkspace>>();
    ON_CALL(*workspace, begin_preview()).WillByDefault(Return(false));
    return workspace;
}
}

TEST(WorkspacePreviewTest, AcquireRevealsEveryWorkspace)
{
    auto const first = revealable();
    auto const second = revealable();
    EXPECT_CALL(*first, begin_preview()).Times(1);
    EXPECT_CALL(*second, begin_preview()).Times(1);

    WorkspacePreview preview;
    preview.acquire({ first, second });
    EXPECT_TRUE(preview.held());

    preview.release();
}

TEST(WorkspacePreviewTest, ReleaseConcealsExactlyWhatWasRevealed)
{
    auto const revealed = revealable();
    auto const visible = already_visible();
    EXPECT_CALL(*revealed, end_preview()).Times(1);
    EXPECT_CALL(*visible, end_preview()).Times(0);

    WorkspacePreview preview;
    preview.acquire({ revealed, visible });
    preview.release();
    EXPECT_FALSE(preview.held());
}

TEST(WorkspacePreviewTest, NothingIsHeldWhenNothingWasRevealed)
{
    auto const visible = already_visible();

    WorkspacePreview preview;
    preview.acquire({ visible });
    EXPECT_FALSE(preview.held());
}

TEST(WorkspacePreviewTest, ReleaseIsIdempotent)
{
    auto const workspace = revealable();
    EXPECT_CALL(*workspace, end_preview()).Times(1);

    WorkspacePreview preview;
    preview.acquire({ workspace });
    preview.release();
    preview.release();
}

TEST(WorkspacePreviewTest, AcquireIsAdditive)
{
    auto const first = revealable();
    auto const second = revealable();
    EXPECT_CALL(*first, end_preview()).Times(1);
    EXPECT_CALL(*second, end_preview()).Times(1);

    WorkspacePreview preview;
    preview.acquire({ first });
    preview.acquire({ second });
    preview.release();
}

TEST(WorkspacePreviewTest, NullWorkspacesAreSkipped)
{
    WorkspacePreview preview;
    preview.acquire({ nullptr });
    EXPECT_FALSE(preview.held());
}

TEST(WorkspacePreviewTest, AWorkspaceThatDiesMidPreviewIsSkipped)
{
    auto workspace = revealable();
    EXPECT_CALL(*workspace, end_preview()).Times(0);

    WorkspacePreview preview;
    preview.acquire({ workspace });

    // The preview holds the workspace weakly, so a workspace that is deleted
    // while the effect is running simply stops resolving.
    workspace.reset();
    preview.release();
}

TEST(WorkspacePreviewTest, DestructionReleasesWhatIsStillHeld)
{
    auto const workspace = revealable();
    EXPECT_CALL(*workspace, end_preview()).Times(1);

    {
        WorkspacePreview preview;
        preview.acquire({ workspace });
    }
}
