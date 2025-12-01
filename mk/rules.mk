# Pattern rule generator. 
define BUILD_RULES

$1:
	mkdir -p $$@

$1/%.o: $(SRC_DIR)/%.cpp | $1
	$2 $3 -c $$< -o $$@

endef

$(eval $(call BUILD_RULES,$(BUILD_HOST),$(HOST_CXX),$(HOST_CXXFLAGS)))
$(eval $(call BUILD_RULES,$(BUILD_PI),$(PI_CXX),$(PI_CXXFLAGS)))