TARGET	=	myprog
CXXFILES	=	prog2 printer

BITS	?=	64
CXX	=	g++
LD	=	g++
CXXFLAGS	+=	-MMD -O3 m$(BITS) -ggdb -std=c++17 -Wall -Werror -Wextra -march=native
LDXFLAGS	+=	-m$(BITS)

ODIR	:=	./obj$(BITS)
out_folder	:=	$(shell mkdir -p $(ODIR))
EXEFILES	=	$(patsubst %, $(ODIR)/%.exe, $(TARGET))
OFILES	=	$(patsubst %, $(ODIR)/%.o, $(CXXFILES))
DFILES	=	$(patsubst %, $(ODIR)/%.d, $(CXXFILES))

.DEFAULT_GOAL	=	all
.PRECIOUS:	$(OFILES)
.PHONY:	all clean

all:	$(EXEFILES)

$(ODIR)/%.o: %.cc
	@echo "[CXX] $< --> $@"
	@$(CXX) $< -o $@ -c $(CXXFLAGS)

$(ODIR)/%.exe: $(OFILES)
	@echo "[LD] $^ --> $@"
	@$(LD) $^ -o $@ $(LDFLAGS)

clean:
	@echo Cleaning up...
	@rm -rf $(ODIR)

-include $(DFILES)
