#pragma once

#include <cstdint>

#include "BlockType.hpp"
#include "GLUtil.hpp"

enum class BlockTexture : uint8_t {
    Grass_Top = 0,
    Grass_Side,
    Dirt,
    Stone,
    Oak_Log,
    Oak_Log_Top,
    Oak_Leaves,
    Sand,
    Water,
};

inline uint32_t GetTextureArray() {
    return GLUtil::LoadTextureArray({
        .paths = {
            "block/grass_block_top.png",
            "block/grass_block_side.png",
            "block/dirt.png",
            "block/stone.png",
            "block/oak_log.png",
            "block/oak_log_top.png",
            "block/oak_leaves.png",
            "block/sand.png",
            "block/water_overlay.png"
        }, 
        .filter = GL_NEAREST,
    });
}

struct BlockFaces {
    BlockTexture top, bottom, front, back, left, right;
};

inline BlockFaces blockTypeToBlockFace[(int)BlockType::Size] = {
    // Air
    {
    },
    // Grass Block
    {
        .top    = BlockTexture::Grass_Top,
        .bottom = BlockTexture::Dirt,
        .front  = BlockTexture::Grass_Side,
        .back   = BlockTexture::Grass_Side,
        .left   = BlockTexture::Grass_Side,
        .right  = BlockTexture::Grass_Side
    },
    // Dirt
    {
        .top    = BlockTexture::Dirt,
        .bottom = BlockTexture::Dirt,
        .front  = BlockTexture::Dirt,
        .back   = BlockTexture::Dirt,
        .left   = BlockTexture::Dirt,
        .right  = BlockTexture::Dirt
    },
    // Stone
    {
        .top    = BlockTexture::Stone,
        .bottom = BlockTexture::Stone,
        .front  = BlockTexture::Stone,
        .back   = BlockTexture::Stone,
        .left   = BlockTexture::Stone,
        .right  = BlockTexture::Stone
    },
    // Oak Log
    {
        .top    = BlockTexture::Oak_Log_Top,
        .bottom = BlockTexture::Oak_Log_Top,
        .front  = BlockTexture::Oak_Log,
        .back   = BlockTexture::Oak_Log,
        .left   = BlockTexture::Oak_Log,
        .right  = BlockTexture::Oak_Log
    },
    // Oak Leaves
    {
        .top    = BlockTexture::Oak_Leaves,
        .bottom = BlockTexture::Oak_Leaves,
        .front  = BlockTexture::Oak_Leaves,
        .back   = BlockTexture::Oak_Leaves,
        .left   = BlockTexture::Oak_Leaves,
        .right  = BlockTexture::Oak_Leaves
    },
    // Sand
    {
        .top    = BlockTexture::Sand,
        .bottom = BlockTexture::Sand,
        .front  = BlockTexture::Sand,
        .back   = BlockTexture::Sand,
        .left   = BlockTexture::Sand,
        .right  = BlockTexture::Sand
    },
    // Water
    {
        .top    = BlockTexture::Water,
        .bottom = BlockTexture::Water,
        .front  = BlockTexture::Water,
        .back   = BlockTexture::Water,
        .left   = BlockTexture::Water,
        .right  = BlockTexture::Water
    },
};
