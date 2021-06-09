/*
Imagicle print2fax
Copyright (C) 2021 Lorenzo Monti

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 3
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
#include <shlobj.h>
#include <gnugettext.hpp>
#pragma hdrstop

#include "IdHTTP.hpp"
#include "frmConfig.h"
#include "Config.h"
#include "Utils.h"
#include "frmMain.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)

#pragma link "JvExStdCtrls"
#pragma link "JvHtControls"
#pragma resource "*.dfm"

TConfigForm *ConfigForm;
//---------------------------------------------------------------------------

__fastcall TConfigForm::TConfigForm(TComponent *Owner)
	: TForm(Owner)
{
}
//---------------------------------------------------------------------------

__fastcall TConfigForm::~TConfigForm()
{
}
//---------------------------------------------------------------------------

void __fastcall TConfigForm::btnBrowseClick(TObject *Sender)
{
	const size_t nChars = MAX_PATH + 1;
	WCHAR buf[nChars];
	BROWSEINFOW bi;
	PIDLIST_ABSOLUTE pidl;

	wcscpy_s(buf, nChars, edtAddressBook->Text.c_str());

	bi.hwndOwner = this->Handle;
	bi.pidlRoot = NULL;
	bi.pszDisplayName = buf;
	bi.lpszTitle = _(L"Choose address book location").c_str();
	bi.ulFlags = BIF_EDITBOX;
	bi.lpfn = NULL;
	bi.lParam = NULL;
	bi.iImage = 0;

	CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
	if ((pidl = SHBrowseForFolderW(&bi)) != NULL) {
		if (SHGetPathFromIDListW(pidl, buf))
			edtAddressBook->Text = buf;
		IMalloc *pMalloc = NULL;
		SHGetMalloc(&pMalloc);
		pMalloc->Free(pidl);
		pMalloc->Release();
	}
	CoUninitialize();
}
//---------------------------------------------------------------------------

void __fastcall TConfigForm::btnDefaultClick(TObject *Sender)
{
	TConfig *Config = TConfig::GetInstance();

	edtAddressBook->Text = Config->DefaultAddrBookPath;
}
//---------------------------------------------------------------------------

void __fastcall TConfigForm::FormCloseQuery(TObject *Sender, bool &CanClose)
{
	UnicodeString Path = Trim(edtAddressBook->Text);
	if (Path.Length() > 0 &&
	!StartsWith(Path, L"http://") &&
	!StartsWith(Path, L"https://") &&
	!Sysutils::DirectoryExists(Path)) {
		MessageDlg(_(L"Specify a valid path for the Address Book"),
			mtError, TMsgDlgButtons() << mbOK, 0);
		CanClose = false;
	}
}
//---------------------------------------------------------------------------

void __fastcall TConfigForm::FormConstrainedResize(TObject *Sender, int &MinWidth,
		int &MinHeight, int &MaxWidth, int &MaxHeight)
{
	MinHeight = 580;
	MinWidth = 466;
}
//---------------------------------------------------------------------------

void __fastcall TConfigForm::FormCreate(TObject *Sender)
{
	TP_Ignore(this, L"lblVersion.Caption"); // version label excluded
	TranslateComponent(this, L"wfigui");

	lblVersion->Caption = _("rel. ") + GetVersionDescription(NULL);

	TStringList *langs = new TStringList();
	try {
		DefaultInstance->GetListOfLanguages(L"wfigui", langs);
		langs->Insert(0, L"en");

		cbLanguage->Items->Add(L"<auto>");

		for (int i = 0; i < langs->Count; i++)
		{
			UErrorCode status = U_ZERO_ERROR;
			AnsiString lang = langs->Strings[i];
			int32_t len = uloc_getDisplayLanguage(lang.c_str(), lang.c_str(),
				NULL, 0, &status) + 1;
			if (status == U_BUFFER_OVERFLOW_ERROR) {
				wchar_t *buf = new wchar_t[len];
				try {
					status = U_ZERO_ERROR;
					uloc_getDisplayLanguage(lang.c_str(), lang.c_str(),
						reinterpret_cast<UChar *>(buf), len, &status);
					if (status == U_ZERO_ERROR)
						cbLanguage->Items->Add(buf);
				}
				__finally {
					delete[] buf;
				}
			}
		}
	}
	__finally {
		delete langs;
	}

	//DefaultInstance->TranslateProperties(hLanguage, L"languages");
}
//---------------------------------------------------------------------------

void __fastcall TConfigForm::edtMAPIProfileChange(TObject *Sender)
{
	rbMAPIUseProfile->Checked = true;
}
//---------------------------------------------------------------------------

void __fastcall TConfigForm::chkODBCAuthClick(TObject *Sender)
{
	edtODBCUser->ReadOnly = !chkODBCAuth->Checked;
	edtODBCPassword->ReadOnly = !chkODBCAuth->Checked;
}
//---------------------------------------------------------------------------

void __fastcall TConfigForm::chkUseSSLClick(TObject *Sender)
{
	if (chkUseSSL->Checked) {
		chkSkipCertificateCheck->Enabled = true;
	} else {
		chkSkipCertificateCheck->Enabled = false;
		chkSkipCertificateCheck->Checked = false;
	}
}
//---------------------------------------------------------------------------

