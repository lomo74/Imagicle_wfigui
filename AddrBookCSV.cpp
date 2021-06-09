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

#include <windows.h>
#include <vcl.h>
#include <curl/curl.h>
#include <curl/easy.h>
#include <gnugettext.hpp>
#pragma hdrstop

#include "AddrBookCSV.h"
#include "Config.h"
#include "indybkport.hpp"
#include "Utils.h"
#include "HttpClient.h"
#include "log.h"

#pragma package(smart_init)
//---------------------------------------------------------------------------

__fastcall TAddressBookCSV::TAddressBookCSV()
	: TAddressBook()
{
}
//---------------------------------------------------------------------------

TFileStream *TAddressBookCSV::LockAddressBook(WORD wMode)
{
	TFileStream *Result = NULL;

	//try to acquire a lock on the address book file
	for (int ntry = 6; ntry > 0; ntry--) {
		try {
			Result = new TFileStream(FABFile, wMode);
			return Result;
		}
		catch (Exception &) {
			Sleep(500);
		}
	}

	throw EAddressBookException(_(L"Unable to lock addressbook.csv!"));
}
//---------------------------------------------------------------------------

static bool StartsWith(const UnicodeString &Str, const UnicodeString &SubStr)
{
	int len = SubStr.Length();

	if (Str.Length() < len) {
		return false;
	}

	return (Str.SubString(1, len).CompareIC(SubStr) == 0);
}
//---------------------------------------------------------------------------

void __fastcall TAddressBookCSV::Load()
{
	UnicodeString Path;
	TFileStream *File;
	TStringList *AddrBook = new TStringList();
	TStringList *Temp = new TStringList();
	int i;

    TConfig *Config = TConfig::GetInstance();

	try {
		Temp->QuoteChar = L'"';
		Temp->Delimiter = L',';

		LOG_DEBUG1(L"TAddressBookCSV::Load: loading address book from %s", Config->AddrBookPath);

		if (StartsWith(Config->AddrBookPath, L"http://") ||
		StartsWith(Config->AddrBookPath, L"https://")) {
			// Seems to be a HTTP download
			FIsReadOnly = true;

			CURL *curl = NULL;
			UnicodeString aContentType, resp;
			long aCode = 0;

			try {
				if ((curl = curl_easy_init()) == NULL)
					throw ECurlException(_(L"Call to curl_easy_init failed"));

				curl_easy_setopt(curl, CURLOPT_URL, AnsiString(Config->AddrBookPath).c_str());

				curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
				curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10);
				curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30);

				resp = GetResponse(curl, aContentType, aCode);

				if (aCode == 401) {
					throw EUnauthorizedError();
				}

				if (aCode != 200 && aCode != 201 && aCode == 202) {
					throw EClientError(aCode);
				}

				AddrBook->Text = resp;
			}
			__finally {
				if (curl) curl_easy_cleanup(curl);
			}
		} else {
			FIsReadOnly = false;

			if (Config->AddrBookPath.Length() == 0) {
				FABFile = L"";
				FIsOnLine = false;
				return;
			} else {
				Path = IncludeTrailingPathDelimiter(Config->AddrBookPath);
				FABFile = Path + L"addressbook.csv";
			}

			WORD wMode;
			if (FileExists(FABFile)) {
				wMode = fmOpenRead;
			} else {
				wMode = fmCreate;
			}

			File = LockAddressBook(wMode | fmShareDenyWrite);

			//read in data
			try {
				AddrBook->LoadFromStream(File, TEncoding::UTF8);
			}
			__finally {
				delete File;
			}
		}

		FNames->Clear();

		for (i = 0; i < AddrBook->Count; i++) {
			Temp->DelimitedText = AddrBook->Strings[i];
			if (Temp->Count >= 2) {
				Add(Temp->Strings[0], Temp->Strings[1]);
			}
		}

		FIsOnLine = true;
	}
	__finally {
		delete AddrBook;
		delete Temp;
	}

	//fire event
	if (FOnAddressBookChanged)
		FOnAddressBookChanged(this);
}
//---------------------------------------------------------------------------

void __fastcall TAddressBookCSV::SetRecipient(const UnicodeString &Name,
	const UnicodeString &Number)
{
	if (!FIsOnLine || FIsReadOnly)
		return;

	TFileStream *File;
	int i;

	if (!ForceDirectories(ExtractFilePath(FABFile)))
		throw EAddressBookException(_(L"Unable to create addressbook.csv!"));

	TStringList *AddrBook = new TStringList();
	TStringList *Temp = new TStringList();

	try {
		Temp->QuoteChar = L'"';
		Temp->Delimiter = L',';

		WORD wMode = FileExists(FABFile)
			? fmOpenReadWrite
			: fmCreate;

		File = LockAddressBook(wMode | fmShareExclusive);

		//read in data
		try {
			AddrBook->LoadFromStream(File, TEncoding::UTF8);

			FNames->Clear();

			for (i = 0; i < AddrBook->Count; i++) {
				Temp->DelimitedText = AddrBook->Strings[i];
				if (Temp->Count >= 2)
					FNames->AddObject(Trim(Temp->Strings[0]),
					new TFaxNumber(Trim(Temp->Strings[1])));
			}

			if ((i = FNames->IndexOf(Name)) < 0) {
				if (Number.Length() > 0)
					FNames->AddObject(Trim(Name),
					new TFaxNumber(Trim(Number)));
			} else {
				if (Number.Length() > 0) {
					bool ChangeNumber = false;

					if (FOnAddressBookDuplicate)
						FOnAddressBookDuplicate(this, Name, ChangeNumber);

					if (!ChangeNumber)
						throw EAddressBookUnchanged();

					static_cast<TFaxNumber *>(FNames->Objects[i])->Number = Trim(Number);
				} else {
					FNames->Delete(i);
				}
			}

			AddrBook->Clear();

			for (i = 0; i < FNames->Count; i++) {
				Temp->Clear();
				Temp->Add(FNames->Strings[i]);
				Temp->Add(static_cast<TFaxNumber *>(FNames->Objects[i])->Number);
				AddrBook->Add(Temp->DelimitedText);
			}

			File->Position = 0;
			File->Size = 0;
			AddrBook->SaveToStream(File, TEncoding::UTF8);
		}
		__finally {
			delete File;
		}
	}
	__finally {
		delete AddrBook;
		delete Temp;
	}

	if (FOnAddressBookChanged)
		FOnAddressBookChanged(this);
}
//---------------------------------------------------------------------------

