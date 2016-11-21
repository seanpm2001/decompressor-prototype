// -*- C++ -*- */
//
// Copyright 2016 WebAssembly Community Group participants
//
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

// Defines the compressor of WASM files based on integer usage.

#ifndef DECOMPRESSOR_SRC_INTCOMP_INTCOMPRESS_H
#define DECOMPRESSOR_SRC_INTCOMP_INTCOMPRESS_H

#include "intcomp/IntCountNode.h"
#include "interp/StreamReader.h"
#include "interp/TraceSexpReader.h"
#include "interp/StreamWriter.h"

namespace wasm {

namespace intcomp {

typedef uint32_t CollectionFlags;


enum class CollectionFlag : CollectionFlags {
  None = 0x0,
  TopLevel = 0x1,
  IntPaths = 0x2,
  All = 0x3
};

inline CollectionFlags Flag(CollectionFlag F) { return CollectionFlags(F); }

inline bool hasFlag(CollectionFlag F, CollectionFlags Flags) {
  return Flag(F) & Flags;
}

class CounterWriter;

class IntCompressor FINAL {
  IntCompressor() = delete;
  IntCompressor(const IntCompressor&) = delete;
  IntCompressor& operator=(const IntCompressor&) = delete;

 public:
  IntCompressor(std::shared_ptr<decode::Queue> InputStream,
                std::shared_ptr<decode::Queue> OutputStream,
                std::shared_ptr<filt::SymbolTable> Symtab);

  ~IntCompressor();

  bool errorsFound() const { return Input == nullptr && Input->errorsFound(); }

  void compress();

  void setTraceProgress(bool NewValue) {
    if (Trace)
      Trace->setTraceProgress(NewValue);
  }

  void setCountCutoff(uint64_t NewCutoff) { CountCutoff = NewCutoff; }
  void setWeightCutoff(uint64_t NewCutoff) { WeightCutoff = NewCutoff; }
  void setLengthLimit(size_t NewLimit) { LengthLimit = NewLimit; }

  void setMinimizeBlockSize(bool NewValue) { (void)NewValue; }

  interp::TraceClassSexpReader& getTrace() { return *Trace; }

  void describe(FILE* Out, CollectionFlags = Flag(CollectionFlag::All));

 private:
  std::shared_ptr<filt::SymbolTable> Symtab;
  interp::StreamReader* Input;
  CounterWriter* Counter;
  decode::ReadCursor StartPos;
  IntCountUsageMap UsageMap;
  interp::TraceClassSexpReader* Trace;
  uint64_t CountCutoff;
  uint64_t WeightCutoff;
  size_t LengthLimit;
  void compressUpToSize(size_t Size);
  void removeSmallUsageCounts() { removeSmallUsageCounts(UsageMap); }
  void removeSmallUsageCounts(IntCountUsageMap& UsageMap);
  bool removeSmallUsageCounts(CountNode* Nd);
};

}  // end of namespace intcomp

}  // end of namespace wasm

#endif  // DECOMPRESSOR_SRC_INTCOMP_INTCOMPRESS_H
