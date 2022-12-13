include config.mak

vpath %.cpp $(SRCDIR)

OBJCS   = $(SRCCS:%.c=%.c.o)
OBJS    = $(SRCS:%.cpp=%.cpp.o)

all: $(PROGRAM)

$(PROGRAM): .depend $(OBJS) $(OBJCS) 
	$(LD) $(OBJS) $(OBJCS)  $(LDFLAGS) -o $(PROGRAM)

%_sse2.cpp.o: %_sse2.cpp .depend
	$(CXX) -c $(CXXFLAGS) -msse2 -o $@ $<

%_ssse3.cpp.o: %_ssse3.cpp .depend
	$(CXX) -c $(CXXFLAGS) -mssse3 -o $@ $<

%_sse41.cpp.o: %_sse41.cpp .depend
	$(CXX) -c $(CXXFLAGS) -msse4.1 -o $@ $<

%_avx.cpp.o: %_avx.cpp .depend
	$(CXX) -c $(CXXFLAGS) -mavx -mpopcnt -o $@ $<

%_avx2.cpp.o: %_avx2.cpp .depend
	$(CXX) -c $(CXXFLAGS) -mavx2 -mpopcnt -mbmi -mbmi2 -o $@ $<

%_avx512bw.cpp.o: %_avx512bw.cpp .depend
	$(CXX) -c $(CXXFLAGS) -mavx512f -mavx512bw -mpopcnt -mbmi -mbmi2 -o $@ $<

%.c.o: %.c .depend
	$(CC) -c $(CCFLAGS) -o $@ $<

%.cpp.o: %.cpp .depend
	$(CXX) -c $(CXXFLAGS) -o $@ $<
	
.depend: config.mak
	@rm -f .depend
	@echo 'generate .depend...'
	@$(foreach SRC, $(SRCS:%=$(SRCDIR)/%), $(CXX) $(SRC) $(CXXFLAGS) -g0 -MT $(SRC:$(SRCDIR)/%.cpp=%.cpp.o) -MM >> .depend;)
	@$(foreach SRC, $(SRCCUS:%=$(SRCDIR)/%), $(NVCC) $(SRC) $(NVCCFLAGS) -MT $(SRC:$(SRCDIR)/%.cu=%.o) -MM >> .depend;)
	
ifneq ($(wildcard .depend),)
include .depend
endif

clean:
	rm -f $(OBJS) $(OBJCS) $(PROGRAM) .depend

distclean: clean
	rm -f config.mak checkhwenccore/rgy_config.h

install: all
	install -d $(PREFIX)/bin
	install -m 755 $(PROGRAM) $(PREFIX)/bin

uninstall:
	rm -f $(PREFIX)/bin/$(PROGRAM)

config.mak:
	./configure
