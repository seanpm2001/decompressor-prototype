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

// Implements a writer that injects abbreviations into the input stream.

#include "intcomp/AbbrevAssignWriter.h"

namespace wasm {

using namespace decode;
using namespace filt;

namespace intcomp {

const char* AbbrevAssignWriter::getDefaultTraceName() const {
  return "AbbrevAssignWriter";
}

utils::TraceClass::ContextPtr AbbrevAssignWriter::getTraceContext() {
  return Writer.getTraceContext();
}

void AbbrevAssignWriter::forwardAbbrevValue(IntType Value) {
  flushDefaultValues();
  writeValue(Value, AbbrevFormat);
}

void AbbrevAssignWriter::forwardOtherValue(IntType Value) {
  DefaultValues.push_back(Value);
}

void AbbrevAssignWriter::setTrace(std::shared_ptr<filt::TraceClassSexp> Trace) {
  Writer::setTrace(Trace);
  Writer.setTrace(Trace);
}

StreamType AbbrevAssignWriter::getStreamType() const {
  return StreamType::Int;
}

bool AbbrevAssignWriter::writeUint8(uint8_t Value) {
  bufferValue(Value);
  return true;
}

bool AbbrevAssignWriter::writeUint32(uint32_t Value) {
  bufferValue(Value);
  return true;
}

bool AbbrevAssignWriter::writeUint64(uint64_t Value) {
  bufferValue(Value);
  return true;
}

bool AbbrevAssignWriter::writeVarint32(int32_t Value) {
  bufferValue(Value);
  return true;
}

bool AbbrevAssignWriter::writeVarint64(int64_t Value) {
  bufferValue(Value);
  return true;
}

bool AbbrevAssignWriter::writeVaruint32(uint32_t Value) {
  bufferValue(Value);
  return true;
}

bool AbbrevAssignWriter::writeVaruint64(uint64_t Value) {
  bufferValue(Value);
  return true;
}

bool AbbrevAssignWriter::writeFreezeEof() {
  writeUntilBufferEmpty();
  flushDefaultValues();
  return Writer.writeFreezeEof();
}

bool AbbrevAssignWriter::writeValue(decode::IntType Value, const filt::Node*) {
  bufferValue(Value);
  return true;
}

bool AbbrevAssignWriter::writeAction(const filt::CallbackNode* Action) {
  const auto* Sym = dyn_cast<SymbolNode>(Action->getKid(0));
  if (Sym == nullptr)
    return false;
  switch (Sym->getPredefinedSymbol()) {
    case PredefinedSymbol::Block_enter:
      writeUntilBufferEmpty();
      flushDefaultValues();
      assert(Root->getBlockEnter()->hasAbbrevIndex());
      forwardAbbrevValue(Root->getBlockEnter()->getAbbrevIndex());
      return true;
    case PredefinedSymbol::Block_exit:
      writeUntilBufferEmpty();
      flushDefaultValues();
      assert(Root->getBlockExit()->hasAbbrevIndex());
      forwardAbbrevValue(Root->getBlockExit()->getAbbrevIndex());
      return true;
    default:
      return Writer.writeAction(Action);
  }
}

void AbbrevAssignWriter::bufferValue(IntType Value) {
  assert(!Buffer.full());
  Buffer.push_back(Value);
  if (!Buffer.full())
    return;
  writeFromBuffer();
}

void AbbrevAssignWriter::writeFromBuffer() {
  // TODO(karlschimpf): When writing values, dont' create abbreviation
  // if there are already default values, and adding as a default value
  // will use less space.
  if (Buffer.empty())
    return;
  // Collect abbreviations available for value sequences in buffer.
  CountNode::IntPtr Nd;
  CountNode::IntPtr Max;
  for (IntType Value : Buffer) {
    Nd = Nd ? lookup(Nd, Value) : lookup(Root, Value);
    if (Nd->hasAbbrevIndex())
      Max = Nd;
  }
  if (!Max) {
    // Default to writing at least one value.
    forwardOtherValue(Buffer[0]);
    popValuesFromBuffer(1);
    return;
  }
  forwardAbbrevValue(Max->getAbbrevIndex());
  popValuesFromBuffer(Max->getPathLength());
}

void AbbrevAssignWriter::writeUntilBufferEmpty() {
  while (!Buffer.empty())
    writeFromBuffer();
}

void AbbrevAssignWriter::popValuesFromBuffer(size_t Size) {
  for (size_t i = 0; i < Size; ++i) {
    if (Buffer.empty())
      return;
    Buffer.pop_front();
  }
}

void AbbrevAssignWriter::writeValue(IntType Value, IntTypeFormat Format) {
  switch (Format) {
    case IntTypeFormat::Uint8:
      Writer.writeUint8(Value);
      break;
    case IntTypeFormat::Uint32:
      Writer.writeUint32(Value);
      break;
    case IntTypeFormat::Uint64:
      Writer.writeUint64(Value);
      break;
    case IntTypeFormat::Varint32:
      Writer.writeVarint32(Value);
      break;
    case IntTypeFormat::Varint64:
      Writer.writeVarint64(Value);
      break;
    case IntTypeFormat::Varuint32:
      Writer.writeVaruint32(Value);
      break;
    case IntTypeFormat::Varuint64:
      Writer.writeVaruint64(Value);
  }
}

void AbbrevAssignWriter::flushDefaultValues() {
  if (DefaultValues.empty())
    return;

  if (DefaultValues.size() == 1) {
    writeValue(Root->getDefaultSingle()->getAbbrevIndex(), AbbrevFormat);
    writeValue(DefaultValues[0], DefaultFormat);
    DefaultValues.clear();
    return;
  }

  writeValue(Root->getDefaultMultiple()->getAbbrevIndex(), AbbrevFormat);
  writeValue(DefaultValues.size(), LoopSizeFormat);
  for (const auto V : DefaultValues)
    writeValue(V, DefaultFormat);
  DefaultValues.clear();
}

}  // end of namespace intcomp

}  // end of namespace wasm