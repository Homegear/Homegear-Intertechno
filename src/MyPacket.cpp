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

MyPacket::MyPacket(int32_t senderAddress, std::string& payload) : _payload(payload)
{
	_senderAddress = senderAddress;
}

MyPacket::~MyPacket()
{
	_payload.clear();
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
				_packet.push_back(_senderAddress & (1 << i) ? '1' : '0');
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
