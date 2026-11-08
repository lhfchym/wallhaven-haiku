CXX ?= g++
CXXFLAGS ?= -O2 -std=c++17 -Wall -Wextra
CPPFLAGS += -I/boot/system/develop/headers/private/shared
LDLIBS += -lshared -lcurl -ltranslation -ltracker -llocalestub -lbe
TARGET = Wallhaven
OBJECTS = Main.o Backend.o DesktopIntegration.o WorkspaceBackground.o Localization.o

PREFIX ?= /boot/home/config/non-packaged
CATKEYS = $(wildcard locales/*.catkeys)
LOCALES = $(filter-out en,$(basename $(notdir $(CATKEYS))))
APP_SIGNATURE = application/x-vnd.lhfchym-Wallhaven

DESTDIR ?=

all: $(TARGET)

$(TARGET): $(OBJECTS) Wallhaven.rdef $(CATKEYS)
	$(CXX) $(CXXFLAGS) $(OBJECTS) $(LDLIBS) -o $@
	rc -o Wallhaven.rsrc Wallhaven.rdef
	xres -o $@ Wallhaven.rsrc
	$(MAKE) bindcatalogs
	mimeset -f $@

%.o: %.cpp Backend.h DesktopIntegration.h Localization.h WorkspaceBackground.h
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

install-staged: $(TARGET)
	install -d "$(DESTDIR)$(PREFIX)/apps/Wallhaven" "$(DESTDIR)$(PREFIX)/documentation/packages/wallhaven"
	install -m 755 $(TARGET) "$(DESTDIR)$(PREFIX)/apps/Wallhaven/Wallhaven"
	install -m 644 README.md README.en.md CHANGELOG.md LICENSE THIRD_PARTY_NOTICES.txt "$(DESTDIR)$(PREFIX)/documentation/packages/wallhaven/"
	install -d "$(DESTDIR)$(PREFIX)/documentation/packages/wallhaven/locales"
	install -m 644 locales/README.md "$(DESTDIR)$(PREFIX)/documentation/packages/wallhaven/locales/"

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

BackendTests: tests/BackendTests.cpp Backend.cpp Backend.h Localization.cpp Localization.h
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -I. tests/BackendTests.cpp Backend.cpp Localization.cpp -lshared -lcurl -llocalestub -lbe -o $@

RequestTests: tests/RequestTests.cpp Backend.cpp Backend.h Localization.cpp Localization.h
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -I. tests/RequestTests.cpp Localization.cpp -lshared -lcurl -llocalestub -lbe -o $@

WorkspaceTests: tests/WorkspaceTests.cpp WorkspaceBackground.cpp WorkspaceBackground.h
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -I. tests/WorkspaceTests.cpp WorkspaceBackground.cpp -lbe -o $@

DesktopTests: tests/DesktopTests.cpp DesktopIntegration.cpp WorkspaceBackground.cpp Localization.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -I. $^ -ltranslation -ltracker -llocalestub -lbe -o $@

CatalogTests: tests/CatalogTests.cpp $(TARGET)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) tests/CatalogTests.cpp -lbe -o $@

catkeys:
	mkdir -p locales
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -DB_COLLECTING_CATKEYS -P -E Main.cpp Backend.cpp DesktopIntegration.cpp > Wallhaven.pre
	collectcatkeys -s $(APP_SIGNATURE) Wallhaven.pre -o locales/en.catkeys
	rm -f Wallhaven.pre

bindcatalogs:
	@set -e; for language in $(LOCALES); do \
		linkcatkeys -o $(TARGET) -s $(APP_SIGNATURE) -tr -l $$language locales/$$language.catkeys; \
	done

test: BackendTests RequestTests WorkspaceTests CatalogTests
	./BackendTests
	./RequestTests
	./WorkspaceTests
	./CatalogTests

test-live: BackendTests
	./BackendTests --live

clean:
	rm -f $(TARGET) $(OBJECTS) Wallhaven.rsrc Wallhaven.pre BackendTests RequestTests WorkspaceTests CatalogTests DesktopTests

.PHONY: all install install-staged clean test test-live catkeys bindcatalogs
