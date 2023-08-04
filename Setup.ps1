
if (!(Test-Path -Path KlemmUI/SDL/include))
{
	echo "SDL directory seems to be empty or missing. Verify you have cloned the repo with submodules or run 'git submodule update --init --recursive'"
	exit
}
cd KlemmUI/
./Setup.ps1
msbuild /p:Configuration=Release
cd ..