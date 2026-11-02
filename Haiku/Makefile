CXX ?= g++
CXXFLAGS ?= -O2 -std=c++17 -Wall -Wextra
CPPFLAGS += -I/boot/system/develop/headers/private/shared
LDLIBS += -lshared -lcurl -ltranslation -ltracker -lbe
TARGET = Wallhaven
OBJECTS = Main.o Backend.o DesktopIntegration.o

PREFIX ?= /boot/home/config/non-packaged
DESTDIR ?=

all: $(TARGET)

$(TARGET): $(OBJECTS) Wallhaven.rdef
	$(CXX) $(CXXFLAGS) $(OBJECTS) $(LDLIBS) -o $@
	rc -o Wallhaven.rsrc Wallhaven.rdef
	xres -o $@ Wallhaven.rsrc
	mimeset -f $@

%.o: %.cpp Backend.h DesktopIntegration.h
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

install-staged: $(TARGET)
	install -d "$(DESTDIR)$(PREFIX)/apps/Wallhaven" "$(DESTDIR)$(PREFIX)/documentation/packages/wallhaven"
	install -m 755 $(TARGET) "$(DESTDIR)$(PREFIX)/apps/Wallhaven/Wallhaven"
	install -m 644 README.md README.en.md CHANGELOG.md LICENSE THIRD_PARTY_NOTICES.txt "$(DESTDIR)$(PREFIX)/documentation/packages/wallhaven/"

install: $(TARGET)
	mkdir -p $(HOME)/config/non-packaged/apps/Wallhaven
	cp $(TARGET) $(HOME)/config/non-packaged/apps/Wallhaven/Wallhaven
	cp README.md $(HOME)/config/non-packaged/apps/Wallhaven/README.md
	mimeset -f $(HOME)/config/non-packaged/apps/Wallhaven/Wallhaven
	mkdir -p $(HOME)/config/settings/deskbar/menu/Applications
	ln -sf $(HOME)/config/non-packaged/apps/Wallhaven/Wallhaven $(HOME)/config/settings/deskbar/menu/Applications/Wallhaven
	ln -sf $(HOME)/config/non-packaged/apps/Wallhaven/Wallhaven $(HOME)/Desktop/Wallhaven
	copyattr -n BEOS:M:STD_ICON $(HOME)/config/non-packaged/apps/Wallhaven/Wallhaven $(HOME)/Desktop/Wallhaven
	copyattr -n BEOS:L:STD_ICON $(HOME)/config/non-packaged/apps/Wallhaven/Wallhaven $(HOME)/Desktop/Wallhaven
	copyattr -n BEOS:M:STD_ICON $(HOME)/config/non-packaged/apps/Wallhaven/Wallhaven $(HOME)/config/settings/deskbar/menu/Applications/Wallhaven
	copyattr -n BEOS:L:STD_ICON $(HOME)/config/non-packaged/apps/Wallhaven/Wallhaven $(HOME)/config/settings/deskbar/menu/Applications/Wallhaven

BackendTests: tests/BackendTests.cpp Backend.cpp Backend.h
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -I. tests/BackendTests.cpp Backend.cpp -lshared -lcurl -lbe -o $@

RequestTests: tests/RequestTests.cpp Backend.cpp Backend.h
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -I. tests/RequestTests.cpp -lshared -lcurl -lbe -o $@

test: BackendTests RequestTests
	./BackendTests
	./RequestTests

test-live: BackendTests
	./BackendTests --live

clean:
	rm -f $(TARGET) $(OBJECTS) Wallhaven.rsrc BackendTests RequestTests

.PHONY: all install install-staged clean test test-live
