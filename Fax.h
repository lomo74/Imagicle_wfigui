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

#ifndef FaxH
#define FaxH
//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include <vector>
//---------------------------------------------------------------------------

class TRecipient
{
private:
	UnicodeString FFirstName;
	UnicodeString FLastName;
	UnicodeString FFaxNumber;
public:
	TRecipient(const UnicodeString &aFirstName,
		const UnicodeString &aLastName,
		const UnicodeString &aFaxNumber);
	TRecipient(const TRecipient &aSrc);
public:
	__property UnicodeString FirstName = { read = FFirstName };
	__property UnicodeString LastName = { read = FLastName };
	__property UnicodeString FaxNumber = { read = FFaxNumber };
};
//---------------------------------------------------------------------------

class TFax
{
private:
	std::vector<UnicodeString> FFiles;
	std::vector<TRecipient> FRecipients;
	UnicodeString FSubject, FBody, FNotificationEmailAddress, FCoverPageName;
	bool FNotifyByEmail, FUseCover;
public:
	TFax() : FNotifyByEmail(false), FUseCover(false) {}
	virtual ~TFax() {}
	size_t GetFileCount();
	const UnicodeString &GetFile(int nIndex);
	size_t GetRecipientCount();
	const TRecipient &GetRecipient(int nIndex);
	void AddFile(const UnicodeString &aFile);
	void AddRecipient(const UnicodeString &aFirstName,
		const UnicodeString &aLastName,
		const UnicodeString &aFaxNumber);
    UnicodeString GetRecipientsAsString();
public:
	__property size_t FileCount = { read = GetFileCount };
	__property const UnicodeString &Files[int nIndex] = { read = GetFile };
	__property size_t RecipientCount = { read = GetRecipientCount };
	__property const TRecipient &Recipients[int nIndex] = { read = GetRecipient };
	__property UnicodeString Subject = { read = FSubject, write = FSubject };
	__property UnicodeString Body = { read = FBody, write = FBody };
	__property bool NotifyByEmail = { read = FNotifyByEmail, write = FNotifyByEmail };
	__property bool UseCover = { read = FUseCover, write = FUseCover };
	__property UnicodeString NotificationEmailAddress = { read = FNotificationEmailAddress, write = FNotificationEmailAddress };
	__property UnicodeString CoverPageName = { read = FCoverPageName, write = FCoverPageName };
    __property UnicodeString RecipientsAsString = { read = GetRecipientsAsString };
};
//---------------------------------------------------------------------------
#endif
