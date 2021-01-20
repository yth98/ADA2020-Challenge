// Copyright 2010-2018 Google LLC
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef OR_TOOLS_FLATZINC_LOGGING_H_
#define OR_TOOLS_FLATZINC_LOGGING_H_

#include <iostream>
#include <string>

#include "ortools/base/commandlineflags.h"
#include "ortools/base/logging.h"

// This file offers logging tool for the flatzinc interpreter.
// It supports internal logging mechanisms as well as official mechanism from
// the flatzinc specifications.

ABSL_DECLARE_FLAG(bool, fz_logging);
ABSL_DECLARE_FLAG(bool, fz_verbose);
ABSL_DECLARE_FLAG(bool, fz_debug);

#define FZENDL std::endl
#define FZLOG \
  if (absl::GetFlag(FLAGS_fz_logging)) std::cout << "%% "

#define FZVLOG \
  if (absl::GetFlag(FLAGS_fz_verbose)) std::cout << "%%%% "

#define FZDLOG \
  if (absl::GetFlag(FLAGS_fz_debug)) std::cout << "%%%%%% "

#define HASVLOG absl::GetFlag(FLAGS_fz_verbose)
#endif  // OR_TOOLS_FLATZINC_LOGGING_H_
