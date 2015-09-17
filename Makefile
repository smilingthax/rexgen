SOURCES=re-ast.cpp re-builder.cpp re-dump.cpp test-ast.cpp test-deriv.cpp
EXEC=test-ast test-deriv parser-ast
PACKAGES=

CPPFLAGS=-O3 -funroll-all-loops -finline-functions -Wall
CFLAGS=-std=c99
LDFLAGS=
CXXFLAGS=-std=c++14

PKG_CONFIG:=pkg-config

ifneq "$(PACKAGES)" ""
  CPPFLAGS+=$(shell $(PKG_CONFIG) --cflags $(PACKAGES))
  LDFLAGS+=$(shell $(PKG_CONFIG) --libs $(PACKAGES))
endif

OBJECTS=$(patsubst %.c,$(PREFIX)%$(SUFFIX).o,\
        $(patsubst %.cpp,$(PREFIX)%$(SUFFIX).o,\
$(SOURCES)))
DEPENDS=$(patsubst %.c,$(PREFIX)%$(SUFFIX).d,\
        $(patsubst %.cpp,$(PREFIX)%$(SUFFIX).d,\
        $(filter-out %.o,""\
$(SOURCES))))

.PHONY: all clean
all: $(EXEC)
ifneq "$(MAKECMDGOALS)" "clean"
  -include $(DEPENDS)
endif

clean:
	rm -f $(EXEC) $(OBJECTS) $(DEPENDS)

%.d: %.c
	@$(CC) $(CPPFLAGS) -MM -MT"$@" -MT"$*.o" -o $@ $<  2> /dev/null

%.d: %.cpp
	@$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MM -MT"$@" -MT"$*.o" -o $@ $<  2> /dev/null

#$(EXEC): $(OBJECTS)
#	$(CXX) -o $@ $^ $(LDFLAGS)

test-ast: test-ast.o re-ast.o re-builder.o re-dump.o
	$(CXX) -o $@ $^ $(LDFLAGS)  $(CPPFLAGS) $(CXXFLAGS)

parser-ast: stringparser.cpp parser-ast.cpp re-ast.o re-builder.o  re-dump.o
	$(CXX) -o $@ $^ $(LDFLAGS)  $(CPPFLAGS) $(CXXFLAGS)

test-deriv: stringparser.cpp test-deriv.o re-ast.o re-builder.o  re-dump.o
	$(CXX) -o $@ $^ $(LDFLAGS)  $(CPPFLAGS) $(CXXFLAGS)

