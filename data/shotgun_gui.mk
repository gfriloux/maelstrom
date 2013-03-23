EXTRA_DIST += \
data/shotgun-128x128.png \
data/shotgun-64x64.png \
data/shotgun.desktop \
data/shotgun.png

shotgun_desktopdir = $(datadir)/applications
shotgun_desktop_DATA = data/shotgun.desktop

pixmapdir = $(datadir)/icons/hicolor/48x48/apps
pixmap_DATA = data/shotgun.png

pixmap64dir = $(datadir)/icons/hicolor/64x64/apps
pixmap64_DATA = data/shotgun-64x64.png

pixmap128dir = $(datadir)/icons/hicolor/128x128/apps
pixmap128_DATA = data/shotgun-128x128.png

rename-icons:
	@cd $(DESTDIR)$(datadir)/icons/hicolor/64x64/apps && \
            mv shotgun-64x64.png shotgun.png
	@cd $(DESTDIR)$(datadir)/icons/hicolor/128x128/apps && \
            mv shotgun-128x128.png shotgun.png

remove-icons:
	rm -f $(DESTDIR)$(datadir)/icons/hicolor/64x64/apps/shotgun.png
	rm -f $(DESTDIR)$(datadir)/icons/hicolor/128x128/apps/shotgun.png

install-data-hook: rename-icons
uninstall-hook: remove-icons
