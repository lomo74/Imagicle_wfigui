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
#include <gnugettext.hpp>
#pragma hdrstop

#include "frmSelect.h"
#include "AddressBook.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"

TSelectRcpt *SelectRcpt;
//---------------------------------------------------------------------------

__fastcall TSelectRcpt::TSelectRcpt(TComponent *Owner)
	: TForm(Owner)
{
	TranslateComponent(this, L"wfigui");

	FNumbers = new TStringList();

	lbNames->Items->Assign(AddressBook);
}
//---------------------------------------------------------------------------

__fastcall TSelectRcpt::~TSelectRcpt()
{
	delete FNumbers;
}
//---------------------------------------------------------------------------

UnicodeString __fastcall TSelectRcpt::GetNumbers()
{
	UnicodeString Result;

	for (int i = 0; i < FNumbers->Count; i++)
		Result += FNumbers->Strings[i] + L";";

	return Result;
}
//---------------------------------------------------------------------------

void __fastcall TSelectRcpt::SplitNumbers(const UnicodeString &Text, TStrings *Dest)
{
	UnicodeString Temp;
	bool bInQuotes = false;

	Dest->Clear();

	for (int i = 1; i <= Text.Length(); i++) {
		if (Text[i] == L';') {
			if (bInQuotes) {
				Temp += Text[i];
			} else {
				Temp = Temp.Trim();
				if (Temp.Length() > 0)
					Dest->Add(Temp);
				Temp = L"";
			}
		} else if (Text[i] == L'"') {
			bInQuotes = !bInQuotes;
			Temp += Text[i];
		} else {
			Temp += Text[i];
		}
	}

	Temp = Temp.Trim();
	if (Temp.Length() > 0)
		Dest->Add(Temp);
}
//---------------------------------------------------------------------------

void __fastcall TSelectRcpt::SetNumbers(const UnicodeString &Value)
{
	int i, pos;

	SplitNumbers(Value, FNumbers);

	lbNames->CheckAll(cbUnchecked, false, false);

	for (i = 0; i < FNumbers->Count; i++) {
		if ((pos = AddressBook->IndexOfNumber(FNumbers->Strings[i])) >= 0)
			lbNames->Checked[pos] = true;
	}
}
//---------------------------------------------------------------------------

void __fastcall TSelectRcpt::FormConstrainedResize(TObject *Sender, int &MinWidth,
  		int &MinHeight, int &MaxWidth, int &MaxHeight)
{
	MinHeight = 150;
	MinWidth = 400;
}
//---------------------------------------------------------------------------

void __fastcall TSelectRcpt::ToggleNumber(int Index, bool Selected)
{
	UnicodeString Number = AddressBook->Numbers[Index];

	if (Selected)
		FNumbers->Add(Number);
	else {
		int pos;

		if ((pos = FNumbers->IndexOf(Number)) >= 0)
			FNumbers->Delete(pos);
	}
}
//---------------------------------------------------------------------------

void __fastcall TSelectRcpt::lbNamesClickCheck(TObject *Sender)
{
	ToggleNumber(lbNames->ItemIndex, lbNames->Checked[lbNames->ItemIndex]);
}
//---------------------------------------------------------------------------

void __fastcall TSelectRcpt::mnuSelectAllClick(TObject *Sender)
{
	int i;
	for (i = 0; i < lbNames->Count; i++) {
		if (!lbNames->Checked[i]) {
			lbNames->Checked[i] = true;
			ToggleNumber(i, true);
		}
	}
}
//---------------------------------------------------------------------------

void __fastcall TSelectRcpt::mnuSelectNoneClick(TObject *Sender)
{
	int i;
	for (i = 0; i < lbNames->Count; i++) {
		if (lbNames->Checked[i]) {
			lbNames->Checked[i] = false;
			ToggleNumber(i, false);
		}
	}
}
//---------------------------------------------------------------------------

void __fastcall TSelectRcpt::lbNamesDrawItem(TWinControl *Control, int Index, TRect &Rect,
  		TOwnerDrawState State)
{
	TRect rect1 = Rect;
	TRect rect2 = Rect;

	rect1.Right = rect1.Left + (rect1.Width() / 2);
	rect2.Left = rect2.Right - (rect2.Width() / 2);

	TCanvas &canv = *lbNames->Canvas;
	canv.FillRect(Rect);
	canv.TextRect(
		rect1,
		rect1.Left + 2,
		rect1.Top + 1,
		lbNames->Items->Strings[Index]);
	canv.TextRect(
		rect2,
		rect2.Left,
		rect2.Top + 1,
		AddressBook->Numbers[Index]);
}
//---------------------------------------------------------------------------

void __fastcall TSelectRcpt::FormCanResize(TObject *Sender, int &NewWidth, int &NewHeight,
  		bool &Resize)
{
	if (NewWidth != Width)
		lbNames->Invalidate();
	Resize = true;
}
//---------------------------------------------------------------------------

