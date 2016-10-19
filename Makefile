AR := sdar
AS := sdas8051
CC := sdcc
CPP := sdcc -E
STCGAL := stcgal

OUT := build
MAIN := main

MAIN_C = src/main.c
SOURCES_C = src/xorshift.c
SOURCES_S = 
INCLUDES = include

STCFLAGS := -P stc15 --trim 5000 -o cpu_core_voltage=low -b 115200
CPPFLAGS := -DFOSC=2500000UL
CFLAGS := -mmcs51 --opt-code-size --no-xinit-opt --std-sdcc99 --funsigned-char --stack-auto
LDFLAGS := --out-fmt-ihx --iram-size 256 --xram-size 0 --code-size 8192

uniq = $(if $1,$(firstword $1) $(call uniq,$(filter-out $(firstword $1),$1)))
INCS := $(addprefix -I,$(INCLUDES))
MAIN_OBJ := $(MAIN_C:.c=.rel)
MAIN_OBJ := $(addprefix $(OUT)/,$(MAIN_OBJ))
OBJS := $(MAIN_C:.c=.rel) $(SOURCES_C:.c=.rel) $(SOURCES_S:.s=.rel)
OBJS := $(addprefix $(OUT)/,$(OBJS))
DEPS := $(OBJS:.rel=.d)
DIRS := $(call uniq,$(dir $(OBJS)))

ifneq ($(V),1)
Q := @
endif

.SUFFIXES:

.PHONY: all
all: $(OUT)/$(MAIN).ihx

.PHONY: clean
clean:
	@echo "  CLEAN"
	$(Q)rm -rf $(OUT)

-include $(DEPS)

$(OUT) $(DIRS):
	$(Q)mkdir -p $@

$(OBJS) $(DEPS): | $(DIRS) $(OUT)

$(OUT)/%.d: %.c
	@echo "  DEP $(notdir $@)"
	$(Q)$(CPP) $(INCS) $(CPPFLAGS) -MMD $< | sed "s|^.*:|$(dir $@)&|" >$@

$(OUT)/%.rel: %.c
	@echo "  CC  $(notdir $@)"
	$(Q)$(CC) $(CFLAGS) $(INCS) $(CPPFLAGS) -c -o $@ $<

$(OUT)/%.rel: %.s
	@echo "  AS  $(notdir $@)"
	$(Q)$(AS) -c -o $@ $<

$(OUT)/$(MAIN).lib: $(OBJS)
	@echo "  AR  $(notdir $@)"
	$(Q)$(AR) rcs $@ $^

$(OUT)/$(MAIN).ihx: $(MAIN_OBJ) $(OUT)/$(MAIN).lib
	@echo "  LD  $(notdir $@)"
	$(Q)$(CC) $(MAIN_OBJ) $(OUT)/$(MAIN).lib -o $@ $(LIBS) $(CFLAGS) $(LDFLAGS) 

.PHONY: flash
flash: $(OUT)/$(MAIN).ihx
	$(Q)$(STCGAL) $(STCFLAGS) $<
