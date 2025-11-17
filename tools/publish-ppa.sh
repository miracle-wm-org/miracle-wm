#!/bin/bash
set -e  # Exit on any error

# Set email for changelog entries (can be overridden by environment)
export DEBEMAIL="${DEBEMAIL:-matthew@matthewkosarek.xyz}"
export DEBFULLNAME="${DEBFULLNAME:-Matthew Kosarek}"

if (( $# < 1 )); then
    echo "Usage: ./publish-ppa.sh <NEW_VERSION> [DISTRO1] [DISTRO2] ..."
    echo "       ./publish-ppa.sh auto [DISTRO1] [DISTRO2] ..."
    echo "Example: ./publish-ppa.sh 0.8.1 noble mantic jammy"
    echo "Example: ./publish-ppa.sh auto noble  # Auto-detect version from git"
    exit 1
fi

version=$1
shift
distros=("$@")

# Auto-detect version from git tags if requested
if [ "$version" = "auto" ]; then
    cd $(dirname $0)/..
    version=$(git describe --tags --abbrev=0 2>/dev/null | sed 's/^v//' || echo "")
    if [ -z "$version" ]; then
        echo "Error: Could not auto-detect version. No git tags found."
        echo "Please specify version manually or create a git tag."
        exit 1
    fi
    echo "Auto-detected version: $version"
    cd - >/dev/null
fi

# Default to quantal quokka and noble numbat if no distros specified
if [ ${#distros[@]} -eq 0 ]; then
    distros=("noble" "jammy")
fi

dir=$(dirname $0)
cd $dir/..

echo "Creating upstream tarball for version $version..."
git archive --format=tar.gz --prefix=miracle-wm-$version/ HEAD > ../miracle-wm_$version.orig.tar.gz

echo "Publishing version $version to PPAs for: ${distros[*]}"

# Extract source to clean directory for building
echo "Extracting source to clean directory..."
cd ..
rm -rf miracle-wm-$version
tar -xzf miracle-wm_$version.orig.tar.gz
cd miracle-wm-$version

# Clean up any existing debian/files to start fresh
echo "Cleaning debian/files..."
rm -f debian/files

# Update changelog for each distro
for distro in "${distros[@]}"; do
    echo "Building for $distro..."
    
    # Get recent commits for the changelog entry
    recent_changes=$(cd ../miracle-wm && git log --oneline --no-merges -3 | sed 's/^[a-f0-9]* /* /' | tr '\n' '\n')
    
    # Create changelog entry with meaningful content
    DEBEMAIL="matthew@matthewkosarek.xyz" DEBFULLNAME="Matthew Kosarek" dch -D $distro -v ${version}-${distro} --force-distribution "Release version $version"
    
    # Add a few recent commits to make the changelog more informative
    if [ -n "$recent_changes" ]; then
        DEBEMAIL="matthew@matthewkosarek.xyz" DEBFULLNAME="Matthew Kosarek" dch -a "Recent changes:"
        echo "$recent_changes" | head -3 | while read line; do
            if [ -n "$line" ]; then
                DEBEMAIL="matthew@matthewkosarek.xyz" DEBFULLNAME="Matthew Kosarek" dch -a "$line"
            fi
        done
    fi
    
    # Clean and build
    rm -rf build
    rm -f debian/files  # Clean debian/files before each build
    
    echo "Building source package for $distro..."
    # Use -k option to specify GPG key if GPGKEY is set
    if [ -n "$GPGKEY" ]; then
        debuild -S -sa -d -k"$GPGKEY"
    else
        debuild -S -sa -d
    fi
    
    # Upload
    echo "Uploading to PPA..."
    dput ppa:matthew-kosarek/miracle-wm ../miracle-wm_${version}-${distro}_source.changes
    
    echo "Successfully uploaded $version for $distro"
done

# Return to original directory
cd ../miracle-wm

echo "All uploads completed!"

# Cleanup
echo "Cleaning up generated files..."
cd ..
rm -f miracle-wm_$version.orig.tar.gz
rm -f miracle-wm_${version}-*.tar.xz
rm -f miracle-wm_${version}-*.dsc
rm -f miracle-wm_${version}-*_source.changes
rm -f miracle-wm_${version}-*_source.build
rm -rf miracle-wm-$version

echo "Done!"
