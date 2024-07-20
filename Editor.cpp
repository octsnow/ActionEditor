#include "Editor.hpp"
#include <iostream>
#include <Windows.h>
#include <commdlg.h>
#include <string>
#include <fstream>
#include <tuple>

// block panel
#define BPANEL_BLOCK_SIZE 8
#define BPANEL_BLOCK_COL 20

// palette
#define PALETTE_W 0.2f
#define PALETTE_BLOCK_COL 3
#define PALETTE_BLOCK_SPACE 0.2f // 1.0f - same to size of block
#define PALETTE_BLOCK_PADLEFT 0.5f // 1.0f - same to size of block
#define PALETTE_BLOCK_PADRIGHT 0.5f // 1.0f - same to size of block

using namespace std;

namespace {
    void ShowFileDialog();

    OctGame* gPGame = nullptr;
    unsigned int gStageWidth = 0;
    unsigned int gStageHeight = 0;
    BlockID* gStage = nullptr;
    unsigned int gX = 0;
    unsigned int gY = 0;
    unsigned int gWidth = 0;
    unsigned int gHeight = 0;
    unsigned int gSelectedBlock = 0;
    BlockID gCurrentBlock = BID_STONE;

    vector<tuple<string, BlockID>> gFilePaths = {
        {"images/blocks/stone.bmp", BID_STONE},
        {"images/blocks/soil.bmp", BID_DIRT},
    };
    vector<Block> gBlockList;

    void LoadImages() {
        if(!gBlockList.empty()) return;

        for(auto bInfo : gFilePaths) {
            string path = get<0>(bInfo);
            BlockID id = get<1>(bInfo);
            GHandle gh = gPGame->LoadImageFile(path, true);

            if(gh == ILFAILED) {
                cerr << "LoadImage(): Failed to load file " << path << endl;
                return;
            }

            Block block;
            block.gh = gh;
            block.bid = id;
            gBlockList.push_back(block);
        }
    }

    void DrawBlockPanel() {
        if(gStage == nullptr) return;

        unsigned int blockPanelWidth = gWidth * (1 - PALETTE_W);
        unsigned int bsize = blockPanelWidth / BPANEL_BLOCK_COL;
        unsigned int sx = gX / BPANEL_BLOCK_SIZE;
        unsigned int sy = gY / BPANEL_BLOCK_SIZE;
        unsigned int w = BPANEL_BLOCK_COL + 1;
        unsigned int h = BPANEL_BLOCK_COL / static_cast<float>(blockPanelWidth) * gHeight + 1;

        if(sx + w > gStageWidth) w = gStageWidth - sx;
        if(sy + h > gStageHeight) h = gStageHeight - sy;

        for(int y = 0; y < h; y++) {
            for(int x = 0; x < w; x++) {
                unsigned int bx = x * bsize - (gX % BPANEL_BLOCK_SIZE) * (blockPanelWidth / static_cast<float>(BPANEL_BLOCK_COL * BPANEL_BLOCK_SIZE));
                unsigned int by = y * bsize - (gY % BPANEL_BLOCK_SIZE) * (blockPanelWidth / static_cast<float>(BPANEL_BLOCK_COL * BPANEL_BLOCK_SIZE));
                unsigned int index = (y + sy) * gStageWidth + (x + sx);
                int color = 0;

                switch(gStage[index]) {
                case BID_NONE:
                    color = 0x000000;
                    break;
                case BID_STONE:
                    color = 0x888888;
                    break;
                case BID_DIRT:
                    color = 0xAA6004;
                    break;
                }
                gPGame->DrawBox(bx, by, bx + bsize, by + bsize, color, true);

                if(index == gSelectedBlock) {
                    gPGame->DrawBox(bx, by, bx + bsize, by + bsize, 0xFFFFFF, true, 0.5);
                }
                gPGame->DrawBox(bx, by, bx + bsize, by + bsize, 0x888888, false);
            }
        }
    }

    void DrawPalette() {
        unsigned int pw = gWidth * PALETTE_W;
        int pX = gWidth - pw;
        int pY = 0;
        int blockSize = 50;
        gPGame->DrawBox(pX, pY, gWidth, gHeight, 0xE1ECFF, true);

        int sX = pX + blockSize * PALETTE_BLOCK_PADLEFT;
        int sY = pY + blockSize * PALETTE_BLOCK_PADRIGHT;
        for(int i = 0; i < gBlockList.size(); i++) {
            int x = sX + i % PALETTE_BLOCK_COL * static_cast<int>((1 + PALETTE_BLOCK_SPACE) * blockSize);
            int y = sY + i / PALETTE_BLOCK_COL * static_cast<int>((1 + PALETTE_BLOCK_SPACE) * blockSize);
            Block block = gBlockList[i];

            gPGame->DrawImage(block.gh, x, y);
        }
    }

    void Draw() {
        DrawBlockPanel();
        DrawPalette();
    }

    void Move() {
        if(gPGame->IsPressed('w')) { // up
            if(gY > 0) {
                gY--;
            }
        }
        if(gPGame->IsPressed('a')) { // left
            if(gX > 0) {
                gX--;
            }
        }
        if(gPGame->IsPressed('s')) { // down
            if(gY < (gStageHeight - BPANEL_BLOCK_COL / static_cast<float>(gWidth) * gHeight) * BPANEL_BLOCK_SIZE) {
                gY++;
            }
        }
        if(gPGame->IsPressed('d')) { // right
            if(gX < (gStageWidth - BPANEL_BLOCK_COL) * BPANEL_BLOCK_SIZE) {
                gX++;
            }
        }
        if(gPGame->IsDown('p')) {
            ShowFileDialog();
        }
    }

    void Update() {
        Move();
        Draw();
        gPGame->Update();
    }

    void SetCurrentBlock(BlockID bid) {
        gCurrentBlock  = bid;
    }

    void SetBlock(unsigned int index, BlockID bid) {
        if(index >= gStageWidth * gStageHeight) return;

        gStage[index] = bid;
    }

    unsigned int PosToStageIndex(unsigned int x, unsigned y) {
        unsigned int bsize = gWidth * (1 - PALETTE_W) / static_cast<float>(BPANEL_BLOCK_COL);
        x += (gX % BPANEL_BLOCK_SIZE) / static_cast<float>(BPANEL_BLOCK_SIZE) * bsize;
        y += (gY % BPANEL_BLOCK_SIZE) / static_cast<float>(BPANEL_BLOCK_SIZE) * bsize;

        unsigned int bx = gX / BPANEL_BLOCK_SIZE + x / bsize;
        unsigned int by = gY / BPANEL_BLOCK_SIZE + y / bsize;

        if(bx >= gStageWidth) bx = gStageWidth;
        if(by >= gStageWidth) by = gStageHeight;

        return by * gStageWidth + bx;
    }

    void Idle() {
        glutPostRedisplay();
    }

    void Display() {
        glClear(GL_COLOR_BUFFER_BIT);
        gPGame->ClearScreen();
        Update();
        gPGame->ScreenSwap();
    }

    void Reshape(int w, int h) {
        glViewport(0, 0, w, h);

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        gluPerspective(30.0, (double)w / (double)h, 1.0, 100.0);

        glMatrixMode(GL_MODELVIEW);

        gWidth = w;
        gHeight = h;
    }

    void MoveOnBlockPanel(int x, int y) {
        if(x > (gWidth * (1 - PALETTE_W))){
            return;
        }

        gSelectedBlock = PosToStageIndex(x, y);
    }

    void MoveOnPalette(int x, int y) {
        if(x <= (gWidth * (1 - PALETTE_W))){
            return;
        }

    }

    void ClickBlockPanel(int x, int y) {
        if(x > (gWidth * (1 - PALETTE_W))){
            return;
        }
        MoveOnBlockPanel(x, y);
        SetBlock(gSelectedBlock, gCurrentBlock);
    }

    void ClickPalette(int x, int y) {
        int bpW = gWidth * (1 - PALETTE_W);
        int blockSize = 50;
        int blockSpace = blockSize * PALETTE_BLOCK_SPACE;
        if(x <= bpW){
            return;
        }
        MoveOnPalette(x, y);

        x -= bpW + blockSize * PALETTE_BLOCK_PADLEFT;
        y -= blockSize * PALETTE_BLOCK_PADRIGHT;

        int i = y / (blockSize + blockSpace) * PALETTE_BLOCK_COL + x / (blockSize + blockSpace);

        if(i >= gBlockList.size()) {
            return;
        }

        Block block = gBlockList[i];
        gCurrentBlock = block.bid;
    }

    void Mouse(int button, int state, int x, int y) {
        if(button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
            ClickBlockPanel(x, y);
            ClickPalette(x, y);
        }
    }

    void MouseMove(int x, int y) {
        MoveOnBlockPanel(x, y);
        MoveOnPalette(x, y);
    }

    void MouseDrag(int x, int y) {
        ClickBlockPanel(x, y);
        ClickPalette(x, y);
    }

    void WriteStageFile(TCHAR* filename) {
#ifdef UNICODE
        vector<char> buffer;
        int size = WideCharToMultiByte(CP_UTF8, 0, filename, -1, NULL, 0, NULL, NULL);
        if(size > 0) {
            buffer.resize(size);
            WideCharToMultiByte(CP_UTF8, 0, filename, -1, static_cast<BYTE*>(&buffer[0]), buffer.size(), NULL, NULL);
        } else {
            return;
        }

        string str(&buffer[0]);
#else
        string str(filename);
#endif

        ofstream ofs(str, ios::out | ios::binary | ios::trunc);

        if(!ofs) {
            cout << "error: faild to saving stage file" << endl;
            return;
        }

        unsigned char width = gStageWidth, height = gStageHeight;
        ofs.write((char*)&width, sizeof(char));
        ofs.write((char*)&height, sizeof(char));
        ofs.write((char*)gStage, sizeof(BlockID) * gStageWidth * gStageHeight);
        ofs.close();
    }

    void ShowFileDialog() {
        TCHAR szFileName[512];
        OPENFILENAME oFileName;

        ZeroMemory(szFileName, sizeof(TCHAR) * 512);
        ZeroMemory(&oFileName, sizeof(OPENFILENAME));
        oFileName.lStructSize = sizeof(OPENFILENAME);
        oFileName.hwndOwner = gPGame->GetWindowHandle();
        oFileName.lpstrFilter = TEXT("Stage Files\0*.stg");
        oFileName.nFilterIndex = 1;
        oFileName.lpstrFile = szFileName;
        oFileName.nMaxFile = 512;
        oFileName.lpstrDefExt = TEXT("stg");
        oFileName.nMaxFileTitle = 0;
        oFileName.lpstrFileTitle = NULL;
        oFileName.lpstrTitle = NULL;

        GetOpenFileName(&oFileName);
        WriteStageFile(szFileName);
    }
}

Editor::Editor(int width, int height) {
    gWidth = width;
    gHeight = height;

    gStage = nullptr;
}

Editor::~Editor() {
    gPGame->Destroy();
}

void Editor::Init(int* argc, char** argv) {
    gPGame->Init(argc, argv, gWidth, gHeight);
    gPGame->IdleFunc(Idle);
    gPGame->DisplayFunc(Display);
    gPGame->ReshapeFunc(Reshape);
    gPGame->MouseFunc(Mouse);
    gPGame->MouseMoveFunc(MouseMove);
    gPGame->MouseDragFunc(MouseDrag);

    LoadImages();

    glClearColor(0.0, 0.0, 1.0, 0.0);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
}

void Editor::Start(int* argc, char** argv) {
    gPGame = new OctGame();
    this->Init(argc, argv);
    glutMainLoop();
    this->Term();
}

void Editor::Term() {
    if(gStage != nullptr) {
        delete gStage;
    }
    gPGame->Destroy();
}

void Editor:: SetStageSize(unsigned int width, unsigned int height) {
    gStageWidth = width;
    gStageHeight = height;

    if(gStage != nullptr) {
        delete gStage;
    }

    gStage = new BlockID[width * height];
    this->ResetStage();
}

void Editor::ResetStage() {
    for(int i = 0; i < gStageWidth * gStageHeight; i++) {
        gStage[i] = BID_NONE;
    }
}
