CXX ?= g++
CXXFLAGS += -O3 -march=native -std=c++17 -Ilibs/include/
LDFLAGS += -Llibs/lib/ -lscip
CASES = 00 02 03 04 07 06 05 08 09 01 10

all: checker scheduler $(CASES:%=out/%.out) validate

validate: $(CASES:%=validate%)

checker: checker.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

scheduler: scheduler.cpp libs/lib/libscip.so
	$(CXX) $(CXXFLAGS) $< -o $@ $(LDFLAGS)

out/%.out: scheduler libs/lib/libscip.so in/%.in
	mkdir -p out
	export LD_LIBRARY_PATH=libs/lib/; ./scheduler in/$(patsubst out/%.out,%.in,$@) $@

validate%: checker in/%.in out/%.out
	./checker --public in/$(patsubst validate%,%.in,$@) out/$(patsubst validate%,%.out,$@)

clean:
	rm -rf checker scheduler out

scipoptsuite-7.0.2: scipoptsuite-7.0.2.tgz
	if [ ! -d "scipoptsuite-7.0.2" ]; then tar xvzf scipoptsuite-7.0.2.tgz; fi

build/bin/scip: | scipoptsuite-7.0.2
	mkdir -p build
	cmake -Bbuild -Hscipoptsuite-7.0.2 -DGMP=OFF -DREADLINE=OFF -DTPI=tny -DZIMPL=OFF -DZLIB=OFF -LH
	cd build; make

libs/lib/libscip.so: | build/bin/scip
	cd build; cmake . -DCMAKE_INSTALL_PREFIX=../libs/ -LH; make install

solve%: build/bin/scip out/%.out.lp
	./build/bin/scip -c "read out/$(patsubst solve%,%,$@).out.lp optimize display solution quit"
