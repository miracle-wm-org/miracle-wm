#!/bin/bash
# Release automation script for miracle-wm
set -e

if (( $# < 1 )); then
    echo "Usage: ./prepare-release.sh <VERSION> [DISTROS...]"
    echo "Example: ./prepare-release.sh 0.8.1 noble mantic jammy"
    echo ""
    echo "This script will:"
    echo "  1. Create and push a git tag"
    echo "  2. Update debian/changelog with recent commits"
    echo "  3. Publish to PPA"
    exit 1
fi

version=$1
shift
distros=("$@")

# Default to current LTS if no distros specified
if [ ${#distros[@]} -eq 0 ]; then
    distros=("noble")
fi

echo "=== Preparing release $version for: ${distros[*]} ==="

cd $(dirname $0)/..

# Check if we're in a clean git state
if ! git diff-index --quiet HEAD --; then
    echo "Error: You have uncommitted changes. Please commit or stash them first."
    exit 1
fi

# Check if tag already exists
if git rev-parse "v$version" >/dev/null 2>&1; then
    echo "Warning: Tag v$version already exists."
    read -p "Do you want to continue anyway? [y/N] " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        exit 1
    fi
else
    # Create and push git tag
    echo "Creating git tag v$version..."
    git tag -a "v$version" -m "Release version $version"
    
    echo "Pushing tag to origin..."
    git push origin "v$version"
fi

# Update debian/changelog in the main repository
echo "Updating main debian/changelog..."
recent_commits=$(git log --oneline --no-merges v$(git describe --tags --abbrev=0 HEAD~1 2>/dev/null | sed 's/^v//' || echo "HEAD~10")..HEAD 2>/dev/null || git log --oneline --no-merges -5)

# Create a backup of current changelog
cp debian/changelog debian/changelog.backup

# Add new entry to changelog
DEBEMAIL="matthew@matthewkosarek.xyz" DEBFULLNAME="Matthew Kosarek" dch -v ${version}-1 "Release version $version"

# Add recent commits
echo "Adding recent changes to changelog..."
echo "$recent_commits" | head -5 | while read commit; do
    if [ -n "$commit" ]; then
        commit_msg=$(echo "$commit" | sed 's/^[a-f0-9]* //')
        DEBEMAIL="matthew@matthewkosarek.xyz" DEBFULLNAME="Matthew Kosarek" dch -a "* $commit_msg"
    fi
done

echo "Changelog updated. Review the changes:"
head -15 debian/changelog

read -p "Does the changelog look good? [Y/n] " -n 1 -r
echo
if [[ $REPLY =~ ^[Nn]$ ]]; then
    echo "Restoring original changelog..."
    mv debian/changelog.backup debian/changelog
    exit 1
fi

rm -f debian/changelog.backup

# Commit the changelog update
git add debian/changelog
git commit -m "Update changelog for version $version"
git push origin HEAD

# Now publish to PPA
echo "Publishing to PPA..."
./tools/publish-ppa.sh $version "${distros[@]}"

echo "=== Release $version completed! ==="
