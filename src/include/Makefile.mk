EXTRA_DIST += \
src/include/azy/azy_private.h \
src/include/extras/cdecode.h \
src/include/extras/cencode.h \
src/include/extras/cJSON.h \
src/include/extras/md5.h \
src/include/extras/pugixml.hpp \
src/include/extras/pugiconfig.hpp \
src/include/extras/sha1.h \
src/include/extras/sha1wrap.h \
src/include/shotgun/shotgun_private.h \
src/include/shotgun/shotgun_xml.h

if BUILD_AZY
header_install_HEADERS += src/include/azy/Azy.h
endif

if BUILD_SHOTGUN
header_install_HEADERS += src/include/shotgun/Shotgun.h
endif
