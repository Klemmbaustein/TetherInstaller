#include "BackgroundTask.h"
#include <cassert>
#include "Log.h"

thread_local BackgroundTask* BackgroundTask::ThisThreadPtr = nullptr;
std::string BackgroundTask::CurrentTaskStatus;
float BackgroundTask::CurrentTaskProgress;
bool BackgroundTask::IsRunningTask;
std::vector<BackgroundTask*> BackgroundTask::AllTasks;

BackgroundTask::BackgroundTask(void (*Function)())
{
	// Using a pointer as an ID. Why not?
	Type = (size_t)Function;
	AllTasks.push_back(this);

	Thread = new std::thread(TaskRun, Function, this);

}

BackgroundTask::~BackgroundTask()
{
	Thread->join();
	delete Thread;
}

void BackgroundTask::SetProgress(float Progress)
{
	assert(ThisThreadPtr);
	ThisThreadPtr->Progress = Progress;
}

void BackgroundTask::SetStatus(std::string NewStatus)
{
	assert(ThisThreadPtr);
	ThisThreadPtr->Status = NewStatus;
}

void BackgroundTask::TaskRun(void (*Function)(), BackgroundTask* ThisTask)
{
	ThisThreadPtr = ThisTask;
	Function();
	ThisTask->Progress = 1;
}

void BackgroundTask::UpdateTaskStatus()
{
	IsRunningTask = false;
	for (size_t i = 0; i < AllTasks.size(); i++)
	{
		if (AllTasks[i]->Progress >= 1)
		{
			delete AllTasks[i];
			AllTasks.erase(AllTasks.begin() + i);
			break;
		}
		else if (!IsRunningTask)
		{
			CurrentTaskProgress = AllTasks[i]->Progress;
			CurrentTaskStatus = AllTasks[i]->Status;
			IsRunningTask = true;
		}
	}
}