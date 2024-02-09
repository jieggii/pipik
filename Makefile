PYTHON_PATH := "/bin/python"
GEN_OUTPUT_FILE := "./src/texts.gen.cpp"

.PHONY: all
all:
	@echo "TARGET           DESCRIPTION"
	@echo "generate-text    generate $(GEN_OUTPUT_FILE) from $(GEN_OUTPUT_FILE)"

.PHONY: help
help: all

.PHONY: generate-texts
generate-texts:
	@$(PYTHON_PATH) ./generate-texts.py --input=texts.json   | tee temp.txt; \
    if [ -s temp.txt ]; then \
        mv temp.txt $(GEN_OUTPUT_FILE); \
    	echo "info: copied generated code to $(GEN_OUTPUT_FILE)"; \
    else \
        rm temp.txt; \
        echo "info: no code was copied to $(GEN_OUTPUT_FILE)"; \
    fi
