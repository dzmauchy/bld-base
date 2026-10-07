#pragma once

/**
 * Push Dataflows
 * @brief Push Dataflows
 * @image push-ns.svg
 */
namespace push {

/**
 * Single precision push dataflows
 * @brief Single precision push dataflows
 * @image push.f32-ns.svg
 */
namespace f_32 {

/**
 * Transformers
 * @brief Transformers
 * @image push.transformers-ns.svg
 */
namespace transformers {}

/**
 * Sinks
 * @brief Sinks
 * @image push.sinks-ns.svg
 */
namespace sinks {}

/**
 * Sources
 * @brief Sources
 * @image push.sources-ns.svg
 */
namespace sources {}

} // namespace f_32

/**
 * Double precision push dataflows
 * @brief Double precision push dataflows
 * @image push.f64-ns.svg
 */
namespace f_64 {

/**
 * Transformers
 * @brief Transformers
 * @image push.transformers-ns.svg
 */
namespace transformers {}

/**
 * Sinks
 * @brief Sinks
 * @image push.sinks-ns.svg
 */
namespace sinks {}

/**
 * Sources
 * @brief Sources
 * @image push.sources-ns.svg
 */
namespace sources {}

} // namespace f_64
} // namespace push
