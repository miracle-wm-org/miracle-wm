# Releasing
## Step 0: Preparation
1. Bump the miracle version number.

## Step 1: Create a Release on Github
1. Navigate to https://github.com/mattkae/miracle-wm/releases
2. Draft a new release
3. Name the tag `v.X.Y.Z`
4. Title the release `v.X.Y.Z`
5. Target a branch, most likely `develop`
6. Describe the release (You may generate release notes, but please make sure that they make sense before doing so)

## Step 2: Snap Release
This happens automatically in CI.

## Step 3: Deb Release
This only works if you have the GPG key that is associated in launchpad.

1. Run `./tools/publish-ppa.sh X.Y.Z` following the tag of a release.
2. Commit the changelog, for historical purposes.

## Step 4: RPM Release
Before following these steps, make sure that you've at least followed [this tutorial](https://www.redhat.com/sysadmin/create-rpm-package).

1. `fkinit`
2. `rpmdev-bumpspec -n 0.3.0 -c "Update to 0.3.0" miracle-wm.spec`
3. `rpmdev-spectool -g miracle-wm.spec`
4. `fedpkg new-sources miracle-wm-0.3.0.tar.gz`
5. `fedpkg ci -m "Update to 0.3.0"`
6. `fedpkg push && fedpkg build`
