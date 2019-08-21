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

#include "TiCc1100.h"

#ifdef SPISUPPORT
#include "../GD.h"
#include "../MyPacket.h"
#include "../MyCulTxPacket.h"

namespace MyFamily
{

TiCc1100::TiCc1100(std::shared_ptr<BaseLib::Systems::PhysicalInterfaceSettings> settings) : IIntertechnoInterface(settings)
{
	try
	{
		_out.init(GD::bl);
		_out.setPrefix(GD::out.getPrefix() + "TI CC110X \"" + settings->id + "\": ");

		_sending = false;

		if(settings->listenThreadPriority == -1)
		{
			settings->listenThreadPriority = 45;
			settings->listenThreadPolicy = SCHED_FIFO;
		}
		if(settings->oscillatorFrequency < 0) settings->oscillatorFrequency = 26000000;
		if(settings->txPowerSetting < 0) settings->txPowerSetting = (gpioDefined(2)) ? 0x27 : 0xC0;
		_out.printDebug("Debug: PATABLE will be set to 0x" + BaseLib::HelperFunctions::getHexString(settings->txPowerSetting, 2));
		if(settings->interruptPin != 0 && settings->interruptPin != 2)
		{
			if(settings->interruptPin > 0) _out.printWarning("Warning: Setting for interruptPin for device CC1100 in intertechno.conf is invalid.");
			settings->interruptPin = 2;
		}

		_spi.reset(new BaseLib::LowLevel::Spi(GD::bl, settings->device, BaseLib::LowLevel::SpiModes::none, 8, 4000000));

		setConfig();
	}
    catch(const std::exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
}

TiCc1100::~TiCc1100()
{
	try
	{
		_stopCallbackThread = true;
		_bl->threadManager.join(_listenThread);
		_spi->close();
		closeGPIO(1);
	}
    catch(const std::exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
}

void TiCc1100::setConfig()
{
	if(_settings->oscillatorFrequency == 26000000)
	{
		_config =
		{
			(_settings->interruptPin == 2) ? (uint8_t)0x46 : (uint8_t)0x5B, //00: IOCFG2 (GDO2_CFG)
			0x2E, //01: IOCFG1 (GDO1_CFG to High impedance (3-state))
			(_settings->interruptPin == 0) ? (uint8_t)0x46 : (uint8_t)0x5B, //02: IOCFG0 (GDO0_CFG)
			0x07, //03: FIFOTHR (FIFO threshold to 33 (TX) and 32 (RX)
			0xD3, //04: SYNC1
			0x91, //05: SYNC0
			0x3D, //06: PKTLEN (Maximum packet length)
			0x04, //07: PKTCTRL1
			0x32, //08: PKTCTRL0
			0x00, //09: ADDR
			0x00, //0A: CHANNR
			0x06, //0B: FSCTRL1
			0x00, //0C: FSCTRL0
			0x10, //0D: FREQ2
			0xB0, //0E: FREQ1
			0x71, //0F: FREQ0
			0x55, //10: MDMCFG4
			0xE4, //11: MDMCFG3
			0x30, //12: MDMCFG2
			0x23, //13: MDMCFG1
			0xB9, //14: MDMCFG0
			0x00, //15: DEVIATN
			0x07, //16: MCSM2
			0x30, //17: MCSM1: IDLE when packet has been received, RX after sending
			0x18, //18: MCSM0
			0x14, //19: FOCCFG
			0x6C, //1A: BSCFG
			0x07, //1B: AGCCTRL2
			0x00, //1C: AGCCTRL1
			0x90, //1D: AGCCTRL0
			0x87, //1E: WOREVT1
			0x6B, //1F: WOREVT0
			0xF8, //20: WORCRTL
			0x56, //21: FREND1
			0x11, //22: FREND0
			0xE9, //23: FSCAL3
			0x2A, //24: FSCAL2
			0x00, //25: FSCAL1
			0x1F, //26: FSCAL0
			0x41, //27: RCCTRL1
			0x00, //28: RCCTRL0
		};
	}
	else if(_settings->oscillatorFrequency == 27000000) _out.printError("Error: Unsupported value for \"oscillatorFrequency\". Currently only 26000000 is supported.");
	else _out.printError("Error: Unknown value for \"oscillatorFrequency\" in intertechno.conf. The only valid value currently is 26000000.");
}

void TiCc1100::setup(int32_t userID, int32_t groupID, bool setPermissions)
{
	try
	{
		_out.printDebug("Debug: CC1100: Setting device permissions");
		if(setPermissions) setDevicePermission(userID, groupID);
		_out.printDebug("Debug: CC1100: Exporting GPIO");
		exportGPIO(1);
		if(gpioDefined(2)) exportGPIO(2);
		_out.printDebug("Debug: CC1100: Setting GPIO permissions");
		if(setPermissions) setGPIOPermission(1, userID, groupID, false);
		if(setPermissions && gpioDefined(2)) setGPIOPermission(2, userID, groupID, false);
		if(gpioDefined(2)) setGPIODirection(2, GPIODirection::OUT);
	}
    catch(const std::exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
}

void TiCc1100::sendPacket(std::shared_ptr<BaseLib::Systems::Packet> packet)
{
	try
	{
		if(!packet)
		{
			_out.printWarning("Warning: Packet was nullptr.");
			return;
		}
		if(_spi->isOpen() || _stopped) return;

        std::shared_ptr<MyPacket> myPacket(std::dynamic_pointer_cast<MyPacket>(packet));
        if(!myPacket) return;

		if(myPacket->hexString().size() > 128)
		{
			_out.printError("Error: Tried to send packet larger than 64 bytes. That is not supported.");
			return;
		}
		std::vector<uint8_t> packetBytes; // = myPacket->byteArray();

		int64_t timeBeforeLock = BaseLib::HelperFunctions::getTime();
		_sendingPending = true;
		_txMutex.lock();
		_sendingPending = false;
		if(_stopCallbackThread || !_spi->isOpen() || _gpioDescriptors[1]->descriptor == -1 || _stopped)
		{
			_txMutex.unlock();
			return;
		}
		_sending = true;
		sendCommandStrobe(CommandStrobes::Enum::SIDLE);
		sendCommandStrobe(CommandStrobes::Enum::SFTX);
		_lastPacketSent = BaseLib::HelperFunctions::getTime();
		if(_lastPacketSent - timeBeforeLock > 100)
		{
			_out.printWarning("Warning: You're sending too many packets at once. Sending Intertechno packets takes a looong time!");
		}
		writeRegisters(Registers::Enum::FIFO, packetBytes);
		sendCommandStrobe(CommandStrobes::Enum::STX);

		if(_bl->debugLevel > 3)
		{
			if(myPacket->getTimeSending() > 0)
			{
				_out.printInfo("Info: Sending (" + _settings->id + "): " + myPacket->hexString() + " Planned sending time: " + BaseLib::HelperFunctions::getTimeString(packet->getTimeSending()));
			}
			else
			{
				_out.printInfo("Info: Sending (" + _settings->id + "): " + myPacket->hexString());
			}
		}

		//Unlocking of _txMutex takes place in mainThread
	}
	catch(const std::exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
}

bool TiCc1100::checkStatus(uint8_t statusByte, Status::Enum status)
{
	try
	{
		if(!_spi->isOpen() || _gpioDescriptors[1]->descriptor == -1) return false;
		if((statusByte & (StatusBitmasks::Enum::CHIP_RDYn | StatusBitmasks::Enum::STATE)) != status) return false;
		return true;
	}
    catch(const std::exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    return false;
}

uint8_t TiCc1100::readRegister(Registers::Enum registerAddress)
{
	try
	{
		if(!_spi->isOpen()) return 0;
		std::vector<uint8_t> data({(uint8_t)(registerAddress | RegisterBitmasks::Enum::READ_SINGLE), 0x00});
		for(uint32_t i = 0; i < 5; i++)
		{
			_spi->readwrite(data);
			if(!(data.at(0) & StatusBitmasks::Enum::CHIP_RDYn)) break;
			data.at(0) = (uint8_t)(registerAddress  | RegisterBitmasks::Enum::READ_SINGLE);
			data.at(1) = 0;
			usleep(20);
		}
		return data.at(1);
	}
    catch(const std::exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    return 0;
}

std::vector<uint8_t> TiCc1100::readRegisters(Registers::Enum startAddress, uint8_t count)
{
	try
	{
		if(!_spi->isOpen()) return std::vector<uint8_t>();
		std::vector<uint8_t> data({(uint8_t)(startAddress | RegisterBitmasks::Enum::READ_BURST)});
		data.resize(count + 1, 0);
		for(uint32_t i = 0; i < 5; i++)
		{
			_spi->readwrite(data);
			if(!(data.at(0) & StatusBitmasks::Enum::CHIP_RDYn)) break;
			data.clear();
			data.push_back((uint8_t)(startAddress  | RegisterBitmasks::Enum::READ_BURST));
			data.resize(count + 1, 0);
			usleep(20);
		}
		return data;
	}
    catch(const std::exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    return std::vector<uint8_t>();
}

uint8_t TiCc1100::writeRegister(Registers::Enum registerAddress, uint8_t value, bool check)
{
	try
	{
		if(!_spi->isOpen()) return 0xFF;
		std::vector<uint8_t> data({(uint8_t)registerAddress, value});
		_spi->readwrite(data);
		if((data.at(0) & StatusBitmasks::Enum::CHIP_RDYn) || (data.at(1) & StatusBitmasks::Enum::CHIP_RDYn)) throw BaseLib::Exception("Error writing to register " + std::to_string(registerAddress) + ".");

		if(check)
		{
			data.at(0) = registerAddress | RegisterBitmasks::Enum::READ_SINGLE;
			data.at(1) = 0;
			_spi->readwrite(data);
			if(data.at(1) != value)
			{
				_out.printError("Error (check) writing to register " + std::to_string(registerAddress) + ".");
				return 0;
			}
		}
		return value;
	}
    catch(const std::exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    return 0;
}

void TiCc1100::writeRegisters(Registers::Enum startAddress, std::vector<uint8_t>& values)
{
	try
	{
		if(!_spi->isOpen()) return;
		std::vector<uint8_t> data({(uint8_t)(startAddress | RegisterBitmasks::Enum::WRITE_BURST) });
		data.insert(data.end(), values.begin(), values.end());
		_spi->readwrite(data);
		if((data.at(0) & StatusBitmasks::Enum::CHIP_RDYn)) _out.printError("Error writing to registers " + std::to_string(startAddress) + ".");
	}
    catch(const std::exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
}

uint8_t TiCc1100::sendCommandStrobe(CommandStrobes::Enum commandStrobe)
{
	try
	{
		if(!_spi->isOpen()) return 0xFF;
		std::vector<uint8_t> data({(uint8_t)commandStrobe});
		for(uint32_t i = 0; i < 5; i++)
		{
			_spi->readwrite(data);
			if(!(data.at(0) & StatusBitmasks::Enum::CHIP_RDYn)) break;
			data.at(0) = (uint8_t)commandStrobe;
			usleep(20);
		}
		return data.at(0);
	}
    catch(const std::exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    return 0;
}

void TiCc1100::enableRX(bool flushRXFIFO)
{
	try
	{
		if(!_spi->isOpen()) return;
		_txMutex.lock();
		if(flushRXFIFO) sendCommandStrobe(CommandStrobes::Enum::SFRX);
		sendCommandStrobe(CommandStrobes::Enum::SRX);
	}
    catch(const std::exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    _txMutex.unlock();
}

void TiCc1100::initChip()
{
	try
	{
		if(!_spi->isOpen())
		{
			_out.printError("Error: Could not initialize TI CC1100. The spi device's file descriptor is not valid.");
			return;
		}
		reset();

		int32_t index = 0;
		for(std::vector<uint8_t>::const_iterator i = _config.begin(); i != _config.end(); ++i)
		{
			if(writeRegister((Registers::Enum)index, *i, true) != *i)
			{
				_spi->close();
				return;
			}
			index++;
		}
		if(writeRegister(Registers::Enum::FSTEST, 0x59, true) != 0x59)
		{
			_spi->close();
			return;
		}
		if(writeRegister(Registers::Enum::TEST2, 0x81, true) != 0x81) //Determined by SmartRF Studio
		{
			_spi->close();
			return;
		}
		if(writeRegister(Registers::Enum::TEST1, 0x35, true) != 0x35) //Determined by SmartRF Studio
		{
			_spi->close();
			return;
		}
		if(writeRegister(Registers::Enum::PATABLE, _settings->txPowerSetting, true) != _settings->txPowerSetting)
		{
			_spi->close();
			return;
		}

		sendCommandStrobe(CommandStrobes::Enum::SFRX);
		usleep(20);

		enableRX(true);
	}
    catch(const std::exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
}

void TiCc1100::reset()
{
	try
	{
		if(!_spi->isOpen()) return;
		sendCommandStrobe(CommandStrobes::Enum::SRES);

		usleep(70); //Measured on HM-CC-VD
	}
    catch(const std::exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
}

bool TiCc1100::crcOK()
{
	try
	{
		if(!_spi->isOpen()) return false;
		std::vector<uint8_t> result = readRegisters(Registers::Enum::LQI, 1);
		if((result.size() == 2) && (result.at(1) & 0x80)) return true;
	}
    catch(const std::exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    return false;
}

void TiCc1100::startListening()
{
	try
	{
		stopListening();
		initDevice();

		_stopped = false;
		_firstPacket = true;
		_stopCallbackThread = false;
		if(_settings->listenThreadPriority > -1) GD::bl->threadManager.start(_listenThread, true, _settings->listenThreadPriority, _settings->listenThreadPolicy, &TiCc1100::mainThread, this);
		else GD::bl->threadManager.start(_listenThread, true, &TiCc1100::mainThread, this);
		IPhysicalInterface::startListening();
	}
    catch(const std::exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
}

void TiCc1100::initDevice()
{
	try
	{
		_spi->open();
		if(!_spi->isOpen()) return;

		initChip();
		_out.printDebug("Debug: CC1100: Setting GPIO direction");
		setGPIODirection(1, GPIODirection::IN);
		_out.printDebug("Debug: CC1100: Setting GPIO edge");
		setGPIOEdge(1, GPIOEdge::BOTH);
		openGPIO(1, true);
		if(!_gpioDescriptors[1] || _gpioDescriptors[1]->descriptor == -1) throw(BaseLib::Exception("Couldn't listen to rf device, because the gpio pointer is not valid: " + _settings->device));
		if(gpioDefined(2)) //Enable high gain mode
		{
			openGPIO(2, false);
			if(!getGPIO(2)) setGPIO(2, true);
			closeGPIO(2);
		}
	}
    catch(const std::exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
}

void TiCc1100::stopListening()
{
	try
	{
		_stopCallbackThread = true;
		_bl->threadManager.join(_listenThread);
		_stopCallbackThread = false;
		if(_spi->isOpen()) _spi->close();
		closeGPIO(1);
		_stopped = true;
		IPhysicalInterface::stopListening();
	}
	catch(const std::exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
}

void TiCc1100::endSending()
{
	try
	{
		sendCommandStrobe(CommandStrobes::Enum::SIDLE);
		sendCommandStrobe(CommandStrobes::Enum::SFRX);
		sendCommandStrobe(CommandStrobes::Enum::SRX);
		_sending = false;
		_lastPacketSent = BaseLib::HelperFunctions::getTime();
	}
	catch(const std::exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
}

void TiCc1100::mainThread()
{
    try
    {
		int32_t pollResult;
		int32_t bytesRead;
		std::vector<char> readBuffer({'0'});

        while(!_stopCallbackThread)
        {
        	try
        	{
				if(_stopped)
				{
					std::this_thread::sleep_for(std::chrono::milliseconds(200));
					continue;
				}
				if(!_stopCallbackThread && (!_spi->isOpen() || _gpioDescriptors[1]->descriptor == -1))
				{
					_out.printError("Connection to TI CC1101 closed unexpectedly... Trying to reconnect...");
					_stopped = true; //Set to true, so that sendPacket aborts
					if(_sending)
					{
						std::this_thread::sleep_for(std::chrono::milliseconds(2000));
						_sending = false;
					}
					_txMutex.unlock(); //Make sure _txMutex is unlocked

					initDevice();
					closeGPIO(1);
					std::this_thread::sleep_for(std::chrono::milliseconds(1000));
					openGPIO(1, true);
					_stopped = false;
					continue;
				}

				pollfd pollstruct {
					(int)_gpioDescriptors[1]->descriptor,
					(short)(POLLPRI | POLLERR),
					(short)0
				};

				pollResult = poll(&pollstruct, 1, 100);
				if(pollResult > 0)
				{
					if(lseek(_gpioDescriptors[1]->descriptor, 0, SEEK_SET) == -1) throw BaseLib::Exception("Could not poll gpio: " + std::string(strerror(errno)));
					bytesRead = read(_gpioDescriptors[1]->descriptor, &readBuffer[0], 1);
					if(!bytesRead) continue;
					if(readBuffer.at(0) == 0x30)
					{
						if(!_sending) _txMutex.try_lock(); //We are receiving, don't send now
						continue; //Packet is being received. Wait for GDO high
					}
					if(_sending) endSending();
					else
					{
						//TODO: Include CULTX recognition
						std::shared_ptr<MyPacket> packet;
						if(crcOK())
						{
							uint8_t firstByte = readRegister(Registers::Enum::FIFO);
							std::vector<uint8_t> packetBytes = readRegisters(Registers::Enum::FIFO, firstByte + 1); //Read packet + RSSI
							packetBytes[0] = firstByte;
							if(packetBytes.size() > 100)
							{
								if(!_firstPacket)
								{
									_out.printWarning("Warning: Too large packet received: " + BaseLib::HelperFunctions::getHexString(packetBytes));
									_spi->close();
									_txMutex.unlock();
									continue;
								}
							}
							else if(!packetBytes.empty())
							{
								//packet.reset(new MyPacket(std::string(packetBytes.data(), packetBytes.size()), true, BaseLib::HelperFunctions::getTime()));
								_out.printInfo("Debug: Received: " + BaseLib::HelperFunctions::getHexString(packetBytes));
							}
							else if(!_firstPacket)
							{
								_out.printWarning("Warning: Too small packet received: " + BaseLib::HelperFunctions::getHexString(packetBytes));
								_spi->close();
								_txMutex.unlock();
								continue;
							}
						}
						else _out.printDebug("Debug: Intertechno packet received, but CRC failed.");
						if(!_sendingPending)
						{
							sendCommandStrobe(CommandStrobes::Enum::SFRX);
							sendCommandStrobe(CommandStrobes::Enum::SRX);
						}
						if(packet)
						{
							if(_firstPacket) _firstPacket = false;
							else raisePacketReceived(packet);
						}
					}
					_txMutex.unlock(); //Packet sent or received, now we can send again
				}
				else if(pollResult < 0)
				{
					_txMutex.unlock();
					_out.printError("Error: Could not poll gpio: " + std::string(strerror(errno)) + ". Reopening...");
					closeGPIO(1);
					std::this_thread::sleep_for(std::chrono::milliseconds(1000));
					openGPIO(1, true);
				}
				//pollResult == 0 is timeout
			}
			catch(const std::exception& ex)
			{
				_txMutex.unlock();
				_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
			}
        }
    }
    catch(const std::exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    _txMutex.unlock();
}
}
#endif
