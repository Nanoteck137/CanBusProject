#pragma once

#include "common.h"

typedef ErrorCode (*Func)(uint8_t* params, size_t num_params);

#define DEFINE_FUNC(name) ErrorCode name(uint8_t* params, size_t num_params)

#define EXPECT_NUM_PARAMS(num)                                                 \
    if (num_params < num)                                                      \
    {                                                                          \
        return ErrorCode::InsufficientFunctionParameters;                      \
    }

#define GET_PARAM(index) params[index]
