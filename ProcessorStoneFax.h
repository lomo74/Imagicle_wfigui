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

#ifndef ProcessorStoneFaxH
#define ProcessorStoneFaxH
//---------------------------------------------------------------------------
#include <vcl.h>
#include "magic.h"
#pragma hdrstop

#include "Processor.h"
//---------------------------------------------------------------------------

class TProcessorStoneFax : public IProcessor
{
private:
	magic_t FMagic;
    TMemoryStream *FMagicData;
	TStringList *FProcessedFiles;
	char *FBuffer;
	UnicodeString FWorkingDirectory;

public:
	TProcessorStoneFax();
	virtual ~TProcessorStoneFax();

	virtual void SetWorkingDirectory(const UnicodeString &aWorkDir);
	virtual UnicodeString ProcessFile(const UnicodeString &aFile);
	virtual void Cleanup();
};
//---------------------------------------------------------------------------
#endif
