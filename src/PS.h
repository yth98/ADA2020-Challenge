#include <algorithm>   // for max, copy, fill_n
#include <cassert>     // for assert
#include <cfenv>       // for feenableexcept
#include <cmath>       // for isfinite
#include <cstdint>     // for uint8_t, int8_t
#include <cstdlib>     // for exit
#include <cstring>     // for strcmp
#include <fstream>     // for ifstream
#include <iomanip>     // for operator<<, setprecision
#include <iostream>    // for operator<<, ifstream, basic_istream::operat...
#include <limits>      // for numeric_limits
#include <memory>      // for allocator, allocator_traits<>::value_type
#include <regex>       // for regex_match, match_results<>::_Base_type
#include <sstream>     // for stringstream
#include <string>      // for string, basic_string, operator+, char_traits
#include <tuple>       // for tuple
#include <utility>     // for pair
#include <vector>      // for vector, vector<>::reference, _Bit_reference

#ifndef DEFINE_PS
#define DEFINE_PS

struct Operation {
    uint32_t duration{}, start_time{};
    uint16_t slices{}, ij{};
    bool done{false}, inTopo{false}, pre{false}, front{false}, back{false};
    std::vector<uint16_t> deps;
    std::basic_string<uint8_t> in_slice;
};

struct Job {
    double weight{};
    uint32_t duration{0};
    std::vector<Operation> ops;
    std::vector<uint16_t> opTopo;
};

void OpTopoSort(Job&, uint16_t&);

std::pair<uint16_t, std::vector<Job>> ReadJobs(const std::string&);

void WriteSchedule(const std::string &, const std::vector<Job> &);

long double CalculateScore(std::vector<Job>&);

uint32_t TraditionalScheduling(std::vector<Job>&, const uint16_t &);

int timeLimit(const uint32_t&, const uint32_t, bool);

#endif
