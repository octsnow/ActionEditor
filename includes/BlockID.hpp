#pragma once

enum BLOCK_ID {
    BID_NONE = 0,
    BID_STONE,
    BID_SOIL,
    BID_GOAL,
    BID_NUM,
};

bool IsBlock(BLOCK_ID blockId);
