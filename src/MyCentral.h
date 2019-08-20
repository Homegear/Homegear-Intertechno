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

#ifndef MYCENTRAL_H_
#define MYCENTRAL_H_

#include "MyPeer.h"
#include "MyPacket.h"
#include "MyCULTXPacket.h"
#include <homegear-base/BaseLib.h>

#include <memory>
#include <mutex>
#include <string>
#include "IPacketVisitor.h"

namespace MyFamily
{

class MyCentral
    : public BaseLib::Systems::ICentral
    , public IPacketVisitor
    , public std::enable_shared_from_this<MyCentral>
{
public:
	MyCentral(ICentralEventSink* eventHandler);
	MyCentral(uint32_t deviceType, std::string serialNumber, ICentralEventSink* eventHandler);
	virtual ~MyCentral();
	virtual void dispose(bool wait = true);

	std::string handleCliCommand(std::string command);
	virtual bool onPacketReceived(std::string& senderId, std::shared_ptr<BaseLib::Systems::Packet> packet);

	std::shared_ptr<MyPeer> getPeer(uint64_t id);
	std::shared_ptr<MyPeer> getPeer(int32_t address);
	std::shared_ptr<MyPeer> getPeer(std::string serialNumber);

	virtual PVariable createDevice(BaseLib::PRpcClientInfo clientInfo, int32_t deviceType, std::string serialNumber, int32_t address, int32_t firmwareVersion, std::string interfaceId);
	virtual PVariable deleteDevice(BaseLib::PRpcClientInfo clientInfo, std::string serialNumber, int32_t flags);
	virtual PVariable deleteDevice(BaseLib::PRpcClientInfo clientInfo, uint64_t peerId, int32_t flags);
	virtual PVariable setInterface(BaseLib::PRpcClientInfo clientInfo, uint64_t peerId, std::string interfaceId);

    bool visitPacket(const std::string& senderId, std::shared_ptr<MyPacket> myPacket) override;
    bool visitPacket(const std::string& senderId, std::shared_ptr<MyCULTXPacket> packet) override;
protected:
	virtual void init();
	virtual void loadPeers();
	virtual void savePeers(bool full);
	virtual void loadVariables() {}
	virtual void saveVariables() {}
	std::shared_ptr<MyPeer> createPeer(uint32_t deviceType, int32_t address, std::string serialNumber, bool save = true);
	void deletePeer(uint64_t id);

	std::pair<int32_t, int32_t> getOldItGroupStartCodeAndChannel(int32_t address);
};

}

#endif
