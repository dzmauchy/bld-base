#pragma once

#include <core/types.hpp>
#include <functional>

extern "C" {
/* life-cycle callbacks */
void on_close(std::function<void()> *cbk);
void on_start(std::function<void()> *cbk);
void on_stop(std::function<void()> *cbk);

/* interval management */
u32  set_interval(u32                    milliseconds,
                  std::function<void()> *cbk);
void clear_interval(u32 intervalId);

/* gpio handling */
bool read_gpio(u32 port,
               u8  pin);
u32  set_gpio(u32                    port,
              u8                     pin,
              std::function<void()> *cbk);
void clear_gpio(u32 gpio_id);
void send_gpio(u32  port,
               u8   pin,
               bool value);

/* ADC/DAC */
f32  read_adc_f32(u32 port,
                  u8  pin);
f64  read_adc_f64(u32 port,
                  u8  pin);
void send_dac_f32(u32 port,
                  u8  pin,
                  f32 value);
void send_dac_f64(u32 port,
                  u8  pin,
                  f64 value);

/* metrics */
void send_value_f32(u32 blockId,
                    u8  inputId,
                    f32 value);
void send_value_f64(u32 blockId,
                    u8  inputId,
                    f64 value);

/* common functions */
f32 random_f32();
f64 random_f64();

/* time functions */
u64 get_time();
}

template <typename T> T random_of();

template <> inline f32 random_of<f32>() { return random_f32(); }

template <> inline f64 random_of<f64>() { return random_f64(); }
