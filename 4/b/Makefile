DEBUG_FLAGS=-W -Wall -Wextra -pedantic -Wno-narrowing -Werror -Wfloat-equal -Wformat=2 -Wconversion -Wlogical-op -fstack-protector-all -D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC -fsanitize=address -fsanitize=undefined -fno-sanitize-recover -Wsuggest-attribute=const -Wsuggest-attribute=pure
RELEASE_FLAGS=-O2

CXXFLAGS=$(FLAGS) -std=c++11
LDFLAGS=-lm rpa_linux/libraries/zeus_local.o rpa_linux/libraries/message_internal.o

CSOURCES=$(wildcard *.c)
CXXSOURCES=$(wildcard *.cc)
JAVASOURCES=$(wildcard *.java)
PASSOURCES=$(wildcard *.pas)

COBJECTS_RELEASE=$(patsubst %.c,%.release,$(CSOURCES))
COBJECTS_DEBUG=$(patsubst %.c,%.debug,$(CSOURCES))

CXXOBJECTS_RELEASE=$(patsubst %.cc,%.release,$(CXXSOURCES))
CXXOBJECTS_DEBUG=$(patsubst %.cc,%.debug,$(CXXSOURCES))

JAVAOBJECTS=$(patsubst %.java,%,$(JAVASOURCES))
PASOBJECTS=$(patsubst %.pas,%,$(PASSOURCES))

OBJECTS=$(COBJECTS_RELEASE) $(COBJECTS_DEBUG) $(CXXOBJECTS_RELEASE) $(CXXOBJECTS_DEBUG) $(JAVAOBJECTS) $(PASOBJECTS)

all: $(COBJECTS_RELEASE) $(COBJECTS_DEBUG) $(CXXOBJECTS_RELEASE) $(CXXOBJECTS_DEBUG) $(JAVAOBJECTS) $(PASOBJECTS)

%.release: %.c
	$(CC) $(CFLAGS) $(RELEASE_FLAGS) $< -o $@ $(LDFLAGS)

%.debug: %.c
	$(CC) $(CFLAGS) $(DEBUG_FLAGS) $< -o $@ $(LDFLAGS)

%.release: %.cc
	$(CXX) $(CXXFLAGS) $(RELEASE_FLAGS) $< -o $@ $(LDFLAGS)

%.debug: %.cc
	$(CXX) $(CXXFLAGS) $(DEBUG_FLAGS) $< -o $@ $(LDFLAGS)

%: %.java
	javac $<

%: %.pas
	ppcx64 -O2 -XS -Xt $<

clean:
	$(RM) $(OBJECTS)

.PHONY: clean
