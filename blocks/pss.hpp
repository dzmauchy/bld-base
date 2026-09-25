#pragma once

#include "core/array.hpp"
#include "core/callback.hpp"

/**
 * <type name="Push stream" description="A stream of data that can be pushed to">
 *   <arg name="T" description="Push stream type"/>
 * </type>
 */
template <typename T>
using Pss = Consumer<T>;

template <typename T>
using VectorizedInput = Array<T*>;
