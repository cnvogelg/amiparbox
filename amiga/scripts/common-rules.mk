info:
	@echo "--- $(CONFIG) $(FLAVOR) $(COMPILER) ---"

all: info $(PROGRAMS) $(DEVICES)

clean:
	$(H)rm -rf $(BUILD_DIR)

clean-all:
	$(H)rm -rf $(BUILD_BASE_DIR) $(DIST_BASE_DIR)

size: $(SIZE_RULES)

# distribution
dist: info $(DIST_FILES) size

dist-all:
	@for a in $(ALL_FLAVORS) ; do \
		for b in $(ALL_CONFIGS) ; do \
			for c in $(ALL_COMPILERS); do \
				$(MAKE) FLAVOR=$$a CONFIG=$$b COMPILER=$$c dist || exit 1 ; \
			done \
		done \
	done


# install files
INSTALL_DIR ?= ../install

install: $(BIN_FILES)
	@if [ ! -d "$(INSTALL_DIR)" ]; then \
		echo "No INSTALL_DIR='$(INSTALL_DIR)' found!" ; \
		exit 1 ; \
	fi
	@echo "  INSTALL  $(BIN_FILES)"
	@cp $(BIN_FILES) $(INSTALL_DIR)/

# compile
$(OBJ_DIR)/%.o : %.c
	@echo "  CC   $<"
	$(H)$(CC) -c $(CFLAGS) $< -o $@

# assemble
$(OBJ_DIR)/%.o : %.s
	@echo "  ASM  $<"
	$(H)$(AS) $(ASFLAGS) $< -o $@
