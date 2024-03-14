# Releasing
## Step 1: Create a Release on Github
1. Navigate to https://github.com/mattkae/miracle-wm/releases
2. Draft a new release
3. Name the tag `v.X.Y.Z`
4. Title the release `v.X.Y.Z`
5. Target a branch, most likely `master`
6. Describe the release (You may generate release notes, but please make sure that they make sense before doing so)

## Step 2: Snap Release
1. Check out the commit that you just released in Step 1
2. Bump the version number in `snap/snapcraft.yaml` to the `X.Y.Z`
3. Commit this buumped version
4. Next (this part is silly), comment out the `override-pull` of the `miracle-wm` part
5. Run `snapcraft`
6. Finally, run `snapcraft upload --release=stable ./miracle-wm_*.snap`

TODO: Implement https://github.com/mattkae/miracle-wm/issues/59 to fix the weirdness of this process

## Step 3: Deb Release
1. Clone the repo (make sure that the folder is called `miracle-wm`)
2. Update `debian/changelog` with:
    - Version `X.Y.Z`
    - The same content as you in the Github release
    - A correct current timestamp
3. Next:
```sh
cd miracle-wm
rm -rf build
debuild -S -sd
cd ..
dput ppa:matthew-kosarek/miracle-wm miracle-*_source.changes
```
4. Navigate to https://launchpad.net/~matthew-kosarek/+archive/ubuntu/miracle-wm
5. Wait for the CI to finish, and the package should be ready