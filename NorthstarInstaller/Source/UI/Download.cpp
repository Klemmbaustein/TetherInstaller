#include "Download.h"
#include <KlemmUI/UI/UIBackground.h>
#include <KlemmUI/UI/UIButton.h>
#include <KlemmUI/UI/UIText.h>
#include <KlemmUI/UI/UIScrollBox.h>

#include "../UI/UIDef.h"
#include "../Installer.h"
#include "../BackgroundTask.h"
#include "../Translation.h"
using namespace Translation;
using namespace KlemmUI;

namespace DownloadWindow
{
	bool IsDownloading = false;
	UIBackground* DownloadBackground = nullptr;
	UIScrollBox* DownloadsBox = nullptr;
	UIText* DownloadFinishedText = nullptr;
	UIText* TitleText = nullptr;
	size_t AllTasksSize = 0;

	BackgroundTask* LatestBackgroundTask = nullptr;

	void Generate();

	struct DownloadInfo
	{
		BackgroundTask* Task = nullptr;
		UIBackground* ProgressBar = nullptr;
		UIText* NameText = nullptr;
	};

	std::vector<DownloadInfo> CurrentDownloads;

	float DissapearTimer = 0;
}

void DownloadWindow::Generate()
{
	DownloadsBox->DeleteChildren();
	DownloadFinishedText = nullptr;
	CurrentDownloads.clear();
	DownloadBackground->MoveToFront();
	DownloadsBox->GetScrollBarBackground()->MoveToFront();
	IsDownloading = false;
	for (BackgroundTask* i : BackgroundTask::AllTasks)
	{
		if (i->Status.size() >= 3 && i->Status.substr(0, 3) == "dl_")
		{
			DownloadInfo NewDownload;
			NewDownload.Task = i;
			NewDownload.NameText = new UIText(0.6f, 0, "", UI::Text);
			NewDownload.ProgressBar = new UIBackground(true, 0, Vector3f(0.3, 0.5, 1), Vector2f(i->Progress / 2.5, 0.05));
			DownloadsBox->AddChild((new UIBackground(true, 0, 1, Vector2f(1.0 / 2.5, 0)))
				->AddChild(NewDownload.ProgressBar
					->SetVerticalAlign(UIBox::Align::Centered)
					->SetPadding(0)
					->AddChild(NewDownload.NameText
						->SetPadding(0, 0, 0.01, 0))));
			CurrentDownloads.push_back(NewDownload);
			IsDownloading = true;
		}
	}

	if (!IsDownloading)
	{
		DownloadFinishedText = new UIText(0.8f, 1, GetTranslation("download_no_downloads"), UI::Text);
		DownloadsBox->AddChild(DownloadFinishedText);
	}

	Installer::GenerateWindowButtons();
}

void DownloadWindow::Update(float WindowBarSize)
{
	if (!DownloadBackground)
	{
		DownloadsBox = new UIScrollBox(false, 0, true);
		DownloadBackground = new UIBackground(false, 0, 0.1f, Vector2f(0.5, 0.25));
		DownloadBackground->HasMouseCollision = true;
		TitleText = new UIText(1.0f, 1, GetTranslation("download_window_title"), UI::Text);
		DownloadBackground
			->SetBorder(UIBox::BorderType::DarkenedEdge, 0.3)
			->AddChild((new UIBackground(true, 0, 1, Vector2f(0.5, 0.005)))
				->SetPadding(0))
			->AddChild(TitleText)
			->AddChild(DownloadsBox
				->SetPadding(0)
				->SetMinSize(Vector2f(0.495, 0.15)));
		SetWindowVisible(false);
	}

	

	if (AllTasksSize != BackgroundTask::AllTasks.size() || (!BackgroundTask::AllTasks.empty() && BackgroundTask::AllTasks[0] != LatestBackgroundTask))
	{
		if (!BackgroundTask::AllTasks.empty())
		{
			LatestBackgroundTask = BackgroundTask::AllTasks[0];
		}
		AllTasksSize = BackgroundTask::AllTasks.size();
		Generate();
		if (IsDownloading)
		{
			SetWindowVisible(true);
		}
	}

	for (auto& i : CurrentDownloads)
	{
		Vector2f Size = Vector2f(i.Task->Progress / 3, 0.05);
		i.ProgressBar->SetMinSize(Size);
		i.ProgressBar->SetMaxSize(Size);
		if (i.Task->Status.size() > 3)
		{
			std::string DisplayedName = i.Task->Status.substr(3);

			if (DisplayedName.size() > 40)
			{
				DisplayedName = DisplayedName.substr(0, 37).append("...");
			}

			i.NameText->SetText(DisplayedName);
		}
		DissapearTimer = 0;
	}
	DownloadBackground->SetPosition(Vector2f(0.5, 1 - WindowBarSize - 0.25));

	if (!DownloadBackground->IsVisible)
	{
		return;
	}

	if (DissapearTimer >= 1.5)
	{
		float Opacity = 1 - (DissapearTimer - 1.5f) * 2;
		DownloadBackground->SetOpacity(Opacity);
		DownloadFinishedText->SetOpacity(Opacity);
		TitleText->SetOpacity(Opacity);
	}
	else
	{
		DownloadBackground->SetOpacity(1);
		TitleText->SetOpacity(1);
	}

	if (DissapearTimer >= 2)
	{
		SetWindowVisible(false);
		return;
	}

	DissapearTimer += Installer::MainWindow->GetDeltaTime();

}

void DownloadWindow::SetWindowVisible(bool NewVisible)
{
	DownloadBackground->IsVisible = NewVisible;
}
