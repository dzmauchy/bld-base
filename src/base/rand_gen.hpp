#pragma once

#include <base/periodic_source.hpp>
#include <core/hal.hpp>

namespace push {

template <typename T>
class RandGen : public PeriodicSource<T> {
 public:
  ~RandGen() override = default;

  [[nodiscard]] auto precision() const { return this->intervalMs(); }
  [[nodiscard]] auto amplitude() const { return amplitude_; }

 protected:
  RandGen(u32 blockId, u32 precision, T amplitude) : PeriodicSource<T>(blockId, precision), amplitude_(amplitude) {}

  [[nodiscard]] T sample() override { return random_of<T>() * amplitude_; }

 private:
  T amplitude_;
};

}  // namespace push
