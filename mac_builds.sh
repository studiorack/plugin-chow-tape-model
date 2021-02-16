#!/bin/bash

# exit on failure
set -e

# clean up old builds
cd Plugin
rm -Rf build/
rm -Rf Bin/*Mac*

# set up build VST
VST_PATH=~/Developer/VST2_SDK/
AAX_PATH=~/Developer/AAX_SDK_2p3p2/
sed -i '' "56s~.*~juce_set_vst2_sdk_path(${VST_PATH})~" CMakeLists.txt
sed -i '' "57s~.*~juce_set_aax_sdk_path(${AAX_PATH})~" CMakeLists.txt
sed -i '' '64s/#//' CMakeLists.txt

# cmake new builds
TEAM_ID=$(more ~/Developer/mac_id)
cmake -Bbuild -GXcode -DCMAKE_XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY="Apple Distribution" \
    -DCMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM=$TEAM_ID \
    -DCMAKE_XCODE_ATTRIBUTE_CODE_SIGN_STYLE="Manual" \
    -D"CMAKE_OSX_ARCHITECTURES=arm64;x86_64"
cmake --build build --config Release -j12 | xcpretty

# copy builds to Bin
mkdir -p Bin/Mac
declare -a plugins=("CHOWTapeModel")
for plugin in "${plugins[@]}"; do
    cp -R build/${plugin}_artefacts/Release/Standalone/${plugin}.app Bin/Mac/${plugin}.app
    cp -R build/${plugin}_artefacts/Release/VST/${plugin}.vst Bin/Mac/${plugin}.vst
    cp -R build/${plugin}_artefacts/Release/VST3/${plugin}.vst3 Bin/Mac/${plugin}.vst3
    cp -R build/${plugin}_artefacts/Release/AU/${plugin}.component Bin/Mac/${plugin}.component
    cp -R build/${plugin}_artefacts/Release/AAX/${plugin}.aaxplugin Bin/Mac/${plugin}.aaxplugin
done

# reset CMakeLists.txt
git restore CMakeLists.txt

# zip builds
VERSION=$(cut -f 2 -d '=' <<< "$(grep 'CMAKE_PROJECT_VERSION:STATIC' build/CMakeCache.txt)")
(
    cd bin
    rm -f "CHOWTapeModel-Mac-${VERSION}.zip"
    zip -r "CHOWTapeModel-Mac-${VERSION}.zip" Mac
)
