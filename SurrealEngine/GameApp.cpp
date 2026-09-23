
#include "Precomp.h"
#include "Utils/Exception.h"
#include "Utils/Logger.h"
#include "Utils/CommandLine.h"
#include "GameApp.h"
#include "GameFolder.h"
#include "Engine.h"
#include "UI/WidgetResourceData.h"
#include "UI/ErrorWindow/ErrorWindow.h"
#include "UI/Launcher/LauncherWindow.h"
#include "Utils/File.h"
#include <stdexcept>
#include <surrealwidgets/core/theme.h>
#include <surrealwidgets/window/window.h>
#include <iostream>
#include <fstream>

int GameApp::main(Array<std::string> args)
{
	auto backend = DisplayBackend::TryCreateBackend();
	DisplayBackend::Set(std::move(backend));
	InitWidgetResources();
	WidgetTheme::SetTheme(std::make_unique<DarkWidgetTheme>());

	try
	{
		CommandLine cmd(args);
		commandline = &cmd;

		if (ErrorWindow::CheckCrashReporter())
			return 0;

		if (commandline->HasArg("-h", "--help"))
		{
			std::cout << "SurrealEngine [--url=<mapname>] [--engineversion=X] [--play] [--logfile=<path>] [Path to game folder]\n";
			std::cout << "  --play           skip the launcher and start the first detected game (use with a game folder path)\n";
			std::cout << "  --logfile=<path> stream the engine log to a plain text file\n";
			return 0;
		}

		// --logfile: stream every log line to disk as it is produced, so a hard crash still leaves a usable log.
		std::ofstream logFile;
		std::string logFileName = commandline->GetArg("-l", "--logfile");
		if (!logFileName.empty())
		{
			logFile.open(logFileName, std::ios::out | std::ios::trunc);
			Logger::Get()->SetCallback([&logFile](const LogMessageLine& line)
			{
				if (!line.Source.empty())
					logFile << "[" << line.Source << "] ";
				logFile << line.Text << std::endl;
			});
		}

		int selectedGameIndex = -1;
		if (commandline->HasArg("-p", "--play"))
		{
			// Unattended launch (mirrors MainAndroid): use the folder(s) from the command line, no launcher window.
			GameFolderSelection::UpdateList();
			if (GameFolderSelection::Games.empty())
				throw std::runtime_error("--play: no supported UE1 game found in the given folder");
			selectedGameIndex = 0;
		}
		else
		{
			selectedGameIndex = LauncherWindow::ExecModal();
		}
		if (selectedGameIndex >= 0)
		{
			GameLaunchInfo info = GameFolderSelection::GetLaunchInfo(selectedGameIndex);
			Engine engine(info);
			engine.Run();
		}
	}
	catch (const std::exception& e)
	{
		ErrorWindow::ExecModal(e.what(), Logger::Get()->GetLog());
	}

	DeinitWidgetResources();
	return 0;
}
