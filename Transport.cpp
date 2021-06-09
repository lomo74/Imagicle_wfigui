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
#pragma hdrstop

#include "Transport.h"
#include "TransportStoneFax.h"

#pragma package(smart_init)
//---------------------------------------------------------------------------

void TTransport::NotifyAll(TTransportEvent aEvent, TFax *pFax, bool result, Exception *pError)
{
	std::vector<ITransportNotify *>::iterator it;

	for (it = FNotifyTargets.begin(); it != FNotifyTargets.end(); it++) {
		TThread::Synchronize(TThread::Current, [&]() {
			try {
				(*it)->TransportNotify(aEvent, pFax, result, pError);
			}
			catch (...) {
				//TODO: loggare errori
			}
		});
	}
}
//---------------------------------------------------------------------------

