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

#pragma hdrstop

#include "Fax.h"

#pragma package(smart_init)
//---------------------------------------------------------------------------

TRecipient::TRecipient(const UnicodeString &aFirstName,
	const UnicodeString &aLastName,
	const UnicodeString &aFaxNumber)
{
	FFirstName = aFirstName;
	FLastName = aLastName;
	FFaxNumber = aFaxNumber;
}
//---------------------------------------------------------------------------

TRecipient::TRecipient(const TRecipient &aSrc)
{
 	FFirstName = aSrc.FirstName;
 	FLastName = aSrc.LastName;
 	FFaxNumber = aSrc.FaxNumber;
}
//---------------------------------------------------------------------------

size_t TFax::GetFileCount()
{
	return FFiles.size();
}
//---------------------------------------------------------------------------

const UnicodeString &TFax::GetFile(int nIndex)
{
	if (nIndex < 0 || nIndex >= FFiles.size())
		throw EListError(L"Index out of bounds");

	return FFiles[nIndex];
}
//---------------------------------------------------------------------------

size_t TFax::GetRecipientCount()
{
	return FRecipients.size();
}
//---------------------------------------------------------------------------

const TRecipient &TFax::GetRecipient(int nIndex)
{
	if (nIndex < 0 || nIndex >= FRecipients.size())
		throw EListError(L"Index out of bounds");

	return FRecipients[nIndex];
}
//---------------------------------------------------------------------------

void TFax::AddFile(const UnicodeString &aFile)
{
	FFiles.push_back(aFile);
}
//---------------------------------------------------------------------------

void TFax::AddRecipient(const UnicodeString &aFirstName,
	const UnicodeString &aLastName,
	const UnicodeString &aFaxNumber)
{
	FRecipients.push_back(TRecipient(aFirstName, aLastName, aFaxNumber));
}
//---------------------------------------------------------------------------

UnicodeString TFax::GetRecipientsAsString()
{
	std::vector<TRecipient>::const_iterator it;
	UnicodeString rec, name;
	TStringBuilder *pBuilder = new TStringBuilder();

	try {
		for (it = FRecipients.begin(); it != FRecipients.end(); it++) {
			if (it->FirstName.Length() > 0 || it->LastName.Length() > 0) {
				TVarRec vars1[] = {
					it->FirstName,
					it->LastName,
				};
				name = Format(L"%s %s", vars1, 2).Trim();

				TVarRec vars2[] = {
					name,
					it->FaxNumber,
				};
				rec = Format(L"%s (%s);", vars2, 2);
			} else {
				rec = it->FaxNumber + L";";
			}

            pBuilder->Append(rec);
		}

		return pBuilder->ToString();
	}
	__finally {
		delete pBuilder;
    }
}
//---------------------------------------------------------------------------

