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

#include "MyPacket.h"

#include "GD.h"

namespace MyFamily
{
MyPacket::MyPacket()
{
}

MyPacket::MyPacket(std::string& rawPacket)
{
	_timeReceived = BaseLib::HelperFunctions::getTime();
	_packet = rawPacket.at(0) == 'i' && rawPacket.size() > 3 ? rawPacket.substr(1, rawPacket.size() - 3) : rawPacket;
	_senderAddress = 0;

	std::string rssiString = _packet.substr(_packet.size() - 2, 2);
	int32_t rssiDevice = BaseLib::Math::getNumber(rssiString);
	//1) Read the RSSI status register
	//2) Convert the reading from a hexadecimal
	//number to a decimal number (RSSI_dec)
	//3) If RSSI_dec ≥ 128 then RSSI_dBm =
	//(RSSI_dec - 256)/2 – RSSI_offset
	//4) Else if RSSI_dec < 128 then RSSI_dBm =
	//(RSSI_dec)/2 – RSSI_offset
	if(rssiDevice >= 128) rssiDevice = ((rssiDevice - 256) / 2) - 74;
	else rssiDevice = (rssiDevice / 2) - 74;
	_rssi = (uint8_t)(rssiDevice * -1);

	if(_packet.size() == 8)
	{
		_channel = 0;
		_senderAddress = 0;
		int32_t j = 0;
		for(int32_t i = _packet.size() - 4; i >= 0; i--)
		{
			_senderAddress |= (parseNibbleSmall(_packet.at(i)) << j);
			j += 2;
		}

		_payload = parseNibbleStringSmall(_packet.at(_packet.size() - 3));
	}
	else if(_packet.size() == 18)
	{
		_channel = 0;
		_senderAddress = 0;
		int32_t j = 0;
		for(int32_t i = _packet.size() - 3; i >= (signed)_packet.size() - 4; i--)
		{
			_channel |= (parseNibble(_packet.at(i)) << j);
			j += 2;
		}
		_channel++;

		j = 0;
		for(int32_t i = _packet.size() - 6; i >= 0; i--)
		{
			_senderAddress |= (parseNibble(_packet.at(i)) << j);
			j += 2;
		}

		_payload = parseNibbleString(_packet.at(_packet.size() - 5));
	}
}

MyPacket::MyPacket(int32_t senderAddress, std::string& payload) : _payload(payload)
{
	_senderAddress = senderAddress;
}

MyPacket::~MyPacket()
{
	_payload.clear();
}

uint8_t MyPacket::parseNibble(char nibble)
{
	switch(nibble)
	{
		case '5':
			return 0;
		case '6':
			return 1;
		case '9':
			return 2;
		case 'A':
			return 3;
	}
	return 0;
}

std::string MyPacket::parseNibbleString(char nibble)
{
	switch(nibble)
	{
		case '5':
			return "00";
		case '6':
			return "01";
		case '9':
			return "10";
		case 'A':
			return "11";
	}
	return "00";
}

uint8_t MyPacket::parseNibbleSmall(char nibble)
{
	switch(nibble)
	{
		case '0':
			return 0;
		case '1':
			return 1;
		case '4':
			return 2;
		case '5':
			return 3;
	}
	return 0;
}

std::string MyPacket::parseNibbleStringSmall(char nibble)
{
	switch(nibble)
	{
		case '0':
			return "00";
		case '1':
			return "0F";
		case '4':
			return "F0";
		case '5':
			return "FF";
	}
	return "00";
}

std::string MyPacket::hexString()
{
	try
	{
		if(!_packet.empty()) return _packet;
		if(_senderAddress & 0xFFFFFC00)
		{
			_packet.reserve(32);
			for(int32_t i = 29; i >= 4; i--)
			{
				_packet.push_back(_senderAddress & (1 << i) ? '1' : '0');
			}
			_packet.insert(_packet.end(), _payload.begin(), _payload.end());
			for(int32_t i = 3; i >= 0; i--)
			{
				_packet.push_back(_senderAddress & (1 << i) ? '1' : '0');
			}
		}
		else
		{
			_packet.reserve(12);
			for(int32_t i = 9; i >= 0; i--)
			{
				_packet.push_back(_senderAddress & (1 << i) ? 'F' : '0');
			}
			_packet.insert(_packet.end(), _payload.begin(), _payload.end());
		}
		return _packet;
	}
	catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return "";
}
}
