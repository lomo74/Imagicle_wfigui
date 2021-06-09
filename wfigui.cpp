/*
Imagicle print2fax
Copyright (C) 2021 Lorenzo Monti

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

//---------------------------------------------------------------------------

#include <vcl.h>
#include <gnugettext.hpp>
#include <Quick.Logger.hpp>
#include <Quick.Logger.Provider.Files.hpp>
#include <Quick.Logger.Provider.IDEDebug.hpp>
#include <tchar.h>
#include "IdHTTP.hpp"
#pragma hdrstop

#include "Config.h"
//---------------------------------------------------------------------------
USEFORM("frmRecipient.cpp", RecipientName);
USEFORM("frmSelect.cpp", SelectRcpt);
USEFORM("frmConfirm.cpp", Confirmation);
USEFORM("frmMain.cpp", MainForm);
USEFORM("frmProgress.cpp", FormProgress);
USEFORM("frmConfig.cpp", ConfigForm);
//---------------------------------------------------------------------------
#pragma link "gnugettext.lib"
#pragma link "odbc32.lib"
#pragma link "icuuc.lib"
#pragma link "quick.lib"
//---------------------------------------------------------------------------
int WINAPI _tWinMain(HINSTANCE, HINSTANCE, LPTSTR, int)
{
	IsMultiThread = true;

	#ifdef MAPIDEBUG
	ShowMessage(L"WARNING: this is a special build only intended for debugging of Extended MAPI code.");
	#endif

	try
	{
		srand((unsigned)time(0));

		TConfig *Config = TConfig::GetInstance();

		_di_ILogProvider intf;

		GlobalLogFileProvider->GetInterface(intf);
		Quick::Logger::Logger->Providers->Add(intf);

		GlobalLogIDEDebugProvider->GetInterface(intf);
		Quick::Logger::Logger->Providers->Add(intf);

		GlobalLogFileProvider->LogLevel = LOG_ONLYERRORS;
		GlobalLogFileProvider->FileName = Config->WFIUserDir + L"wfigui.log";
		GlobalLogFileProvider->MaxRotateFiles = 10;
		GlobalLogFileProvider->MaxFileSizeInMB = 10;
		GlobalLogFileProvider->RotatedFilesPath = Config->WFIUserDir + L"logs_archive";
		GlobalLogFileProvider->TimePrecission = true;
		GlobalLogFileProvider->Enabled = false;

		GlobalLogIDEDebugProvider->LogLevel = LOG_ALL;
		GlobalLogIDEDebugProvider->Enabled = true;

		TP_GlobalIgnoreClass(__classid(TFont));

		TP_GlobalIgnoreClassProperty(__classid(TAction), L"Category");
		TP_GlobalIgnoreClassProperty(__classid(TControl), L"HelpKeyword");
		TP_GlobalIgnoreClassProperty(__classid(TControl), L"ImeName");
		TP_GlobalIgnoreClassProperty(__classid(TDateTimePicker), L"Format");
		TP_GlobalIgnoreClassProperty(__classid(TIdHTTPRequest), L"Accept");
		TP_GlobalIgnoreClassProperty(__classid(TIdHTTPRequest), L"UserAgent");

		UnicodeString path = ExtractFilePath(ExtractFileDir(Application->ExeName)) + L"locale";

		bindtextdomain(L"wfigui", path);
		bindtextdomain(L"delphi", path);
		//Delphi strings translation
		AddDomainForResourceString(L"delphi");
		//default domain for text translations
		textdomain(L"wfigui");

		Application->Initialize();
		Application->MainFormOnTaskBar = true;
		Application->Title = "Imagicle print2fax";
		Application->CreateForm(__classid(TMainForm), &MainForm);
		Application->Run();
	}
	catch (Exception &exception)
	{
		Application->ShowException(&exception);
	}
	catch (...)
	{
		try
		{
			throw Exception("");
		}
		catch (Exception &exception)
		{
			Application->ShowException(&exception);
		}
	}
	return 0;
}
//---------------------------------------------------------------------------
