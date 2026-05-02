.PHONY: build docker-build docker-run pt1 pt2

# To test touch in QEMU, set PEBBLE_QEMU_PATH=$(HOME)/.local/share/pebble-qemu/qemu-pebble-wrapper.sh
# once a 4.9.171+ SDK is published to sdk.repebble.com — current 4.9.169 firmware
# boot-loops against QEMU 10.x's pebble-emery machine model.

build:
	pebble build

pt1: build
	pebble install --emulator basalt --logs

pt2: build
	pebble install --emulator emery --logs

docker-build:
	docker build --platform linux/amd64 -t pebble-builder .

docker-run: docker-build
	mkdir -p build
	docker run --rm --platform linux/amd64 \
		-v "$(PWD)/src:/app/src:ro" \
		-v "$(PWD)/resources:/app/resources:ro" \
		-v "$(PWD)/package.json:/app/package.json:ro" \
		-v "$(PWD)/wscript:/app/wscript:ro" \
		-v "$(PWD)/build:/app/build" \
		pebble-builder
