#pragma once

#include "../OctGame/OctGame.hpp"

#pragma comment(lib, "User32.lib")
#pragma comment(lib, "Comdlg32.lib")

typedef uint8_t BlockID;

const BlockID BID_NONE = 0;
const BlockID BID_STONE = 1;
const BlockID BID_DIRT = 2;

struct Block {
    BlockID bid;
    GHandle gh;
};

class Editor {
public:
    Editor(int width, int height);
    ~Editor();
    void Init(int* argc, char** argv);
    void Start(int* argc, char** argv);
    void Term();

    void SetStageSize(unsigned int width, unsigned int height);
    void ResetStage();
};
