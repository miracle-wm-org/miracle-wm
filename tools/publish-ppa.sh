#!/bin/bash
set -e  # Exit on any error

# Set email for changelog entries
export DEBEMAIL="matthew@matthewkosarek.xyz"
export DEBFULLNAME="Matthew Kosarek"

if (( $# < 1 )); then
    echo "Usage: ./publish-ppa.sh <NEW_VERSION> [DISTRO1] [DISTRO2] ..."
    echo "Example: ./publish-ppa.sh 0.8.1 noble mantic jammy"
    exit 1
fi

version=$1
shift
distros=("$@")

# Default to current LTS if no distros specified
if [ ${#distros[@]} -eq 0 ]; then
    distros=("noble")
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

# Update changelog for each distro
for distro in "${distros[@]}"; do
    echo "Building for $distro..."
    
    # Update changelog
    DEBEMAIL="matthew@matthewkosarek.xyz" DEBFULLNAME="Matthew Kosarek" dch -D $distro -v ${version}-${distro} "Update to version $version" --force-distribution
    
    # Clean and build
    rm -rf build
    
    echo "Building source package for $distro..."
    debuild -S -sa -d
    
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
