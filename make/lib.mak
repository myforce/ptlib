
ifeq ($(P_SHAREDLIB),1)

ifndef MAJOR_VERSION
MAJOR_VERSION	= 1
endif

ifndef MINOR_VERSION
MINOR_VERSION	= 0
endif

ifndef BUILD_NUMBER
BUILD_NUMBER	= 0
endif

LIBNAME_MAJ		= $(LIB_BASENAME).$(MAJOR_VERSION)
LIBNAME_MIN		= $(LIBNAME_MAJ).$(MINOR_VERSION)
LIBNAME_PAT		= $(LIBNAME_MIN).$(BUILD_NUMBER)

$(LIBDIR)/$(LIB_BASENAME): $(LIBDIR)/$(LIBNAME_PAT)
	@rm -f $(LIBDIR)/$(LIB_BASENAME)
	ln -s $(LIBDIR)/$(LIBNAME_PAT) $(LIBDIR)/$(LIB_BASENAME)
	@rm -f $(LIBDIR)/$(LIBNAME_MAJ)
	ln -s $(LIBDIR)/$(LIBNAME_PAT) $(LIBDIR)/$(LIBNAME_MAJ)
	@rm -f $(LIBDIR)/$(LIBNAME_MIN)
	ln -s $(LIBDIR)/$(LIBNAME_PAT) $(LIBDIR)/$(LIBNAME_MIN)

$(LIBDIR)/$(LIBNAME_PAT): $(OBJS)
	@if [ ! -d $(LIBDIR) ] ; then mkdir $(LIBDIR) ; fi
	gcc -shared -Wl,-soname,$(LIB_BASENAME).1 -o $(LIBDIR)/$(LIBNAME_PAT) $(OBJS)

CLEAN_FILES += $(LIBDIR)/$(LIBNAME_PAT) $(LIBDIR)/$(LIB_BASENAME) $(LIBDIR)/$(LIBNAME_MAJ) $(LIBDIR)/$(LIBNAME_MIN)

install: $(LIBDIR)/$(LIBNAME_PAT)
	$(INSTALL) $(LIBDIR)/$(LIBNAME_PAT) $(INSTALLLIB_DIR)/$(LIBNAME_PAT)
	ln -s $(INSTALLLIB_DIR)/$(LIBNAME_PAT) $(INSTALLLIB_DIR)/$(LIB_BASENAME)
	ln -s $(INSTALLLIB_DIR)/$(LIBNAME_PAT) $(INSTALLLIB_DIR)/$(LIBNAME_MAJ)
	ln -s $(INSTALLLIB_DIR)/$(LIBNAME_PAT) $(INSTALLLIB_DIR)/$(LIBNAME_MIN)

else

$(LIBDIR)/$(LIB_BASENAME): $(OBJS)
	@if [ ! -d $(LIBDIR) ] ; then mkdir $(LIBDIR) ; fi
ifdef RANLIB
	ar rc $(LIBDIR)/$(LIB_BASENAME) $(OBJS)
	ranlib $(LIBDIR)/$(LIB_BASENAME)
else
	ar rcs $(LIBDIR)/$(LIB_BASENAME) $(OBJS)
endif

CLEAN_FILES += $(LIBDIR)/$(LIB_BASENAME)

endif

