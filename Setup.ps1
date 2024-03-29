
if (!(Test-Path -Path KlemmUI/Dependencies/SDL/include))
{
	echo "SDL directory seems to be empty or missing. Verify you have cloned the repo with submodules or run 'git submodule update --init --recursive'"
	exit
}

if (!(Test-Path -Path Dependencies\SDL\VisualC\SDL\x64\Release))
{
	cd KlemmUI/
	./Setup.ps1
	cd ..
}

cp KlemmUI\Dependencies\SDL\VisualC\SDL\x64\Release\SDL2.dll NorthstarInstaller

cd curl
cmake -DCURL_USE_SCHANNEL=on -DBUILD_SHARED_LIBS=off -DCMAKE_C_COMPILER=cl -B Build

cd Build
msbuild.exe lib/libcurl_static.vcxproj /p:Configuration=Release /p:Platform=x64

cd ../..