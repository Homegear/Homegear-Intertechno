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

#ifndef CUNX_H
#define CUNX_H

#include <homegear-base/BaseLib.h>
#include "IIntertechnoInterface.h"

namespace MyFamily
{

class Cunx : public IIntertechnoInterface
{
    public:
		Cunx(std::shared_ptr<BaseLib::Systems::PhysicalInterfaceSettings> settings);
        virtual ~Cunx();
        void startListening();
        void stopListening();
        virtual bool isOpen() { return _socket->Connected(); }
		
		void sendPacket(std::shared_ptr<BaseLib::Systems::Packet> packet);
    protected:
        BaseLib::Output _out;
        std::string _port;
        std::unique_ptr<C1Net::TcpSocket> _socket;
        std::string stackPrefix;

        void reconnect();
        void processData(std::vector<uint8_t>& data);
        void send(std::string data);
        std::string readFromDevice();
        void listen();
    private:
};

}
#endif
