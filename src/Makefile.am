AUTOMAKE_OPTIONS = subdir-objects

AM_CPPFLAGS = -Wall -std=c++11 -DFORTIFY_SOURCE=2 -DGCRYPT_NO_DEPRECATED
AM_LDFLAGS = -Wl,-rpath=/lib/homegear -Wl,-rpath=/usr/lib/homegear -Wl,-rpath=/usr/local/lib/homegear
LIBS += -Wl,-Bdynamic

libdir = $(localstatedir)/lib/homegear/modules
lib_LTLIBRARIES = mod_intertechno.la
mod_intertechno_la_SOURCES = MyFamily.cpp MyFamily.h MyPacket.cpp MyPacket.h MyPeer.cpp MyPeer.h Factory.cpp Factory.h GD.cpp GD.h MyCentral.cpp MyCentral.h Interfaces.h Interfaces.cpp PhysicalInterfaces/IIntertechnoInterface.h PhysicalInterfaces/IIntertechnoInterface.cpp PhysicalInterfaces/Cul.h PhysicalInterfaces/Cul.cpp PhysicalInterfaces/Coc.h PhysicalInterfaces/Coc.cpp PhysicalInterfaces/Cunx.h PhysicalInterfaces/Cunx.cpp PhysicalInterfaces/TiCc1100.h PhysicalInterfaces/TiCc1100.cpp
mod_intertechno_la_LDFLAGS =-module -avoid-version -shared
install-exec-hook:
	rm -f $(DESTDIR)$(libdir)/mod_intertechno.la
