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
#include <IniFiles.hpp>
#include <gnugettext.hpp>
#pragma hdrstop

#include "Config.h"
#include "ConfigSerializerIni.h"
#include "frmConfig.h"

#pragma package(smart_init)

//---------------------------------------------------------------------------
TConfig *TConfig::FConfig = NULL;
//---------------------------------------------------------------------------

TConfig::TConfig()
	:
    TObject(),
	FLanguageChanged(false),
	FAddrBookTypeChanged(false),
	FAddrBookLocationChanged(false),
    FLogLevelChanged(false),
	FAddrBookType(abNone),
	FODBCAuth(false),
	FNotify(false),
	FUseSSL(true),
	FSkipCertificateCheck(false),
    FLogLevel(LOGLEVEL_NONE)
{
	WCHAR buf[MAX_PATH + 1] = { 0 };

	SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, buf);
	FWFIUserDir = Sysutils::IncludeTrailingPathDelimiter(buf) + L"Imagicle print2fax\\";
	Sysutils::ForceDirectories(FWFIUserDir);

	SHGetFolderPathW(NULL, CSIDL_COMMON_APPDATA, NULL, SHGFP_TYPE_CURRENT, buf);
	FWFICommonDir = Sysutils::IncludeTrailingPathDelimiter(buf) + L"Imagicle print2fax\\";
	FWFITempDir = FWFICommonDir + L"faxtmp\\";

	Sysutils::ForceDirectories(FWFICommonDir);
	Sysutils::ForceDirectories(FWFITempDir);
}
//---------------------------------------------------------------------------

TConfig::~TConfig()
{
}
//---------------------------------------------------------------------------

TConfig *TConfig::GetInstance()
{
	if (FConfig == NULL) {
		FConfig = new TConfig();
	}

	return FConfig;
}
//---------------------------------------------------------------------------

void TConfig::Load()
{
	IConfigSerializer *Serializer = new TConfigSerializerIni(this);
	try {
		Serializer->Load();
	}
	__finally {
		delete Serializer;
    }

	FireEvents();
}
//---------------------------------------------------------------------------

bool TConfig::Configure()
{
	TStringList *langs = new TStringList();
	TConfigForm *ConfForm = new TConfigForm(NULL);
	bool bRet = false;

	try {
		DefaultInstance->GetListOfLanguages(L"wfigui", langs);
		langs->Insert(0, L"en");

		ConfForm->edtServer->Text = Server;
		ConfForm->chkUseSSL->Checked = UseSSL;
		ConfForm->chkSkipCertificateCheck->Checked = SkipCertificateCheck;
		ConfForm->chkSkipCertificateCheck->Enabled = UseSSL;
		ConfForm->edtUsername->Text = Username;
		ConfForm->edtPassword->Text = Password;
		ConfForm->edtNotificationEmail->Text = NotificationEmail;
		ConfForm->chkNotify->Checked = Notify;
		if (Language.Length() == 0) {
			ConfForm->cbLanguage->ItemIndex = 0;
		} else {
			int idx = langs->IndexOf(Language);
			if (idx >= 0)
				ConfForm->cbLanguage->ItemIndex = idx + 1;
		}
		ConfForm->cbAddrBookType->ItemIndex = (int)AddrBookType;
		ConfForm->edtAddressBook->Text = AddrBookPath;
		ConfForm->rbMAPIDefProfile->Checked = MAPIUseDefProfile;
		ConfForm->rbMAPIUseProfile->Checked = !MAPIUseDefProfile;
		ConfForm->edtMAPIProfile->Text = MAPIProfile;
		ConfForm->edtODBCDSN->Text = ODBCDSN;
		ConfForm->edtODBCUser->Text = ODBCUid;
		ConfForm->edtODBCPassword->Text = ODBCPwd;
		ConfForm->edtODBCTable->Text = ODBCTable;
		ConfForm->edtODBCName->Text = ODBCNameField;
		ConfForm->edtODBCFax->Text = ODBCFaxField;
		ConfForm->chkODBCAuth->Checked = ODBCAuth;
		ConfForm->edtODBCUser->ReadOnly = !ODBCAuth;
		ConfForm->edtODBCPassword->ReadOnly = !ODBCAuth;

		ConfForm->cbLogLevel->ItemIndex = LogLevel;

		if (ConfForm->ShowModal() == mrOk) {
			try {
				Server = ConfForm->edtServer->Text.Trim();
				UseSSL = ConfForm->chkUseSSL->Checked;
				SkipCertificateCheck = ConfForm->chkSkipCertificateCheck->Checked;
				Username = ConfForm->edtUsername->Text;
				Password = ConfForm->edtPassword->Text;
				NotificationEmail = ConfForm->edtNotificationEmail->Text.Trim();
				Notify = ConfForm->chkNotify->Checked;
				AddrBookType = static_cast<TWFIAddressBookType>(ConfForm->cbAddrBookType->ItemIndex);
				AddrBookPath = ConfForm->edtAddressBook->Text.Trim();
				if (ConfForm->cbLanguage->ItemIndex > 0)
					Language = langs->Strings[ConfForm->cbLanguage->ItemIndex - 1];
				else
					Language = L"";
				MAPIUseDefProfile = ConfForm->rbMAPIDefProfile->Checked;
				MAPIProfile = ConfForm->edtMAPIProfile->Text;
				ODBCDSN = ConfForm->edtODBCDSN->Text;
				ODBCAuth = ConfForm->chkODBCAuth->Checked;
				ODBCUid = ConfForm->edtODBCUser->Text;
				ODBCPwd = ConfForm->edtODBCPassword->Text;
				ODBCTable = ConfForm->edtODBCTable->Text;
				ODBCNameField = ConfForm->edtODBCName->Text;
				ODBCFaxField = ConfForm->edtODBCFax->Text;

                LogLevel = ConfForm->cbLogLevel->ItemIndex;

				FireEvents();
			}
			__finally {
				bRet = true;

				Save();
			}
		}
	}
	__finally {
		delete ConfForm;
		delete langs;
	}

	return bRet;
}
//---------------------------------------------------------------------------

void TConfig::Save()
{
	IConfigSerializer *Serializer = new TConfigSerializerIni(this);
	try {
		Serializer->Save();
	}
	__finally {
		delete Serializer;
	}
}
//---------------------------------------------------------------------------

void TConfig::SetAddrBookType(TWFIAddressBookType Value)
{
	if (Value != FAddrBookType) {
		FAddrBookType = Value;

		FAddrBookTypeChanged = true;
	}
}
//---------------------------------------------------------------------------

void TConfig::SetAddrBookPath(const UnicodeString &Value)
{
	if (Value.CompareIC(FAddrBookPath) != 0) {
		FAddrBookPath = Value;

		if (FAddrBookType == abCSV)
			FAddrBookLocationChanged = true;
	}
}
//---------------------------------------------------------------------------

void TConfig::SetLanguage(const UnicodeString &Value)
{
	if (Value.CompareIC(FLanguage) != 0) {
		FLanguage = Value;

		DefaultInstance->UseLanguage(FLanguage);

		FLanguageChanged = true;
	}
}
//---------------------------------------------------------------------------

void TConfig::SetMAPIUseDefProfile(bool Value)
{
	if (Value != FMAPIUseDefProfile) {
		FMAPIUseDefProfile = Value;

		if (FAddrBookType == abMAPI)
			FAddrBookLocationChanged = true;
	}
}
//---------------------------------------------------------------------------

void TConfig::SetMAPIProfile(const UnicodeString &Value)
{
	if (Value.CompareIC(FMAPIProfile) != 0) {
		FMAPIProfile = Value;

		if (FAddrBookType == abMAPI)
			FAddrBookLocationChanged = true;
	}
}
//---------------------------------------------------------------------------

void TConfig::SetODBCDSN(const UnicodeString &Value)
{
	if (Value.CompareIC(FODBCDSN) != 0) {
		FODBCDSN = Value;

		if (FAddrBookType == abODBC)
			FAddrBookLocationChanged = true;
	}
}
//---------------------------------------------------------------------------

void TConfig::SetODBCTable(const UnicodeString &Value)
{
	if (Value.CompareIC(FODBCTable) != 0) {
		FODBCTable = Value;

		if (FAddrBookType == abODBC)
			FAddrBookLocationChanged = true;
	}
}
//---------------------------------------------------------------------------

void TConfig::SetODBCNameField(const UnicodeString &Value)
{
	if (Value.CompareIC(FODBCNameField) != 0) {
		FODBCNameField = Value;

		if (FAddrBookType == abODBC)
			FAddrBookLocationChanged = true;
	}
}
//---------------------------------------------------------------------------

void TConfig::SetODBCFaxField(const UnicodeString &Value)
{
	if (Value.CompareIC(FODBCFaxField) != 0) {
		FODBCFaxField = Value;

		if (FAddrBookType == abODBC)
			FAddrBookLocationChanged = true;
	}
}
//---------------------------------------------------------------------------

void TConfig::SetODBCAuth(bool Value)
{
	if (Value != FODBCAuth) {
		FODBCAuth = Value;

		if (FAddrBookType == abODBC)
			FAddrBookLocationChanged = true;
	}
}
//---------------------------------------------------------------------------

void TConfig::SetODBCUid(const UnicodeString &Value)
{
	if (Value.Compare(FODBCUid) != 0) {
		FODBCUid = Value;

		if (FAddrBookType == abODBC)
			FAddrBookLocationChanged = true;
	}
}
//---------------------------------------------------------------------------

void TConfig::SetODBCPwd(const UnicodeString &Value)
{
	if (Value.Compare(FODBCPwd) != 0) {
		FODBCPwd = Value;

		if (FAddrBookType == abODBC)
			FAddrBookLocationChanged = true;
	}
}
//---------------------------------------------------------------------------

void TConfig::SetLogLevel(int Value)
{
	if (Value != FLogLevel) {
		FLogLevel = Value;

		FLogLevelChanged = true;
    }
}
//---------------------------------------------------------------------------

void TConfig::FireEvents()
{
	if (FLogLevelChanged) {
		if (FOnLogLevelChanged)
			FOnLogLevelChanged(this, FLogLevel);
	}

	if (FLanguageChanged) {
		if (FOnLanguageChanged)
			FOnLanguageChanged(this);

		FLanguageChanged = false;
	}

	if (FAddrBookTypeChanged) {
		if (FOnAddrBookTypeChanged)
			FOnAddrBookTypeChanged(this, FAddrBookType);

		FAddrBookTypeChanged = false;
	} else if (FAddrBookLocationChanged) {
		if (FOnAddrBookLocationChanged && FAddrBookType != abNone)
			FOnAddrBookLocationChanged(this);

		FAddrBookLocationChanged = false;
	}
}
//---------------------------------------------------------------------------

