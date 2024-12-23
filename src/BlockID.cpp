#include "BlockID.hpp"

bool IsBlock(BLOCK_ID blockId) {
    return BLOCK_ID::BID_NONE < blockId && blockId <= BLOCK_ID::BID_SOIL;
}
