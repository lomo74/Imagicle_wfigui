//---------------------------------------------------------------------------

#include <vcl.h>
#include <IniFiles.hpp>
#include <gnugettext.hpp>
#pragma hdrstop

#include "Config.h"
#include "Utils.h"
#include "ConfigSerializerIni.h"
#include "log.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
//---------------------------------------------------------------------------

static const UnicodeString Section = L"PARAMETERS";
static const UnicodeString ServerIdent = L"Server";
static const UnicodeString UseSSLIdent = L"UseSSL";
static const UnicodeString SkipCertificateCheckIdent = L"SkipCertificateCheck";
static const UnicodeString UsernameIdent = L"Username";
static const UnicodeString PasswordIdent = L"Password";
static const UnicodeString NotificationEmailIdent = L"NotificationEmail";
static const UnicodeString NotifyIdent = L"Notify";
static const UnicodeString AddrBookTypeIdent = L"AddrBookType";
static const UnicodeString AddrBookPathIdent = L"AddrBookPath";
static const UnicodeString MAPIUseDefProfileIdent = L"MAPIUseDefProfile";
static const UnicodeString MAPIProfileIdent = L"MAPIProfile";
static const UnicodeString LanguageIdent = L"Language";
static const UnicodeString ODBCDSNIdent = L"ODBCDSN";
static const UnicodeString ODBCTableIdent = L"ODBCTable";
static const UnicodeString ODBCNameFieldIdent = L"ODBCNameField";
static const UnicodeString ODBCFaxFieldIdent = L"ODBCFaxField";
static const UnicodeString ODBCAuthIdent = L"ODBCAuth";
static const UnicodeString ODBCUidIdent = L"ODBCUid";
static const UnicodeString ODBCPwdIdent = L"ODBCPwd";
static const UnicodeString LogLevelIdent = L"LogLevel";
//---------------------------------------------------------------------------

TConfigSerializerIni::TConfigSerializerIni(TConfig *Config)
	: FConfig(Config)
{
	FUserIniFile = Config->WFIUserDir + L"wfigui.ini";
	FCommonIniFile = Config->WFICommonDir + L"wfigui.ini";
}
//---------------------------------------------------------------------------

void TConfigSerializerIni::Load()
{
	LOG_INFO2(L"TConfigSerializerIni::Load: reading configuration. User file=%s common file=%s", FUserIniFile, FCommonIniFile);

	//first time install?
	if (!Sysutils::FileExists(FCommonIniFile) &&
	!Sysutils::FileExists(FUserIniFile) &&
	MessageDlg(_(L"Client is not configured. Would you like to configure it now?"),
	mtWarning, TMsgDlgButtons() << mbYes << mbNo, 0) == mrYes) {
		FConfig->Configure();
		return;
	}

	TIniFile *CommonIni = new TIniFile(FCommonIniFile);
	TIniFile *UserIni = new TIniFile(FUserIniFile);

	try {
		FConfig->Server = UserIni->ReadString(Section, ServerIdent, L"");
		if (FConfig->Server.Trim().Length() == 0)
			FConfig->Server = CommonIni->ReadString(Section, ServerIdent, L"");

		bool UseSSL = CommonIni->ReadBool(Section, UseSSLIdent, true);
		FConfig->UseSSL = UserIni->ReadBool(Section, UseSSLIdent, UseSSL);

		bool SkipCertificateCheck = CommonIni->ReadBool(Section, SkipCertificateCheckIdent, false);
		FConfig->SkipCertificateCheck = UserIni->ReadBool(Section, SkipCertificateCheckIdent, SkipCertificateCheck);

		FConfig->Username = UserIni->ReadString(Section, UsernameIdent, L"");
		//if (FConfig->Username.Trim().Length() == 0)
		//	FConfig->Username = CommonIni->ReadString(Section, UsernameIdent, L"");

		FConfig->Password = DecryptString(UserIni->ReadString(Section, PasswordIdent, L""));
		//if (FConfig->Password.Trim().Length() == 0)
		//	FConfig->Password = DecryptString(CommonIni->ReadString(Section, PasswordIdent, L""));

		FConfig->NotificationEmail = UserIni->ReadString(Section, NotificationEmailIdent, L"");
		FConfig->Notify = UserIni->ReadBool(Section, NotifyIdent, false);
		FConfig->AddrBookType = static_cast<TWFIAddressBookType>(UserIni->ReadInteger(Section, AddrBookTypeIdent, abCSV));
		FConfig->AddrBookPath = UserIni->ReadString(Section, AddrBookPathIdent, FConfig->DefaultAddrBookPath);
		FConfig->Language = UserIni->ReadString(Section, LanguageIdent, L"");
		FConfig->MAPIUseDefProfile = UserIni->ReadBool(Section, MAPIUseDefProfileIdent, true);
		FConfig->MAPIProfile = UserIni->ReadString(Section, MAPIProfileIdent, L"");
		FConfig->ODBCDSN = UserIni->ReadString(Section, ODBCDSNIdent, L"");
		FConfig->ODBCTable = UserIni->ReadString(Section, ODBCTableIdent, L"");
		FConfig->ODBCNameField = UserIni->ReadString(Section, ODBCNameFieldIdent, L"");
		FConfig->ODBCFaxField = UserIni->ReadString(Section, ODBCFaxFieldIdent, L"");
		FConfig->ODBCAuth = UserIni->ReadBool(Section, ODBCAuthIdent, false);
		FConfig->ODBCUid = UserIni->ReadString(Section, ODBCUidIdent, L"");
		FConfig->ODBCPwd = DecryptString(UserIni->ReadString(Section, ODBCPwdIdent, L""));

		FConfig->LogLevel = UserIni->ReadInteger(Section, LogLevelIdent, LOGLEVEL_NONE);
	}
	__finally {
		delete CommonIni;
		delete UserIni;
	}
}
//---------------------------------------------------------------------------

void TConfigSerializerIni::Save()
{
	LOG_INFO1(L"TConfigSerializerIni::Save: writing configuration. User file=%s", FUserIniFile);

	TIniFile *CommonIni = new TIniFile(FCommonIniFile);
	TIniFile *UserIni = new TIniFile(FUserIniFile);

	try {
		if (FConfig->Server.Length() == 0 || FConfig->Server.CompareIC(CommonIni->ReadString(Section, ServerIdent, L"")) == 0)
			UserIni->DeleteKey(Section, ServerIdent);
		else
			UserIni->WriteString(Section, ServerIdent, FConfig->Server);

		if (FConfig->UseSSL == CommonIni->ReadBool(Section, UseSSLIdent, true))
			UserIni->DeleteKey(Section, UseSSLIdent);
		else
			UserIni->WriteBool(Section, UseSSLIdent, FConfig->UseSSL);

		if (FConfig->SkipCertificateCheck == CommonIni->ReadBool(Section, SkipCertificateCheckIdent, false))
			UserIni->DeleteKey(Section, SkipCertificateCheckIdent);
		else
			UserIni->WriteBool(Section, SkipCertificateCheckIdent, FConfig->SkipCertificateCheck);

		//if (FConfig->Username.Length() == 0 || FConfig->Username.CompareIC(CommonIni->ReadString(Section, UsernameIdent, L"")) == 0)
		//	UserIni->DeleteKey(Section, UsernameIdent);
		//else
			UserIni->WriteString(Section, UsernameIdent, FConfig->Username);

		//if (FConfig->Password.Length() == 0 || FConfig->Password == CommonIni->ReadString(Section, PasswordIdent, L""))
		//	UserIni->DeleteKey(Section, PasswordIdent);
		//else
			UserIni->WriteString(Section, PasswordIdent, EncryptString(FConfig->Password));

		UserIni->WriteString(Section, NotificationEmailIdent, FConfig->NotificationEmail);
		UserIni->WriteBool(Section, NotifyIdent, FConfig->Notify);
		UserIni->WriteString(Section, AddrBookTypeIdent, FConfig->AddrBookType);
		UserIni->WriteString(Section, AddrBookPathIdent, FConfig->AddrBookPath);
		UserIni->WriteString(Section, LanguageIdent, FConfig->Language);
		UserIni->WriteBool(Section, MAPIUseDefProfileIdent, FConfig->MAPIUseDefProfile);
		UserIni->WriteString(Section, MAPIProfileIdent, FConfig->MAPIProfile);
		UserIni->WriteString(Section, ODBCDSNIdent, FConfig->ODBCDSN);
		UserIni->WriteString(Section, ODBCTableIdent, FConfig->ODBCTable);
		UserIni->WriteString(Section, ODBCNameFieldIdent, FConfig->ODBCNameField);
		UserIni->WriteString(Section, ODBCFaxFieldIdent, FConfig->ODBCFaxField);
		UserIni->WriteBool(Section, ODBCAuthIdent, FConfig->ODBCAuth);
		UserIni->WriteString(Section, ODBCUidIdent, FConfig->ODBCUid);
		UserIni->WriteString(Section, ODBCPwdIdent, EncryptString(FConfig->ODBCPwd));

        UserIni->WriteInteger(Section, LogLevelIdent, FConfig->LogLevel);
	}
	__finally {
		delete CommonIni;
		delete UserIni;
	}
}
//---------------------------------------------------------------------------

