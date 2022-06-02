LV2_DESTDIR=$(DESTDIR)/usr/lib/lv2

all:
	$(MAKE) -C mod-compressor/
	$(MAKE) -C mod-compressor-advanced/
	$(MAKE) -C mod-noisegate/
	$(MAKE) -C mod-noisegate-advanced/

install: all
	install -d $(LV2_DESTDIR)
	cp -r mod-compressor/system-compressor.lv2              $(LV2_DESTDIR)
	cp -r mod-compressor/advanced-compressor.lv2            $(LV2_DESTDIR)
	cp -r mod-noisegate/system-noisegate.lv2                $(LV2_DESTDIR)
	cp -r mod-noisegate/advanced-noisegate.lv2              $(LV2_DESTDIR)

clean:
	$(MAKE) clean -C mod-compressor/
	$(MAKE) clean -C mod-compressor-advanced/
	$(MAKE) clean -C mod-noisegate/
	$(MAKE) clean -C mod-noisegate-advanced/
