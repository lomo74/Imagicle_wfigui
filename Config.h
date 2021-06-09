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

#ifndef ConfigH
#define ConfigH
//---------------------------------------------------------------------------

enum TWFIAddressBookType {
	abNone,
	abCSV,
	abMAPI,
	abODBC,
};
//---------------------------------------------------------------------------

#define LOGLEVEL_NONE 0
#define LOGLEVEL_ERRORS 1
#define LOGLEVEL_DEBUG 2
//---------------------------------------------------------------------------

typedef void __fastcall (__closure *TAddrBookTypeChangeEvent)(TObject *, TWFIAddressBookType);
typedef void __fastcall (__closure *TAddrBookLocationChangeEvent)(TObject *);
typedef void __fastcall (__closure *TLanguageChangeEvent)(TObject *);
typedef void __fastcall (__closure *TLogLevelChangeEvent)(TObject *, int);
//---------------------------------------------------------------------------

class TConfig : public TObject
{
private:
	static TConfig *FConfig;
	UnicodeString FWFIUserDir;
	UnicodeString FWFICommonDir;
	UnicodeString FWFITempDir;
	UnicodeString FServer;
	bool FUseSSL;
    bool FSkipCertificateCheck;
	UnicodeString FUsername;
	UnicodeString FPassword;
	UnicodeString FNotificationEmail;
	bool FNotify;
	UnicodeString FAddrBookPath;
	UnicodeString FLanguage;
	UnicodeString FODBCDSN;
	UnicodeString FODBCTable;
	UnicodeString FODBCNameField;
	UnicodeString FODBCFaxField;
	bool FODBCAuth;
	UnicodeString FODBCUid;
	UnicodeString FODBCPwd;
	int FLogLevel;
	TWFIAddressBookType FAddrBookType;
	bool FAddrBookLocationChanged, FAddrBookTypeChanged, FLanguageChanged, FLogLevelChanged;
	bool FMAPIUseDefProfile;
	UnicodeString FMAPIProfile;
	TAddrBookTypeChangeEvent FOnAddrBookTypeChanged;
	TAddrBookLocationChangeEvent FOnAddrBookLocationChanged;
	TLanguageChangeEvent FOnLanguageChanged;
	TLogLevelChangeEvent FOnLogLevelChanged;
	void SetAddrBookType(TWFIAddressBookType Value);
	void SetAddrBookPath(const UnicodeString &Value);
	void SetLanguage(const UnicodeString &Value);
	void SetMAPIUseDefProfile(bool Value);
	void SetMAPIProfile(const UnicodeString &Value);
	void SetODBCDSN(const UnicodeString &Value);
	void SetODBCTable(const UnicodeString &Value);
	void SetODBCNameField(const UnicodeString &Value);
	void SetODBCFaxField(const UnicodeString &Value);
	void SetODBCAuth(bool Value);
	void SetODBCUid(const UnicodeString &Value);
	void SetODBCPwd(const UnicodeString &Value);
    void SetLogLevel(int Value);
	void FireEvents();
    TConfig();
public:
	static TConfig *GetInstance();
	static void Destroy() { if (FConfig) { delete FConfig; FConfig = NULL; } }
	virtual __fastcall ~TConfig();
	bool Configure();
	void Load();
	void Save();
	__property UnicodeString WFIUserDir = { read = FWFIUserDir };
	__property UnicodeString WFICommonDir = { read = FWFICommonDir };
	__property UnicodeString WFITempDir = { read = FWFITempDir };
	__property UnicodeString Server = { read = FServer, write = FServer };
	__property bool UseSSL = { read = FUseSSL, write = FUseSSL };
    __property bool SkipCertificateCheck = { read = FSkipCertificateCheck, write = FSkipCertificateCheck };
	__property UnicodeString Username = { read = FUsername, write = FUsername };
	__property UnicodeString Password = { read = FPassword, write = FPassword };
	__property UnicodeString NotificationEmail = { read = FNotificationEmail, write = FNotificationEmail };
	__property bool Notify = { read = FNotify, write = FNotify };
	__property TWFIAddressBookType AddrBookType = { read = FAddrBookType, write = SetAddrBookType };
	__property UnicodeString AddrBookPath = { read = FAddrBookPath, write = SetAddrBookPath };
	__property UnicodeString DefaultAddrBookPath = { read = FWFIUserDir };
	__property UnicodeString Language = { read = FLanguage, write = SetLanguage };
	__property bool MAPIUseDefProfile = { read = FMAPIUseDefProfile, write = SetMAPIUseDefProfile };
	__property UnicodeString MAPIProfile = { read = FMAPIProfile, write = SetMAPIProfile };
	__property TAddrBookTypeChangeEvent OnAddrBookTypeChanged =
		{ read = FOnAddrBookTypeChanged, write = FOnAddrBookTypeChanged };
	__property TAddrBookLocationChangeEvent OnAddrBookLocationChanged =
		{ read = FOnAddrBookLocationChanged, write = FOnAddrBookLocationChanged };
	__property TLanguageChangeEvent OnLanguageChanged =
		{ read = FOnLanguageChanged, write = FOnLanguageChanged };
	__property TLogLevelChangeEvent OnLogLevelChanged =
		{ read = FOnLogLevelChanged, write = FOnLogLevelChanged };
	__property UnicodeString ODBCDSN = { read = FODBCDSN, write = SetODBCDSN };
	__property UnicodeString ODBCTable = { read = FODBCTable, write = SetODBCTable };
	__property UnicodeString ODBCNameField = { read = FODBCNameField, write = SetODBCNameField };
	__property UnicodeString ODBCFaxField = { read = FODBCFaxField, write = SetODBCFaxField };
	__property bool ODBCAuth = { read = FODBCAuth, write = SetODBCAuth };
	__property UnicodeString ODBCUid = { read = FODBCUid, write = SetODBCUid };
	__property UnicodeString ODBCPwd = { read = FODBCPwd, write = SetODBCPwd };
	__property int LogLevel = { read = FLogLevel, write = SetLogLevel };
};
//---------------------------------------------------------------------------
#endif
