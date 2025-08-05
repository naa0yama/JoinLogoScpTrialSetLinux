TARGET    = join_logo_scp
SRCS      = JlsTop.cpp
SRCS      += JlsIF.cpp
SRCS      += JlsScript.cpp JlsAutoScript.cpp JlsDataset.cpp
SRCS      += JlsCmdSet.cpp JlsCmdArg.cpp JlsReformData.cpp JlsAutoReform.cpp
SRCS      += JlsScrGlobal.cpp JlsScrMem.cpp JlsScrReg.cpp
SRCS      += JlsScrFuncReg.cpp JlsScrFuncList.cpp JlsScriptDecode.cpp
SRCS      += JlsScriptLimit.cpp JlsScriptLimVar.cpp JlsScriptState.cpp
SRCS      += CnvStrTime.cpp LocalEtc.cpp
OBJS      = $(SRCS:.cpp=.o)
PREFIX    = /usr/local
BINDIR    = $(PREFIX)/bin
CXX       = g++
#CXX       = clang++
CXXFLAGS  = -Wall -std=gnu++17
LIBS      = -static-libgcc -static-libstdc++ -static

.PHONY: all clean
all: clean $(OBJS) $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS)

$(OBJS): $(SRCS)
	$(CXX) $(CXXFLAGS) -c $^

install:
	install -d $(BINDIR)
	install -m 0755 $(TARGET) $(BINDIR)

uninstall:
	$(RM) $(BINDIR)/$(TARGET)

clean:
	$(RM) $(OBJS) $(TARGET)
