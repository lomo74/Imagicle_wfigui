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

#ifndef frmConfigH
#define frmConfigH
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include <ExtCtrls.hpp>
#include <jpeg.hpp>
#include <Dialogs.hpp>
#include <ComCtrls.hpp>
#include <ImgList.hpp>
#include "JvComCtrls.hpp"
#include "JvExControls.hpp"
#include "JvExStdCtrls.hpp"
#include "JvHtControls.hpp"
#include <System.ImageList.hpp>
#include <Vcl.Samples.Spin.hpp>
#include <unicode/utypes.h>
#include <unicode/uloc.h>
//---------------------------------------------------------------------------
class TConfigForm : public TForm
{
__published:	// IDE-managed Components
	TPanel *Panel1;
	TImage *Image1;
	TLabel *Label5;
	TLabel *Label1;
	TEdit *edtServer;
	TLabel *Label2;
	TEdit *edtUsername;
	TLabel *Label3;
	TEdit *edtPassword;
	TLabel *Default;
	TEdit *edtNotificationEmail;
	TPanel *Panel3;
	TButton *btnOK;
	TButton *btnCancel;
	TGroupBox *gbAddressBook;
	TEdit *edtAddressBook;
	TButton *btnBrowse;
	TButton *btnDefault;
	TLabel *lblVersion;
	TLabel *Label11;
	TComboBox *cbLanguage;
	TPageControl *tabs;
	TTabSheet *tsServer;
	TTabSheet *tsAddrBook;
	TComboBox *cbAddrBookType;
	TLabel *Label14;
	TLabel *Label13;
	TGroupBox *gbMAPI;
	TRadioButton *rbMAPIDefProfile;
	TRadioButton *rbMAPIUseProfile;
	TEdit *edtMAPIProfile;
	TImageList *images;
	TGroupBox *gbODBC;
	TEdit *edtODBCDSN;
	TEdit *edtODBCUser;
	TEdit *edtODBCPassword;
	TEdit *edtODBCFax;
	TEdit *edtODBCTable;
	TEdit *edtODBCName;
	TLabel *Label15;
	TLabel *Label16;
	TLabel *Label17;
	TLabel *Label18;
	TLabel *Label19;
	TLabel *Label20;
	TCheckBox *chkODBCAuth;
	TTabSheet *tsOptions;
	TCheckBox *chkNotify;
	TCheckBox *chkUseSSL;
	TCheckBox *chkSkipCertificateCheck;
	TComboBox *cbLogLevel;
	TLabel *Label4;
	void __fastcall btnBrowseClick(TObject *Sender);
	void __fastcall btnDefaultClick(TObject *Sender);
	void __fastcall FormCloseQuery(TObject *Sender, bool &CanClose);
	void __fastcall FormConstrainedResize(TObject *Sender, int &MinWidth, int &MinHeight,
  		int &MaxWidth, int &MaxHeight);
	void __fastcall FormCreate(TObject *Sender);
	void __fastcall edtMAPIProfileChange(TObject *Sender);
	void __fastcall chkODBCAuthClick(TObject *Sender);
	void __fastcall chkUseSSLClick(TObject *Sender);
public:		// User declarations
	__fastcall TConfigForm(TComponent *Owner);
	virtual __fastcall ~TConfigForm();
};
//---------------------------------------------------------------------------
extern PACKAGE TConfigForm *ConfigForm;
//---------------------------------------------------------------------------

#endif
