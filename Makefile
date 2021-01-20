CXX ?= g++
CXXFLAGS += -O3 -march=native -std=c++17 -Ilibs/include/ -Ilibs-or/include/
LDFLAGS += -Llibs/lib/ -lscip -Llibs-or/lib/ -lortools
CASES = 00 01 02 03 04 07 06 05 08 09 10
include in-private/Makefile

.PHONY: private_validate
.PRECIOUS: $(CASES:%=out/%.out) $(PRIVATE_CASES:%=out-private/%.out)

all: checker scheduler $(CASES:%=out/%.out) validate

validate: $(CASES:%=out/%.out) $(CASES:%=validate%)

private_case: checker scheduler $(PRIVATE_CASES:%=out-private/%.out)

PRIVATE_CHECK_CMD = echo $(t); ./checker in-private/$(t).in out-private/$(t).out
private_validate: checker
	@$(foreach t, $(PRIVATE_CASES), if [ -f "out-private/$(t).out" ]; then $(PRIVATE_CHECK_CMD); fi;)

checker: checker.cpp
	$(CXX) $(CXXFLAGS) $^ -o $@

scheduler: scheduler.cpp src/PS.cpp | libs/lib/libscip.so libs-or/lib/libortools.so
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

out/%.out: scheduler in/%.in | libs/lib/libscip.so libs-or/lib/libortools.so
	mkdir -p out
	export LD_LIBRARY_PATH=libs/lib/:libs-or/lib/$${LD_LIBRARY_PATH:+:$$LD_LIBRARY_PATH}; time ./scheduler in/$(patsubst out/%.out,%.in,$@) $@

out-private/%.out: checker scheduler in-private/%.in | libs/lib/libscip.so libs-or/lib/libortools.so
	mkdir -p out-private
	export LD_LIBRARY_PATH=libs/lib/:libs-or/lib/$${LD_LIBRARY_PATH:+:$$LD_LIBRARY_PATH}; time ./scheduler in-private/$(patsubst out-private/%.out,%.in,$@) $@
	./checker --public in-private/$(patsubst out-private/%.out,%.in,$@) $@

validate%: checker in/%.in out/%.out
	./checker --public in/$(patsubst validate%,%.in,$@) out/$(patsubst validate%,%.out,$@)

clean:
	rm -rf checker scheduler

scipoptsuite-7.0.2: scipoptsuite-7.0.2.tgz
	@if [ ! -d "scipoptsuite-7.0.2" ]; then tar xvzf scipoptsuite-7.0.2.tgz; fi

bliss/libbliss.a: bliss-0.73.zip
	@if [ ! -d "bliss" ]; then unzip bliss-0.73.zip; mv bliss-0.73 bliss; fi
	cd bliss; make

build/bin/scip: | scipoptsuite-7.0.2 bliss/libbliss.a
	mkdir -p build
	@if [ ! -d "build/bliss" ]; then ln -s ../bliss build/bliss; fi
	cd build; cmake -B. -H../scipoptsuite-7.0.2 -DBLISS_DIR=../bliss -DGMP=OFF -DREADLINE=OFF -DSYM=bliss -DTPI=tny -DZIMPL=OFF -DZLIB=OFF -LH
	cd build; make

libs/lib/libscip.so: | build/bin/scip
	cd build; cmake . -DCMAKE_INSTALL_PREFIX=../libs/ -LH; make install

solve%: build/bin/scip out/%.out.lp
	./build/bin/scip -c "read out/$(patsubst solve%,%,$@).out.lp optimize display solution quit"

or-tools/CMakeLists.txt:
	@if [ ! -d "or-tools" ]; then git clone https://github.com/google/or-tools; fi

build-or/lib/libortools.so: or-tools/CMakeLists.txt
	mkdir -p build-or
	cd build-or; cmake -B. -H../or-tools -DBUILD_SAMPLES=OFF -DBUILD_EXAMPLES=OFF -DBUILD_DEPS=ON -DUSE_COINOR=OFF -LH; make

libs-or/lib/libortools.so: | build-or/lib/libortools.so
	cd build-or; cmake . -DCMAKE_INSTALL_PREFIX=../libs-or/ -LH; make install
