mkdir Release -force

if (Test-Path -Path "Release/Data")
{
	rm -r -fo "Release/Data"
}

cp NorthstarInstaller/Data Release/Data -Recurse -force
cp NorthstarInstaller/update.bat Release/update.bat -Recurse -force
cp KlemmUI\Dependencies\SDL\VisualC\SDL\x64\Release\SDL2.dll Release\SDL2.dll
cp License.txt Release\License.txt