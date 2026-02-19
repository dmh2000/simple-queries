LANGS = rust go python typescript cpp

.PHONY: all test clean \
        $(addprefix build-,$(LANGS)) \
        $(addprefix test-,$(LANGS)) \
        $(addprefix clean-,$(LANGS))

all: $(addprefix build-,$(LANGS))
test: $(addprefix test-,$(LANGS))
clean: $(addprefix clean-,$(LANGS))

define LANG_RULES
build-$(1):
	@echo "=== Building $(1) ==="
	$$(MAKE) -C $(1)

test-$(1):
	@echo "=== Testing $(1) ==="
	$$(MAKE) -C $(1) test

clean-$(1):
	$$(MAKE) -C $(1) clean
endef

$(foreach lang,$(LANGS),$(eval $(call LANG_RULES,$(lang))))
