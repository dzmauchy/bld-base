#pragma once

#include "core/array.hpp"
#include "core/callback.hpp"

/**
 * <type id="pss" name="Push stream">
 *   <description>A stream of data that can be pushed to</description>
 *   <param name="T">Push stream type</param>
 * </type>
 */
template <typename T>
using Pss = Consumer<T>;

template <typename T>
using VectorizedInput = Array<T*>;
