EXTRA_DIST += \
src/tests/azy/client.c \
src/tests/azy/server.c \
src/tests/azy/stress_client.c \
src/tests/azy/test.azy \
src/tests/azy/type1.azy \
src/tests/azy/type2.azy \
src/tests/azy/server.pem

check_PROGRAMS =

if BUILD_AZY
include src/tests/azy.mk
endif

