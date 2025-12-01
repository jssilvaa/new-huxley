.PHONY: cli cli-run cli-shell

cli:
	@echo "CLI script located at $(CLI_ENTRY)"
	@echo "Run: $(PYTHON) $(CLI_ENTRY) <host> <port>"

cli-run:
	$(PYTHON) $(CLI_ENTRY) $(CLI_HOST) $(CLI_PORT)

cli-shell:
	@echo "Launch additional shells via: $(PYTHON) $(CLI_ENTRY) <host> <port>"
