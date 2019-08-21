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

#include "../GD.h"
#include "Cul.h"

namespace MyFamily
{

Cul::Cul(std::shared_ptr<BaseLib::Systems::PhysicalInterfaceSettings> settings) : IIntertechnoInterface(settings)
{
	_out.init(GD::bl);
	_out.setPrefix(GD::out.getPrefix() + "Intertechno CUL \"" + settings->id + "\": ");

	signal(SIGPIPE, SIG_IGN);
}

Cul::~Cul()
{
	stopListening();
}

void Cul::setup(int32_t userID, int32_t groupID, bool setPermissions)
{
    try
    {
    	if(setPermissions) setDevicePermission(userID, groupID);
    }
    catch(const std::exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
}

void Cul::startListening()
{
	try
	{
		stopListening();

		if(_settings->device.empty())
		{
			_out.printError("Error: No device defined for CUL. Please specify it in \"intertechno.conf\".");
			return;
		}

		if(_settings->baudrate <= 0) _settings->baudrate = 57600;
		_serial.reset(new BaseLib::SerialReaderWriter(_bl, _settings->device, _settings->baudrate, 0, true, -1));
		_serial->openDevice(false, false, false);
		if(!_serial->isOpen())
		{
			_out.printError("Error: Could not open device.");
			return;
		}
		std::string listenPacket = "X21\r\n";
		_serial->writeLine(listenPacket);
		if(!_additionalCommands.empty()) _serial->writeLine(_additionalCommands);

		_stopCallbackThread = false;
		_stopped = false;
		if(_settings->listenThreadPriority > -1) _bl->threadManager.start(_listenThread, true, _settings->listenThreadPriority, _settings->listenThreadPolicy, &Cul::listen, this);
		else _bl->threadManager.start(_listenThread, true, &Cul::listen, this);
		IPhysicalInterface::startListening();
	}
    catch(const std::exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
}

void Cul::stopListening()
{
	try
	{
		_stopCallbackThread = true;
		_bl->threadManager.join(_listenThread);
		_stopped = true;
		if(_serial) _serial->closeDevice();
		IPhysicalInterface::stopListening();
	}
	catch(const std::exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
}

void Cul::listen()
{
    try
    {
    	std::string data;
    	data.reserve(100);
    	int32_t result = 0;

        while(!_stopCallbackThread)
        {
        	try
        	{
				if(_stopped || !_serial->isOpen())
				{
					if(_stopCallbackThread) return;
					if(_stopped) _out.printWarning("Warning: Connection to device closed. Trying to reconnect...");
					_serial->closeDevice();
					std::this_thread::sleep_for(std::chrono::milliseconds(10000));
					_serial->openDevice(false, false, false);
					if(!_serial->isOpen())
					{
						_out.printError("Error: Could not open device.");
						return;
					}
					std::string listenPacket = "X21\r\n";
					_serial->writeLine(listenPacket);
					if(!_additionalCommands.empty()) _serial->writeLine(_additionalCommands);
					continue;
				}

				result = _serial->readLine(data);
				if(result == -1)
				{
					_out.printError("Error reading from serial device.");
					_stopped = true;
					continue;
				}
				else if(result == 1)
				{
#ifdef DEBUG
					//std::vector<std::string> data{ "i1045510D\r\n", "i1045540D\r\n", "i10515114\r\n", "i1051540D\r\n", "i1054510D\r\n", "i1054540D\r\n", "i1055110D\r\n", "i1055140D\r\n" };
					// Debug CULTX
					std::vector<std::string> data
					{
					"tAEFD49049E0F\r\n",
					"tAE1134034807\r\n",
					"tA0FD7287202C\r\n",
					"tA01077577C11\r\n",
					"tAE1134034809\r\n",
					"tA01077577C0A\r\n",
					"tAEFD49049E37\r\n",
					"tA01077577CFC\r\n",
					"tAE11340348FD\r\n",
					"tAED357057011\r\n",
					"tA0FC72972038\r\n",
					"tAEFD49049E38\r\n",
					"tAED35705700A\r\n",
					"tA01077577C07\r\n",
					"tAE1134034804\r\n",
					"tA0FC7297203A\r\n",
					"tA01077577C09\r\n",
					"tA01077577C09\r\n",
					"tAE1134034807\r\n",
					"tAED35705700A\r\n",
					"tA01077577C08\r\n",
					"tAE1134034809\r\n",
					"tAED35705700B\r\n",
					"tA0FC73173A3A\r\n"
					};

					int32_t index = BaseLib::HelperFunctions::getRandomNumber(0, data.size()-1);
					processPacket(data.at(index));
					_lastPacketReceived = BaseLib::HelperFunctions::getTime();
					std::this_thread::sleep_for(std::chrono::milliseconds(3000));
#endif
					continue;
				}

				processPacket(data);
				_lastPacketReceived = BaseLib::HelperFunctions::getTime();
			}
			catch(const std::exception& ex)
			{
				_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
			}
        }
    }
    catch(const std::exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
}

void Cul::processPacket(std::string& data)
{
	try
	{
		std::shared_ptr<BaseLib::Systems::Packet> packet = nullptr;

	    if(GD::bl->debugLevel >= 5) _out.printDebug("Debug: Raw packet received: " + BaseLib::HelperFunctions::trim(data));

	    // CULTX
	    if(data.size() > 9 && data.at(0) == 't' && (data.at(5) == data.at(8) || data.at(6) == data.at(9)))
		{
	    	if(GD::bl->debugLevel >= 5) _out.printDebug("Debug: Recognized CULTX packet");
			packet = std::make_shared<MyCulTxPacket>(data);
			packet->setTag(GD::CULTX);
			raisePacketReceived(packet);
			return;
		}

	    // Intertechno
	    if(data.size() > 6 && data.at(0) == 'i') {
	    	if(GD::bl->debugLevel >= 5) _out.printDebug("Debug: Recognized Intertechno packet");
	    	packet = std::make_shared<MyPacket>(data);
	    	packet->setTag(GD::INTERTECHNO);
	    	raisePacketReceived(packet);
	    	return;
	    }

	    // Not recognized
		if(data.compare(0, 4, "LOVF") == 0) _out.printWarning("Warning: CUL with id " + _settings->id + " reached 1% limit. You need to wait, before sending is allowed again.");
		else _out.printInfo("Info: Unknown IT packet received: " + data);
		return;

	}
	catch(const std::exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
}

void Cul::sendPacket(std::shared_ptr<BaseLib::Systems::Packet> packet)
{
	try
	{
		std::shared_ptr<MyPacket> myPacket(std::dynamic_pointer_cast<MyPacket>(packet));
		if(!myPacket) return;

		if(_stopped || !_serial)
		{
			_out.printWarning("Warning: !!!Not!!! sending packet " + myPacket->hexString() + ", because device is not open.");
			return;
		}

		if(!_serial->isOpen())
		{
			_serial->closeDevice();
			_serial->openDevice(false, false, false);
			if(!_serial->isOpen())
			{
				_out.printError("Error: Could not open device.");
				return;
			}
			std::string listenPacket = "X21\r\n";
			_serial->writeLine(listenPacket);
			if(!_additionalCommands.empty()) _serial->writeLine(_additionalCommands);
		}

		std::string hexString = "is" + myPacket->hexString() + "\n";
		std::vector<char> data;
		data.insert(data.end(), hexString.begin(), hexString.end());

		_out.printInfo("Info: Sending (" + _settings->id + "): " + myPacket->hexString());

		_serial->writeData(data);
		_lastPacketSent = BaseLib::HelperFunctions::getTime();
		// Sleep as CUL cannot handle too much commands in short time
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
	}
	catch(const std::exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
}

}
