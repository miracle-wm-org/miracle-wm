if (( $# < 2 )); then
    echo "Usage: ./publish-ppa.sh <NEW_VERSION> <DISTRO>"
    exit 1
fi


version=$1
distro=$2
dir=$(dirname $0)
cd $dir/..
rm -rf build
debuild -S -sd
cd $dir/../..
dput ppa:matthew-kosarek/miracle-wm miracle-wm_${version}-${distro}_source.changes