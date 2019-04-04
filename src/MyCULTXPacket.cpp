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

#include "MyCULTXPacket.h"

#include "GD.h"
#include "IPacketVisitor.h"

namespace MyFamily
{
MyCULTXPacket::MyCULTXPacket()
{
}

MyCULTXPacket::MyCULTXPacket(std::string& rawPacket)
{
	_timeReceived = BaseLib::HelperFunctions::getTime();
	_packet = rawPacket;

	std::string rawString = _packet.substr(1);
	auto packetVector = GD::bl->hf.getUBinary(rawString);
	//_senderAddress = BaseLib::BitReaderWriter::getPosition8(packetVector, 11, 4);
	_senderAddress = (  ((BaseLib::BitReaderWriter::getPosition8(packetVector, 8, 4))<<3) + ((BaseLib::BitReaderWriter::getPosition8(packetVector, 12, 4))>>1));
	_type = BaseLib::BitReaderWriter::getPosition8(packetVector, 4, 4);
	_rssi = 0;



	// 1010 0000 1111 1101 0111 0010
	// A		0	1	 0	  7	   8		378C16
	// 1010 0000 0001 0000 0111 1000
	// 0	4	 8	  12   16	20
	auto value_ten = BaseLib::BitReaderWriter::getPosition8(packetVector, 16, 4);
	auto value_one = BaseLib::BitReaderWriter::getPosition8(packetVector, 20, 4);
	auto value_dotone = BaseLib::BitReaderWriter::getPosition8(packetVector, 24, 4);

	float rawvalue = value_ten*10;
	rawvalue += value_one;
	rawvalue += value_dotone*0.1;

	// Temp adjusted by -50
	if(_type==0) rawvalue -= 50;

	_payload = std::to_string(rawvalue);

}

bool MyCULTXPacket::acceptVisitor(const std::string& senderId, const std::shared_ptr<IPacketVisitor>& visitor)
{
    return visitor->visitPacket(senderId, shared_from_this());
}

MyCULTXPacket::MyCULTXPacket(int32_t senderAddress, std::string& payload) : _payload(payload)
{
	_senderAddress = senderAddress;
}

MyCULTXPacket::~MyCULTXPacket()
{
	_payload.clear();
}

std::string MyCULTXPacket::hexString()
{
	try
	{
		if(!_packet.empty()) return _packet;
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
