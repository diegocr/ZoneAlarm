PRGNAME	=	ZoneAlarm$(SYS)
CC	=	m68k-amigaos-gcc
LDFLAGS	=	-nostdlib -noixemul -m68060
DEFS	=	-D__NOLIBBASE__ 
WARNS	=	-W -Wall -Winline 
OPTIM	=	-O3 -funroll-loops
CFLAGS	=	$(LDFLAGS) $(OPTIM) -I. $(DEFS) $(WARNS)
LIBS	=	-lamiga -Wl,-Map,$@.map,--cref

ifdef DEBUG
	CFLAGS += -DDEBUG -g
endif

OBJDIR	= .objs$(SYS)
OBJS	=	\
	$(OBJDIR)/startup.o	\
	$(OBJDIR)/global.o	\
	$(OBJDIR)/debug.o	\
	$(OBJDIR)/ipc.o		\
	$(OBJDIR)/main.o	\
	$(OBJDIR)/palloc.o	\
	$(OBJDIR)/pchs.o	\
	$(OBJDIR)/pch_dos.o	\
	$(OBJDIR)/pch_exec.o	\
	$(OBJDIR)/pch_socket.o	\
	$(OBJDIR)/exit.o	\
	$(OBJDIR)/utils.o	\
	$(OBJDIR)/gpbn.o	\
	$(OBJDIR)/usema.o	\
	$(OBJDIR)/SFPatch.o	\
	$(OBJDIR)/taskreg.o	\
	$(OBJDIR)/za_task.o	\
	$(OBJDIR)/alert.o	\
	$(OBJDIR)/thread.o	\
	$(OBJDIR)/xvs.o		\
	$(OBJDIR)/icon.o

all:	
#	@make $(PRGNAME).nodebug SYS=.nodebug
	@make $(PRGNAME) DEBUG=1

nodebug:
	@make $(PRGNAME).nodebug SYS=.nodebug

$(PRGNAME):	$(OBJDIR) $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LIBS)

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(OBJDIR)/%.o: %.c
	@echo Compiling $@
	@$(CC) $(CFLAGS) -c $< -o $@

