EXTRA_DIST += \
src/modules/sawed-off/module.desktop.in \
src/modules/sawed-off/e-module-sawed-off.edc

sawedoff_filesdir = $(MODULE_DIR)/sawed-off
sawedoff_files_DATA = \
src/modules/sawed-off/module.desktop \
src/modules/sawed-off/e-module-sawed-off.edj

pkgdir = $(MODULE_DIR)/sawed-off/$(MODULE_ARCH)
pkg_LTLIBRARIES = src/modules/sawed-off/module.la

src_modules_sawed_off_module_la_SOURCES = \
src/modules/sawed-off/e_mod_main.h \
src/modules/sawed-off/e_mod_main.c \
src/modules/sawed-off/e_mod_config.c

src_modules_sawed_off_module_la_CPPFLAGS = \
-I$(top_srcdir) \
-I$(top_srcdir)/src/bin/shotgun \
-I$(top_srcdir)/src/include/shotgun \
-DMODULE_BUILD=1 \
-DPACKAGE_DATA_DIR=\"$(MODULE_DIR)/sawed-off\" \
@E_CFLAGS@ \
@SHOTGUN_CFLAGS@

src_modules_sawed_off_module_la_LIBADD = \
@E_LIBS@ \
@SHOTGUN_LIBS@

src_modules_sawed_off_module_la_LDFLAGS = -module -avoid-version

src/modules/sawed-off/e-module-sawed-off.edj: src/modules/sawed-off/e-module-sawed-off.edc
	$(edje_cc) $< $@

