LV2_DESTDIR=$(DESTDIR)/usr/lib/lv2

all:
	$(MAKE) -C mod-compressor/
	$(MAKE) -C mod-compressor-advanced/
	$(MAKE) -C mod-noisegate/
	$(MAKE) -C mod-noisegate-advanced/

install: all
	install -d $(LV2_DESTDIR)
	cp -r mod-compressor/               $(LV2_DESTDIR)
	cp -r mod-compressor-advanced/      $(LV2_DESTDIR)
	cp -r mod-noisegate/                $(LV2_DESTDIR)
	cp -r mod-noisegate-advanced/       $(LV2_DESTDIR)

clean:
	$(MAKE) clean -C mod-compressor/
	$(MAKE) clean -C mod-compressor-advanced/
	$(MAKE) clean -C mod-noisegate/
	$(MAKE) clean -C mod-noisegate-advanced/
