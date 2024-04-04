all: build

build:
	idf.py build

flash:
	-make close
	@usbport=$$(ls /dev/cu.usbmodem* | head -1); \
	if [ -z "$$usbport" ]; then \
		echo "No USB port found"; \
	else \
		idf.py -p $$usbport flash; \
	fi
	screen /dev/cu.usbmodem*

close:
	@echo "Closing screen"
	@screen -X -S $$(screen -ls | grep Detached | awk '{print $$1}') quit

.PHONY: build flash