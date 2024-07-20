#include "Editor.hpp"

#ifdef DEBUG
#pragma comment(lib, "opencv_world455d.lib")

#else
#pragma comment(lib, "opencv_world455.lib")

#endif

#define WINDOW_W 1920
#define WINDOW_H 1080

int main(int argc, char** argv) {
    Editor editor(WINDOW_W, WINDOW_H);
    editor.SetStageSize(100, 50);
    editor.Start(&argc, argv);

    return 0;
}
