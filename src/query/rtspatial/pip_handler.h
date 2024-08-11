#pragma once
#include <optix.h>
#include <thrust/pair.h>

#include <cstdint>

#include "rtspatial/utils/queue.h"
#include "query/rtspatial/lsi_context.h"

extern "C" __forceinline__ __device__ void rtspatial_handle_point_contains(
    uint32_t geom_id, uint32_t query_id, void* arg) {
}

extern "C" __forceinline__ __device__ void rtspatial_handle_envelope_contains(
    uint32_t geom_id, uint32_t query_id, void* arg) {
}

extern "C" __forceinline__ __device__ void rtspatial_handle_envelope_intersects(
    uint32_t geom_id, uint32_t query_id, void* arg) {



}
