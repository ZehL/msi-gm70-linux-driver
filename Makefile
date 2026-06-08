CC = gcc
CFLAGS = -O2
LIBS = -lm
TARGET = msi_gm70_driver
PREFIX = /usr/local

all: $(TARGET)

$(TARGET): msi_gm70_driver.c
	$(CC) $(CFLAGS) -o $(TARGET) msi_gm70_driver.c $(LIBS)

install: all
	install -D -m 755 $(TARGET) $(DESTDIR)$(PREFIX)/bin/$(TARGET)
	install -D -m 644 msi-mouse-driver.service $(DESTDIR)/etc/systemd/system/msi-mouse-driver.service
	systemctl daemon-reload

uninstall:
	systemctl stop msi-mouse-driver.service || true
	systemctl disable msi-mouse-driver.service || true
	rm -f $(DESTDIR)$(PREFIX)/bin/$(TARGET)
	rm -f $(DESTDIR)/etc/systemd/system/msi-mouse-driver.service
	systemctl daemon-reload

clean:
	rm -f $(TARGET)

.PHONY: all install uninstall clean
