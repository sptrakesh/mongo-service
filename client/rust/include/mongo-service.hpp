//
// Created by Rakesh on 21/10/2025.
//

#pragma once

#include "rust/cxx.h"
#include "mongo-service/src/mongoservice.rs.h"

void init_logger( Logger conf );
void init( Configuration conf );
rust::Vec<uint8_t> execute( rust::Vec<uint8_t> data );
