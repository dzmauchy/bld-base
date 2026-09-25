#pragma once

#include "core/array.hpp"
#include "core/callback.hpp"

/*{"kind":"type","id":"pss","name":"Push stream","description":"A stream of data that can be pushed to","params":{"T":{"name":"Push stream type"}}}*/
template <typename T>
using Pss = Consumer<T>;

template <typename T>
using VectorizedInput = Array<T*>;
