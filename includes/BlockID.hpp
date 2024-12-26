#pragma once

enum BLOCK_ID {
    BID_NONE = 0,

    // blocks
    BID_STONE,
    BID_SOIL,
    BID_CONVEYOR_LEFT,
    BID_CONVEYOR_RIGHT,

    // gimmicks
    BID_GOAL,
    BID_LEASER,

    BID_NUM,
};

bool IsBlock(BLOCK_ID blockId);
