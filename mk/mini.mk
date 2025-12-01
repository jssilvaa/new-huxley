MINI_SRC := mini_client.cpp

.PHONY: mini_client mini_client_pi deploy-test-pi run-test-pi

mini_client: $(OBJ_HOST_NOMAIN) $(MINI_SRC)
	$(HOST_CXX) $(HOST_CXXFLAGS) $^ -o $@ $(HOST_LDFLAGS)

mini_client_pi: $(OBJ_PI_NOMAIN) $(MINI_SRC)
	$(PI_CXX) $(PI_CXXFLAGS) $^ -o $@ $(PI_LDFLAGS)

deploy-test-pi: mini_client_pi
	scp mini_client_pi $(PI_USER)@$(PI_HOST):$(PI_PATH)
	ssh $(PI_USER)@$(PI_HOST) "chmod +x $(PI_PATH)/mini_client_pi"

run-test-pi: deploy-test-pi
	ssh $(PI_USER)@$(PI_HOST) "$(PI_PATH)/mini_client_pi"