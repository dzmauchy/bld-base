#pragma once

template <typename... Args>
class Consumer {
 public:
  virtual ~Consumer() = default;
  virtual void operator()(Args... args) = 0;
};

template <typename R, typename... Args>
class Function {
 public:
  virtual ~Function() = default;
  virtual R operator()(Args... args) = 0;
};

class Callback : public Consumer<> {
 public:
  ~Callback() override = default;
};
