CXX ?= g++
CXXFLAGS += -O3 -march=native -std=c++17
CASES = 00 01 02 03 04 05 06 07 08 09 10

all: checker scheduler $(CASES:%=out/%.out) validate

validate: $(CASES:%=validate%)

checker: checker.cpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $<

scheduler: scheduler.cpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $<

out/%.out: scheduler in/%.in
	mkdir -p out
	./scheduler in/$(patsubst out/%.out,%.in,$@) $@

validate%: checker in/%.in out/%.out
	./checker --public in/$(patsubst validate%,%.in,$@) out/$(patsubst validate%,%.out,$@)

clean:
	rm -rf checker scheduler out
