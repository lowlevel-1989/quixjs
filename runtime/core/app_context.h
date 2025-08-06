#pragma once

#include "backend/backend.h"
#include "core/config_loader.h"

typedef struct {
    Backend* backend;
    Config*  cfg;
} AppContext;
