//---------------------------------------------------------------------------

#ifndef frmConfirmH
#define frmConfirmH
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include <ExtCtrls.hpp>
//---------------------------------------------------------------------------
class TConfirmation : public TForm
{
__published:	// IDE-managed Components
	TMemo *Messages;
	TPanel *Panel1;
	TPanel *Panel2;
	TLabel *Result;
	TButton *Close;
	TLabel *Label2;
	void __fastcall FormCreate(TObject *Sender);
private:	// User declarations
public:		// User declarations
	__fastcall TConfirmation(TComponent *Owner);
};
//---------------------------------------------------------------------------
extern PACKAGE TConfirmation *Confirmation;
//---------------------------------------------------------------------------
#endif
