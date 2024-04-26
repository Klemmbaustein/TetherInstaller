#pragma once
#include <atomic>

namespace AppUpdate
{
	void Check();
	void Update();
	
	void CheckForUpdates();
	void CheckForInstallerUpdate();
	void UpdateInstaller();

	extern std::atomic<bool> RequiresUpdate;
}