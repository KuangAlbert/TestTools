# Created by Xie Qihuai 2015-09-15
all:

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

testing-%: testing-%.o
	$(CC)  $(LDFLAGS) $< -o $@

testing-tuner: testing-tuner.o tef701x.o patch_v101.o patch_v102.o
	$(CC)  $(LDFLAGS) $^ -o $@

all: testing-ipod testing-tuner

install:
	@if [ 'x${DESTDIR}' = x ]; then \
	    echo "error: DESTDIR not export"; \
	    exit 1;                 \
	 fi
	install -d $(DESTDIR)/usr/bin
	install -d $(DESTDIR)/etc/rc3.d
	install -d $(DESTDIR)/etc/rc5.d
	install -d $(DESTDIR)/etc/init.d
	install add-dev $(DESTDIR)/etc/init.d/add-dev
	ln -sf ../init.d/add-dev $(DESTDIR)/etc/rc3.d/S01add-dev
	ln -sf ../init.d/add-dev $(DESTDIR)/etc/rc5.d/S01add-dev
	install testing-ipod  $(DESTDIR)/usr/bin/testing-ipod
	install testing-tuner  $(DESTDIR)/usr/bin/testing-tuner

clean:
	rm -rf *.o
	rm -rf testing-ipod
	rm -rf testing-tuner
