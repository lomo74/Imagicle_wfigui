//---------------------------------------------------------------------------

#ifndef ConfigSerializerIniH
#define ConfigSerializerIniH
//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include "ConfigSerializer.h"
//---------------------------------------------------------------------------

class TConfig;

class TConfigSerializerIni : public IConfigSerializer
{
private:
	TConfig *FConfig;
	UnicodeString FUserIniFile;
	UnicodeString FCommonIniFile;
public:
	TConfigSerializerIni(TConfig *Config);
	virtual ~TConfigSerializerIni() {}
	virtual void Load();
	virtual void Save();
};
//---------------------------------------------------------------------------
#endif
