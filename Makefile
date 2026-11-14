.DEFAULT_GOAL := build-debug

.PHONY: build-debug build-release run-debug run-release test-debug test-release
.PHONY: bd br rd rr td tr
.PHONY: build-sanitized run-sanitized test-sanitized bs rs ts

build-debug:
	cmake -S . -B build/debug -DCMAKE_BUILD_TYPE=Debug \
		-DBUILD_TESTING=ON -DENABLE_SANITIZERS=OFF
	cmake --build build/debug

build-release:
	cmake -S . -B build/release -DCMAKE_BUILD_TYPE=Release \
		-DBUILD_TESTING=ON -DENABLE_SANITIZERS=OFF
	cmake --build build/release

build-sanitized:
	cmake -S . -B build/sanitized -DCMAKE_BUILD_TYPE=Debug \
		-DBUILD_TESTING=ON -DENABLE_SANITIZERS=ON
	cmake --build build/sanitized

run-debug: build-debug
	./build/debug/avl_console

run-release: build-release
	./build/release/avl_console

run-sanitized: build-sanitized
	./build/sanitized/avl_console

test-debug: build-debug
	ctest --test-dir build/debug --output-on-failure

test-release: build-release
	ctest --test-dir build/release --output-on-failure

test-sanitized: build-sanitized
	ctest --test-dir build/sanitized --output-on-failure

bd: build-debug
br: build-release
rd: run-debug
rr: run-release
td: test-debug
tr: test-release
bs: build-sanitized
rs: run-sanitized
ts: test-sanitized
