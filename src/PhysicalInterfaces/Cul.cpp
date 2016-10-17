/* Copyright 2013-2016 Sathya Laufer
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
	_settings = settings;
	_out.init(GD::bl);
	_out.setPrefix(GD::out.getPrefix() + "Intertechno CUL \"" + settings->id + "\": ");

	signal(SIGPIPE, SIG_IGN);
}

Cul::~Cul()
{
	stopListening();
}

void Cul::setup(int32_t userID, int32_t groupID)
{
    try
    {
    	setDevicePermission(userID, groupID);
    }
    catch(const std::exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
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

		_serial.reset(new BaseLib::SerialReaderWriter(_bl, _settings->device, 57600, 0, true, -1));
		_serial->openDevice(false, false, false);
		if(!_serial->isOpen())
		{
			_out.printError("Error: Could not open device.");
			return;
		}

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
    catch(BaseLib::Exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
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
    catch(BaseLib::Exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
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
					/*std::vector<std::string> data{ "i1045510D\r\n", "i1045540D\r\n", "i10515114\r\n", "i1051540D\r\n", "i1054510D\r\n", "i1054540D\r\n", "i1055110D\r\n", "i1055140D\r\n" };
					int32_t index = BaseLib::HelperFunctions::getRandomNumber(0, 7);
					processPacket(data.at(index));
					_lastPacketReceived = BaseLib::HelperFunctions::getTime();*/
					/*if(BaseLib::HelperFunctions::getTimeSeconds() % 10 == 0)
					{
						std::vector<std::string> data{ "i4500140D\r\n", "i4500150D\r\n", "i4540140D\r\n", "i4540150D\r\n", "i4510140D\r\n", "i4510150D\r\n", "i4550140D\r\n", "i4550150D\r\n", "i4504140D\r\n", "i4504150D\r\n", "i4544140D\r\n", "i4544150D\r\n", "i4514140D\r\n", "i4514150D\r\n", "i4554140D\r\n", "i4554150D\r\n", "i4501140D\r\n", "i4501150D\r\n", "i4541140D\r\n", "i4541150D\r\n", "i4511140D\r\n", "i4511150D\r\n", "i4551140D\r\n", "i4551150D\r\n", "i4505140D\r\n", "i4505150D\r\n", "i4545140D\r\n", "i4545150D\r\n", "i4515140D\r\n", "i4515150D\r\n", "i4555140D\r\n", "i4555150D\r\n"  };
						for(uint32_t i = 0; i < data.size(); i++)
						{
							processPacket(data.at(i));
							_lastPacketReceived = BaseLib::HelperFunctions::getTime();
						}
						std::this_thread::sleep_for(std::chrono::milliseconds(1000));
					}*/
					continue;
				}

				processPacket(data);
				_lastPacketReceived = BaseLib::HelperFunctions::getTime();
			}
			catch(const std::exception& ex)
			{
				_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
			}
			catch(BaseLib::Exception& ex)
			{
				_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
			}
			catch(...)
			{
				_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
			}
        }
    }
    catch(const std::exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void Cul::processPacket(std::string& data)
{
	try
	{
		if(data.size() < 6 || data.at(0) != 'i') return;

		PMyPacket packet(new MyPacket(data));
		raisePacketReceived(packet);
	}
	catch(const std::exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
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
		}

		std::string hexString = "is" + myPacket->hexString() + "\n";
		std::vector<char> data;
		data.insert(data.end(), hexString.begin(), hexString.end());

		_out.printInfo("Info: Sending (" + _settings->id + "): " + packet->hexString());

		_serial->writeData(data);
	}
	catch(const std::exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

}
