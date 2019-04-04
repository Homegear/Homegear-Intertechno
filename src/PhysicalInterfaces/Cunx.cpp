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

#include "Cunx.h"
#include <homegear-base/BaseLib.h>
#include "../GD.h"
#include "../MyPacket.h"
#include "../MyCULTXPacket.h"

namespace MyFamily
{

Cunx::Cunx(std::shared_ptr<BaseLib::Systems::PhysicalInterfaceSettings> settings) : IIntertechnoInterface(settings)
{
	_out.init(GD::bl);
	_out.setPrefix(GD::out.getPrefix() + "CUNX \"" + settings->id + "\": ");

	signal(SIGPIPE, SIG_IGN);

	_socket = std::unique_ptr<BaseLib::TcpSocket>(new BaseLib::TcpSocket(_bl));

	if(settings->listenThreadPriority == -1)
	{
		settings->listenThreadPriority = 45;
		settings->listenThreadPolicy = SCHED_FIFO;
	}
}

Cunx::~Cunx()
{
	try
	{
		_stopCallbackThread = true;
		GD::bl->threadManager.join(_listenThread);
	}
    catch(const std::exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
}

void Cunx::sendPacket(std::shared_ptr<BaseLib::Systems::Packet> packet)
{
	try
	{
		std::shared_ptr<MyPacket> myPacket(std::dynamic_pointer_cast<MyPacket>(packet));
		if(!myPacket) return;

		if(!isOpen())
		{
			_out.printWarning(std::string("Warning: !!!Not!!! sending packet, because device is not connected or opened: ") + myPacket->hexString());
			return;
		}

		_out.printInfo("Info: Sending (" + _settings->id + "): " + myPacket->hexString());
		send("is" + myPacket->hexString() + "\n");
		
		_lastPacketSent = BaseLib::HelperFunctions::getTime();
	}
	catch(const std::exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
}

void Cunx::send(std::string data)
{
	try
    {
    	if(data.size() < 3) return; //Otherwise error in printWarning
		std::lock_guard<std::mutex> sendGuard(_sendMutex);
    	if(!_socket->connected() || _stopped)
    	{
    		_out.printWarning(std::string("Warning: !!!Not!!! sending: ") + data.substr(2, data.size() - 3));
    		return;
    	}
    	_socket->proofwrite(data);
    	 return;
    }
    catch(const BaseLib::SocketOperationException& ex)
    {
    	_out.printError(ex.what());
    }
    catch(const std::exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    _stopped = true;
}

void Cunx::startListening()
{
	try
	{
		stopListening();
		_socket = std::unique_ptr<BaseLib::TcpSocket>(new BaseLib::TcpSocket(_bl, _settings->host, _settings->port, _settings->ssl, _settings->caFile, _settings->verifyCertificate));
		_socket->setAutoConnect(false);
		_out.printDebug("Connecting to CUNX with hostname " + _settings->host + " on port " + _settings->port + "...");
		_stopped = false;
		if(_settings->listenThreadPriority > -1) GD::bl->threadManager.start(_listenThread, true, _settings->listenThreadPriority, _settings->listenThreadPolicy, &Cunx::listen, this);
		else GD::bl->threadManager.start(_listenThread, true, &Cunx::listen, this);
		IPhysicalInterface::startListening();
	}
    catch(const std::exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
}

void Cunx::reconnect()
{
	try
	{
		_socket->close();
		_out.printDebug("Connecting to CUNX device with hostname " + _settings->host + " on port " + _settings->port + "...");
		_socket->open();
		_hostname = _settings->host;
		_ipAddress = _socket->getIpAddress();
		_stopped = false;
		send("X21\r\n");
		if(!_additionalCommands.empty()) send(_additionalCommands);
		_out.printInfo("Connected to CUNX device with hostname " + _settings->host + " on port " + _settings->port + ".");
	}
    catch(const std::exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
}

void Cunx::stopListening()
{
	try
	{
		_stopCallbackThread = true;
		GD::bl->threadManager.join(_listenThread);
		_stopCallbackThread = false;
		_socket->close();
		_stopped = true;
		IPhysicalInterface::stopListening();
	}
	catch(const std::exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
}

void Cunx::listen()
{
    try
    {
    	uint32_t receivedBytes = 0;
    	int32_t bufferMax = 2048;
		std::vector<char> buffer(bufferMax);

        while(!_stopCallbackThread)
        {
        	if(_stopped || !_socket->connected())
        	{
        		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        		if(_stopCallbackThread) return;
        		if(_stopped) _out.printWarning("Warning: Connection to CUNX closed. Trying to reconnect...");
        		reconnect();
        		continue;
        	}
        	std::vector<uint8_t> data;
			try
			{
				do
				{
					receivedBytes = _socket->proofread(&buffer[0], bufferMax);
					if(receivedBytes > 0)
					{
						data.insert(data.end(), &buffer.at(0), &buffer.at(0) + receivedBytes);
						if(data.size() > 1000000)
						{
							_out.printError("Could not read from CUNX: Too much data.");
							break;
						}
					}
				} while(receivedBytes == (unsigned)bufferMax);
			}
			catch(const BaseLib::SocketTimeOutException& ex)
			{
				if(data.empty()) continue; //When receivedBytes is exactly 2048 bytes long, proofread will be called again, time out and the packet is received with a delay of 5 seconds. It doesn't matter as packets this big are only received at start up.
			}
			catch(const BaseLib::SocketClosedException& ex)
			{
				_stopped = true;
				_out.printWarning("Warning: " + std::string(ex.what()));
				std::this_thread::sleep_for(std::chrono::milliseconds(10000));
				continue;
			}
			catch(const BaseLib::SocketOperationException& ex)
			{
				_stopped = true;
				_out.printError("Error: " + std::string(ex.what()));
				std::this_thread::sleep_for(std::chrono::milliseconds(10000));
				continue;
			}
			if(data.empty() || data.size() > 1000000) continue;

        	if(_bl->debugLevel >= 6)
        	{
        		_out.printDebug("Debug: Packet received from CUNX. Raw data:");
        		_out.printBinary(data);
        	}

        	processData(data);

        	_lastPacketReceived = BaseLib::HelperFunctions::getTime();
        }
    }
    catch(const std::exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
}

void Cunx::processData(std::vector<uint8_t>& data)
{
	try
	{
		if(data.empty()) return;
		std::string packets;
		packets.insert(packets.end(), data.begin(), data.end());

		if(GD::bl->debugLevel >= 5) _out.printDebug("Debug: Raw packet received: " + BaseLib::HelperFunctions::trim(packets));

		std::istringstream stringStream(packets);
		std::string packetHex;
		while(std::getline(stringStream, packetHex))
		{

		    // CULTX
		    if(packetHex.size() > 9 && packetHex.at(0) == 't' && (packetHex.at(5) == packetHex.at(8) || packetHex.at(6) == packetHex.at(9)))
			{
		    	if(GD::bl->debugLevel >= 5) _out.printDebug("Debug: Recognized CULTX packet");
		    	PMyCULTXPacket packet(new MyCULTXPacket(packetHex));
				raisePacketReceived(packet);
				continue;
			}

		    // Intertechno
		    if(packetHex.size() > 6 && packetHex.at(0) == 'i') {
		    	if(GD::bl->debugLevel >= 5) _out.printDebug("Debug: Recognized Intertechno packet");
		    	PMyPacket packet(new MyPacket(packetHex));
		    	raisePacketReceived(packet);
		    	continue;
		    }

		    // Not recognized
			if(packetHex == "LOVF\n") _out.printWarning("Warning: CUNX with id " + _settings->id + " reached 1% limit. You need to wait, before sending is allowed again.");
			else _out.printInfo("Info: Unknown IT packet received: " + packetHex);
			continue;

		}
	}
    catch(const std::exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
}
}
