/* Copyright 2013-2019 Homegear GmbH
 *
 * Homegear is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Homegear is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Homegear.  If not, see <http://www.gnu.org/licenses/>.
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
 */

#ifndef CUL_H_
#define CUL_H_

#include "../MyPacket.h"
#include "../MyCULTXPacket.h"
#include <homegear-base/BaseLib.h>
#include "IIntertechnoInterface.h"

namespace MyFamily
{

class Cul : public IIntertechnoInterface
{
public:
	Cul(std::shared_ptr<BaseLib::Systems::PhysicalInterfaceSettings> settings);
	virtual ~Cul();

	virtual void startListening();
	virtual void stopListening();
	virtual void setup(int32_t userID, int32_t groupID, bool setPermissions);
	virtual bool isOpen() { return _serial && _serial->isOpen() && !_stopped; }

	virtual void sendPacket(std::shared_ptr<BaseLib::Systems::Packet> packet);
protected:
	std::unique_ptr<BaseLib::SerialReaderWriter> _serial;

	void listen();
	void processPacket(std::string& data);
};

}

#endif
