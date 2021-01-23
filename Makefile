CXX ?= g++
CXXFLAGS += -O3 -march=native -std=c++17
CASES = 00 01 02 03 04 07 06 05 08 09 10
include in-private/Makefile

.PHONY: private_validate
.PRECIOUS: $(CASES:%=out/%.out) $(PRIVATE_CASES:%=out-private/%.out)

all: checker scheduler $(CASES:%=out/%.out) validate

validate: checker
	@$(foreach t, $(CASES), if [ -f "out/$(t).out" ]; then echo $(t); ./checker in/$(t).in out/$(t).out; fi;)

private_case: checker scheduler $(PRIVATE_CASES:%=out-private/%.out)

PRIVATE_CHECK_CMD = echo $(t); ./checker in-private/$(t).in out-private/$(t).out
private_validate: checker
	@$(foreach t, $(PRIVATE_CASES), if [ -f "out-private/$(t).out" ]; then $(PRIVATE_CHECK_CMD); fi;)

checker: checker.cpp
	$(CXX) $(CXXFLAGS) $^ -o $@

scheduler: scheduler.cpp src/PS.cpp
	# make libs-or/lib/libortools.so
	$(CXX) $(CXXFLAGS) -Ilibs-or/include/ $^ -o $@ -Llibs-or/lib/ -lortools

out/%.out: scheduler in/%.in
	mkdir -p out
	export LD_LIBRARY_PATH=libs/lib/:libs-or/lib/$${LD_LIBRARY_PATH:+:$$LD_LIBRARY_PATH}; time ./scheduler in/$(patsubst out/%.out,%.in,$@) $@

out-private/%.out: checker scheduler in-private/%.in
	mkdir -p out-private
	export LD_LIBRARY_PATH=libs/lib/:libs-or/lib/$${LD_LIBRARY_PATH:+:$$LD_LIBRARY_PATH}; time ./scheduler in-private/$(patsubst out-private/%.out,%.in,$@) $@
	./checker --public in-private/$(patsubst out-private/%.out,%.in,$@) $@

clean:
	rm -rf checker scheduler

or-tools/CMakeLists.txt:
	@if [ ! -d "or-tools" ]; then git clone https://github.com/google/or-tools; fi

build-or/lib/libortools.so: or-tools/CMakeLists.txt
	mkdir -p build-or
	cd build-or; cmake -B. -H../or-tools -DBUILD_SAMPLES=OFF -DBUILD_EXAMPLES=OFF -DBUILD_DEPS=ON -DUSE_COINOR=OFF -LH; make

libs-or/lib/libortools.so: | build-or/lib/libortools.so
	cd build-or; cmake . -DCMAKE_INSTALL_PREFIX=../libs-or/ -LH; make install
