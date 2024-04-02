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
    - Version `X.Y.Z-distro` (where `distro` is "jammy", "noble", or "mantic")
    - The same content as you in the Github release
    - A correct current timestamp
    - `jammy` as the release (repeat with `mantic` and `noble`)
3. Next:
```sh
cd miracle-wm
./tools/publish-ppa.sh <X.Y.Z> <DISTRO>
```
4. Navigate to https://launchpad.net/~matthew-kosarek/+archive/ubuntu/miracle-wm
5. Wait for the CI to finish, and the package should be ready

Note that you should rebuild for `mantic` and `noble`. Follow the instructions to make sure that it uploads.

## Step 4: RPM Release
Before following these steps, make sure that you've at least followed [this tutorial](https://www.redhat.com/sysadmin/create-rpm-package).

1. Open up `rpm/miracle-wm.spec`
2. Update the version to `X.Y.Z`
3. Updated `BuildRequires` or `Requires` if need be
4. Add a new entry to the changelog with the same data that is in the Github release, your user name, and a correct timestamp
5. Move to a directory above `miracle-wm` and run:
   ```
    tar --create --file miracle-wm-X.Y.Z.tar.gz miracle-wm
    ```
6. `mv miracle-wm-X.Y.Z.tar.gz ~/rpmbuild/SOURCES`
7. `cp miracle-wm/specs/miracle-wm.spec ~/rpmbuild/SPECS/`
8. `rpmbuild -bb ~/rpmbuild/SPECS/miracle-wm.spec`
9. After the build, you should be able to install your rpm:
   ```sh
   sudo dnf install ~/rpmbuild/RPMS/<ARCH_TRIPLET>/miracle-wm-X.Y.Z-<REVISION>.<HASH>.<ARCH_TRIPLET>.rpm
   ```
