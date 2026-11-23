#pragma once

#include <cstdint>

enum class BlockType : uint8_t {
    Air = 0,
    Grass_Block,
    Dirt,
    Stone,
    Oak_Log,
    Oak_Leaves,
    Sand,
    Water,
    Snow,
    Spruce_Log,
    Spruce_Leaves,
    Ice,
    Size
};

inline bool BlockIsTransparent(BlockType type) {
    return type == BlockType::Oak_Leaves || type == BlockType::Water || type == BlockType::Spruce_Leaves || type == BlockType::Ice;
}
