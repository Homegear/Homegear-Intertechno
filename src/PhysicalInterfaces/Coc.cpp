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

#include "Coc.h"
#include "../GD.h"
#include "../MyCulTxPacket.h"
#include "../MyPacket.h"

namespace MyFamily
{

Coc::Coc(std::shared_ptr<BaseLib::Systems::PhysicalInterfaceSettings> settings) : IIntertechnoInterface(settings)
{
	_out.init(GD::bl);
	_out.setPrefix(GD::out.getPrefix() + "COC \"" + settings->id + "\": ");

	_stackPrefix = "";
	for(uint32_t i = 1; i < settings->stackPosition; i++)
	{
		_stackPrefix.push_back('*');
	}
}

Coc::~Coc()
{
	try
	{
		if(_socket)
		{
			_socket->removeEventHandler(_eventHandlerSelf);
			_socket->closeDevice();
			_socket.reset();
		}
	}
    catch(const std::exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
}

void Coc::sendPacket(std::shared_ptr<BaseLib::Systems::Packet> packet)
{
	try
	{
		std::shared_ptr<MyPacket> myPacket(std::dynamic_pointer_cast<MyPacket>(packet));
		if(!myPacket) return;
		
		if(!_socket)
		{
			_out.printError("Error: Couldn't write to COC device, because the device descriptor is not valid: " + _settings->device);
			return;
		}
		
		std::string hexString = "is" + myPacket->hexString() + "\n";
		std::vector<char> data;
		data.insert(data.end(), hexString.begin(), hexString.end());

		_out.printInfo("Info: Sending (" + _settings->id + "): " + myPacket->hexString());

		_socket->writeData(data);
		_lastPacketSent = BaseLib::HelperFunctions::getTime();
	}
	catch(const std::exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
}

void Coc::startListening()
{
	try
	{
		_socket = GD::bl->serialDeviceManager.get(_settings->device);
		if(!_socket) _socket = GD::bl->serialDeviceManager.create(_settings->device, 38400, O_RDWR | O_NOCTTY | O_NDELAY, true, 45);
		if(!_socket) return;
		_eventHandlerSelf = _socket->addEventHandler(this);
		_socket->openDevice(false, false);
		if(gpioDefined(2))
		{
			openGPIO(2, false);
			if(!getGPIO(2)) setGPIO(2, true);
			closeGPIO(2);
		}
		if(gpioDefined(1))
		{
			openGPIO(1, false);
			if(!getGPIO(1))
			{
				setGPIO(1, false);
				std::this_thread::sleep_for(std::chrono::milliseconds(1000));
				setGPIO(1, true);
				std::this_thread::sleep_for(std::chrono::milliseconds(2000));
			}
			closeGPIO(1);
		}
		std::string listenPacket = "X21\r\n";
		_socket->writeLine(listenPacket);
		if(!_additionalCommands.empty()) _socket->writeLine(_additionalCommands);
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		IPhysicalInterface::startListening();
	}
    catch(const std::exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
}

void Coc::stopListening()
{
	try
	{
		if(!_socket) return;
		_socket->removeEventHandler(_eventHandlerSelf);
		_socket->closeDevice();
		_socket.reset();
		IPhysicalInterface::stopListening();
	}
	catch(const std::exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
}

void Coc::lineReceived(const std::string& data)
{
    try
    {
		if(GD::bl->debugLevel >= 5)
		{
			std::string rawPacket = data;
			_out.printDebug("Debug: Raw packet received: " + BaseLib::HelperFunctions::trim(rawPacket));
		}

    	std::string packetHex;
		if(_stackPrefix.empty())
		{
			if(data.size() > 0 && data.at(0) == '*') return;
			packetHex = data;
		}
		else
		{
			if(data.size() + 1 <= _stackPrefix.size()) return;
			if(data.substr(0, _stackPrefix.size()) != _stackPrefix || data.at(_stackPrefix.size()) == '*') return;
			else packetHex = data.substr(_stackPrefix.size());
		}
		
		std::shared_ptr<BaseLib::Systems::Packet> packet = nullptr;

	    // CULTX
	    if(packetHex.size() > 9 && packetHex.at(0) == 't' && (packetHex.at(5) == packetHex.at(8) || packetHex.at(6) == packetHex.at(9)))
		{
	    	if(GD::bl->debugLevel >= 5) _out.printDebug("Debug: Recognized CULTX packet");
			 packet = std::make_shared<MyCulTxPacket>(packetHex);
			 packet->setTag(GD::CULTX);
			raisePacketReceived(packet);
			return;
		}

	    // Intertechno
	    if(packetHex.size() > 6 && packetHex.at(0) == 'i') {
	    	if(GD::bl->debugLevel >= 5) _out.printDebug("Debug: Recognized Intertechno packet");
	    	packet = std::make_shared<MyPacket>(packetHex);
	    	packet->setTag(GD::INTERTECHNO);
	    	raisePacketReceived(packet);
	    	return;
	    }

	    // Not recognized
		if(packetHex == "LOVF\n") _out.printWarning("Warning: COC with id " + _settings->id + " reached 1% limit. You need to wait, before sending is allowed again.");
		else _out.printInfo("Info: Unknown IT packet received: " + packetHex);
		return;


    }
    catch(const std::exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
}

void Coc::setup(int32_t userID, int32_t groupID, bool setPermissions)
{
    try
    {
    	if(setPermissions) setDevicePermission(userID, groupID);
    	exportGPIO(1);
		if(setPermissions) setGPIOPermission(1, userID, groupID, false);
		setGPIODirection(1, GPIODirection::OUT);
		exportGPIO(2);
		if(setPermissions) setGPIOPermission(2, userID, groupID, false);
		setGPIODirection(2, GPIODirection::OUT);
    }
    catch(const std::exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
}

}
