# Object list per architecture
OBJ_HOST := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_HOST)/%.o,$(SRC))
OBJ_PI   := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_PI)/%.o,$(SRC))

OBJ_HOST_NOMAIN := $(filter-out $(BUILD_HOST)/main.o,$(OBJ_HOST))
OBJ_PI_NOMAIN := $(filter-out $(BUILD_PI)/main.o,$(OBJ_PI))

# Link final binaries
$(TARGET_HOST): $(OBJ_HOST)
	$(HOST_CXX) -o $@ $^ $(HOST_LDFLAGS)

$(TARGET_PI): $(OBJ_PI)
	$(PI_CXX) -o $@ $^ $(PI_LDFLAGS)

.PHONY: host pi deploy run clean stop
host: $(TARGET_HOST)
pi: $(TARGET_PI)

deploy: stop $(TARGET_PI)
	scp $(TARGET_PI) $(PI_USER)@$(PI_HOST):/tmp/$(TARGET_PI).new
	ssh $(PI_USER)@$(PI_HOST) "mv /tmp/$(TARGET_PI).new $(PI_PATH)/$(TARGET_PI) && chmod +x $(PI_PATH)/$(TARGET_PI)"

run: deploy
	ssh $(PI_USER)@$(PI_HOST) "$(PI_PATH)/$(TARGET_PI) --no-block"

stop:
	ssh $(PI_USER)@$(PI_HOST) 'pids=$$(pidof $(TARGET_PI) 2>/dev/null || true); if [ -n "$$pids" ]; then kill $$pids; fi'

clean:
	rm -rf $(BUILD_PI) $(BUILD_HOST)
	rm -f $(TARGET_PI) $(TARGET_HOST) mini_client mini_client_pi