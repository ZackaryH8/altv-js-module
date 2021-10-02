// Copyright 2020 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef INCLUDE_CPPGC_INTERNAL_POINTER_POLICIES_H_
#define INCLUDE_CPPGC_INTERNAL_POINTER_POLICIES_H_

#include <cstdint>
#include <type_traits>

#include "cppgc/internal/write-barrier.h"
#include "cppgc/sentinel-pointer.h"
#include "cppgc/source-location.h"
#include "cppgc/type-traits.h"
#include "v8config.h"  // NOLINT(build/include_directory)

namespace cppgc {
namespace internal {

class HeapBase;
class PersistentRegion;
class CrossThreadPersistentRegion;

// Tags to distinguish between strong and weak member types.
class StrongMemberTag;
class WeakMemberTag;
class UntracedMemberTag;

struct DijkstraWriteBarrierPolicy {
  static void InitializingBarrier(const void*, const void*) {
    // Since in initializing writes the source object is always white, having no
    // barrier doesn't break the tri-color invariant.
  }
  static void AssigningBarrier(const void* slot, const void* value) {
    WriteBarrier::Params params;
    switch (WriteBarrier::GetWriteBarrierType(slot, value, params)) {
      case WriteBarrier::Type::kGenerational:
        WriteBarrier::GenerationalBarrier(params, slot);
        break;
      case WriteBarrier::Type::kMarking:
        WriteBarrier::DijkstraMarkingBarrier(params, value);
        break;
      case WriteBarrier::Type::kNone:
        break;
    }
  }
};

struct NoWriteBarrierPolicy {
  static void InitializingBarrier(const void*, const void*) {}
  static void AssigningBarrier(const void*, const void*) {}
};

class V8_EXPORT EnabledCheckingPolicy {
 protected:
  template <typename T>
  void CheckPointer(const T* ptr) {
    if (!ptr || (kSentinelPointer == ptr)) return;

    CheckPointersImplTrampoline<T>::Call(this, ptr);
  }

 private:
  void CheckPointerImpl(const void* ptr, bool points_to_payload);

  template <typename T, bool = IsCompleteV<T>>
  struct CheckPointersImplTrampoline {
    static void Call(EnabledCheckingPolicy* policy, const T* ptr) {
      policy->CheckPointerImpl(ptr, false);
    }
  };

  template <typename T>
  struct CheckPointersImplTrampoline<T, true> {
    static void Call(EnabledCheckingPolicy* policy, const T* ptr) {
      policy->CheckPointerImpl(ptr, IsGarbageCollectedTypeV<T>);
    }
  };

  const HeapBase* heap_ = nullptr;
};

class DisabledCheckingPolicy {
 protected:
  void CheckPointer(const void*) {}
};

#if V8_ENABLE_CHECKS
using DefaultMemberCheckingPolicy = EnabledCheckingPolicy;
using DefaultPersistentCheckingPolicy = EnabledCheckingPolicy;
#else
using DefaultMemberCheckingPolicy = DisabledCheckingPolicy;
using DefaultPersistentCheckingPolicy = DisabledCheckingPolicy;
#endif
// For CT(W)P neither marking information (for value), nor objectstart bitmap
// (for slot) are guaranteed to be present because there's no synchonization
// between heaps after marking.
using DefaultCrossThreadPersistentCheckingPolicy = DisabledCheckingPolicy;

class KeepLocationPolicy {
 public:
  constexpr const SourceLocation& Location() const { return location_; }

 protected:
  constexpr KeepLocationPolicy() = default;
  constexpr explicit KeepLocationPolicy(const SourceLocation& location)
      : location_(location) {}

  // KeepLocationPolicy must not copy underlying source locations.
  KeepLocationPolicy(const KeepLocationPolicy&) = delete;
  KeepLocationPolicy& operator=(const KeepLocationPolicy&) = delete;

  // Location of the original moved from object should be preserved.
  KeepLocationPolicy(KeepLocationPolicy&&) = default;
  KeepLocationPolicy& operator=(KeepLocationPolicy&&) = default;

 private:
  SourceLocation location_;
};

class IgnoreLocationPolicy {
 public:
  constexpr SourceLocation Location() const { return {}; }

 protected:
  constexpr IgnoreLocationPolicy() = default;
  constexpr explicit IgnoreLocationPolicy(const SourceLocation&) {}
};

#if CPPGC_SUPPORTS_OBJECT_NAMES
using DefaultLocationPolicy = KeepLocationPolicy;
#else
using DefaultLocationPolicy = IgnoreLocationPolicy;
#endif

struct StrongPersistentPolicy {
  using IsStrongPersistent = std::true_type;
  static V8_EXPORT PersistentRegion& GetPersistentRegion(const void* object);
};

struct WeakPersistentPolicy {
  using IsStrongPersistent = std::false_type;
  static V8_EXPORT PersistentRegion& GetPersistentRegion(const void* object);
};

struct StrongCrossThreadPersistentPolicy {
  using IsStrongPersistent = std::true_type;
  static V8_EXPORT CrossThreadPersistentRegion& GetPersistentRegion(
      const void* object);
};

struct WeakCrossThreadPersistentPolicy {
  using IsStrongPersistent = std::false_type;
  static V8_EXPORT CrossThreadPersistentRegion& GetPersistentRegion(
      const void* object);
};

// Forward declarations setting up the default policies.
template <typename T, typename WeaknessPolicy,
          typename LocationPolicy = DefaultLocationPolicy,
          typename CheckingPolicy = DefaultCrossThreadPersistentCheckingPolicy>
class BasicCrossThreadPersistent;
template <typename T, typename WeaknessPolicy,
          typename LocationPolicy = DefaultLocationPolicy,
          typename CheckingPolicy = DefaultPersistentCheckingPolicy>
class BasicPersistent;
template <typename T, typename WeaknessTag, typename WriteBarrierPolicy,
          typename CheckingPolicy = DefaultMemberCheckingPolicy>
class BasicMember;

}  // namespace internal

}  // namespace cppgc

#endif  // INCLUDE_CPPGC_INTERNAL_POINTER_POLICIES_H_
