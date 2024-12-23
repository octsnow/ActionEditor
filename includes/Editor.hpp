#pragma once

#include <stdint.h>

#pragma comment(lib, "User32.lib")
#pragma comment(lib, "Comdlg32.lib")

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
