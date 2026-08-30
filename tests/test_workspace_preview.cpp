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

#define GLM_ENABLE_EXPERIMENTAL
#include "workspace_preview.h"

#include "mock_output.h"
#include "mock_workspace.h"

#include <glm/gtx/transform.hpp>
#include <gtest/gtest.h>

using namespace miracle;
using namespace testing;

namespace
{
using MockWorkspacePtr = std::shared_ptr<NiceMock<test::MockWorkspace>>;

/// The state a workspace is left in by its hide animation: translated off
/// screen and fully transparent.
glm::mat4 const HIDDEN_TRANSFORM = glm::translate(glm::mat4(1.f), glm::vec3(1280, 0, 0));

/// A workspace that is out of the scene, and so has to be revealed and then
/// concealed again.
MockWorkspacePtr revealable()
{
    return std::make_shared<NiceMock<test::MockWorkspace>>();
}

/// Puts \p workspace on an output. The output holds the workspace weakly, so
/// that a test can still drop the workspace and watch the preview cope.
std::shared_ptr<test::MockOutput> give_output(MockWorkspacePtr const& workspace)
{
    auto output = std::make_shared<NiceMock<test::MockOutput>>();
    ON_CALL(*workspace, get_output())
        .WillByDefault(Return(output));
    return output;
}

/// A workspace that is the active one on its output, and so is already in the
/// scene and left alone.
MockWorkspacePtr already_visible()
{
    auto workspace = revealable();
    auto const output = give_output(workspace);
    ON_CALL(*output, active())
        .WillByDefault(Invoke([weak = std::weak_ptr<AbstractWorkspace>(workspace)]
    {
        return weak.lock();
    }));
    return workspace;
}
}

TEST(WorkspacePreviewTest, AcquireRevealsEveryWorkspace)
{
    auto const first = revealable();
    auto const second = revealable();
    EXPECT_CALL(*first, set_containers_shown(false)).Times(AnyNumber());
    EXPECT_CALL(*second, set_containers_shown(false)).Times(AnyNumber());
    EXPECT_CALL(*first, set_containers_shown(true)).Times(1);
    EXPECT_CALL(*second, set_containers_shown(true)).Times(1);

    WorkspacePreview preview;
    preview.acquire({ first, second });
    EXPECT_TRUE(preview.held());

    preview.release();
}

TEST(WorkspacePreviewTest, AcquireSnapsTheTransformAndAlphaToTheirShownValues)
{
    auto const workspace = revealable();
    ON_CALL(*workspace, transform())
        .WillByDefault(Return(HIDDEN_TRANSFORM));
    ON_CALL(*workspace, alpha())
        .WillByDefault(Return(0.f));

    // The preview is released when it goes out of scope, which puts these back.
    EXPECT_CALL(*workspace, transform(HIDDEN_TRANSFORM)).Times(AnyNumber());
    EXPECT_CALL(*workspace, alpha(0.f)).Times(AnyNumber());

    // Otherwise the windows go back into the scene off screen and invisible.
    EXPECT_CALL(*workspace, transform(glm::mat4(1.f))).Times(1);
    EXPECT_CALL(*workspace, alpha(1.f)).Times(1);

    WorkspacePreview preview;
    preview.acquire({ workspace });
}

TEST(WorkspacePreviewTest, ReleaseRestoresTheTransformAndAlphaThatWereThereBefore)
{
    auto const workspace = revealable();
    ON_CALL(*workspace, transform())
        .WillByDefault(Return(HIDDEN_TRANSFORM));
    ON_CALL(*workspace, alpha())
        .WillByDefault(Return(0.f));

    InSequence seq;
    EXPECT_CALL(*workspace, transform(glm::mat4(1.f))).Times(1);
    EXPECT_CALL(*workspace, alpha(1.f)).Times(1);
    EXPECT_CALL(*workspace, set_containers_shown(true)).Times(1);
    EXPECT_CALL(*workspace, set_containers_shown(false)).Times(1);
    EXPECT_CALL(*workspace, transform(HIDDEN_TRANSFORM)).Times(1);
    EXPECT_CALL(*workspace, alpha(0.f)).Times(1);

    WorkspacePreview preview;
    preview.acquire({ workspace });
    preview.release();
}

TEST(WorkspacePreviewTest, ReleaseConcealsExactlyWhatWasRevealed)
{
    auto const revealed = revealable();
    auto const visible = already_visible();
    EXPECT_CALL(*revealed, set_containers_shown(true)).Times(1);
    EXPECT_CALL(*revealed, set_containers_shown(false)).Times(1);
    EXPECT_CALL(*visible, set_containers_shown(_)).Times(0);

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
    EXPECT_CALL(*workspace, set_containers_shown(true)).Times(1);
    EXPECT_CALL(*workspace, set_containers_shown(false)).Times(1);

    WorkspacePreview preview;
    preview.acquire({ workspace });
    preview.release();
    preview.release();
}

TEST(WorkspacePreviewTest, AcquireIsAdditive)
{
    auto const first = revealable();
    auto const second = revealable();
    EXPECT_CALL(*first, set_containers_shown(true)).Times(1);
    EXPECT_CALL(*second, set_containers_shown(true)).Times(1);
    EXPECT_CALL(*first, set_containers_shown(false)).Times(1);
    EXPECT_CALL(*second, set_containers_shown(false)).Times(1);

    WorkspacePreview preview;
    preview.acquire({ first });
    preview.acquire({ second });
    preview.release();
}

TEST(WorkspacePreviewTest, AcquiringTheSameWorkspaceTwiceRevealsItOnce)
{
    auto const workspace = revealable();
    EXPECT_CALL(*workspace, set_containers_shown(false)).Times(AnyNumber());
    EXPECT_CALL(*workspace, set_containers_shown(true)).Times(1);

    // A second reveal would snapshot the transform and alpha of a workspace that
    // is already showing, and the state it was hidden in would be lost.
    WorkspacePreview preview;
    preview.acquire({ workspace });
    preview.acquire({ workspace });
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
    EXPECT_CALL(*workspace, set_containers_shown(true)).Times(1);
    EXPECT_CALL(*workspace, set_containers_shown(false)).Times(0);

    WorkspacePreview preview;
    preview.acquire({ workspace });

    // The preview holds the workspace weakly, so a workspace that is deleted
    // while the effect is running simply stops resolving.
    workspace.reset();
    preview.release();
}

TEST(WorkspacePreviewTest, ReleaseLeavesAWorkspaceThatBecameActiveInTheScene)
{
    auto const workspace = revealable();
    auto const output = give_output(workspace);
    ON_CALL(*workspace, transform())
        .WillByDefault(Return(HIDDEN_TRANSFORM));
    EXPECT_CALL(*workspace, set_containers_shown(true)).Times(1);
    EXPECT_CALL(*workspace, set_containers_shown(false)).Times(0);
    EXPECT_CALL(*workspace, transform(glm::mat4(1.f))).Times(1);
    EXPECT_CALL(*workspace, transform(HIDDEN_TRANSFORM)).Times(0);

    WorkspacePreview preview;
    preview.acquire({ workspace });

    // An effect that ends by adopting the workspace it was previewing leaves it
    // as the active one. Putting the preview away must not undo that.
    ON_CALL(*output, active())
        .WillByDefault(Invoke([weak = std::weak_ptr<AbstractWorkspace>(workspace)]
    {
        return weak.lock();
    }));
    preview.release();
}

TEST(WorkspacePreviewTest, DestructionReleasesWhatIsStillHeld)
{
    auto const workspace = revealable();
    EXPECT_CALL(*workspace, set_containers_shown(true)).Times(1);
    EXPECT_CALL(*workspace, set_containers_shown(false)).Times(1);

    {
        WorkspacePreview preview;
        preview.acquire({ workspace });
    }
}
