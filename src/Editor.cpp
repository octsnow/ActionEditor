#include "Editor.hpp"
#include <iostream>
#include <Windows.h>
#include <commdlg.h>
#include <string>
#include <fstream>

#include "OctGame.hpp"
#include "ImageList.hpp"
#include "BlockId.hpp"

// block panel
#define BPANEL_BLOCK_SIZE 8
#define BPANEL_BLOCK_COL 20

// palette
#define PALETTE_W 0.2f
#define PALETTE_BLOCK_COL 6
#define PALETTE_BLOCK_SPACE 0.2f // if is 1.0f, same to size of block
#define PALETTE_BLOCK_PADLEFT 0.5f // if is 1.0f, same to size of block
#define PALETTE_BLOCK_PADRIGHT 0.5f // if is 1.0f, same to size of block

using namespace std;

namespace {
    OctGame* gPGame = nullptr;
    unsigned char gStageWidth = 0;
    unsigned char gStageHeight = 0;
    BLOCK_ID* gStage = nullptr;
    unsigned int gX = 0;
    unsigned int gY = 0;
    unsigned int gWidth = 0;
    unsigned int gHeight = 0;
    unsigned int gSelectedBlock = 0;
    BLOCK_ID gCurrentBlockType = BID_STONE;
    int gPressedMouseButton;

    vector<string> gFilePaths = {
        "images/blocks/stone.bmp",
        "images/blocks/soil.bmp",
        "images/blocks/conveyor_left.bmp",
        "images/blocks/conveyor_right.bmp",
        "images/gimmicks/goal1.bmp",
    };
    vector<GHandle> gBlockGHandles;

    void ShowOpenFileDialog(TCHAR *szFileName) {
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
    }

    void ShowSaveFileDialog(TCHAR *szFileName) {
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
        //oFileName.Flags = OFN_EXPLORER;

        GetSaveFileName(&oFileName);
    }

    void WriteStageFile(TCHAR *filename) {
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
            printf("%s", str.c_str());
            return;
        }

        unsigned char* buffer = new unsigned char[gStageWidth * gStageHeight];
        ofs.write((char*)&gStageWidth, sizeof(char));
        ofs.write((char*)&gStageHeight, sizeof(char));

        for(int row = 0; row < gStageHeight; row++) {
            for(int col = 0; col < gStageWidth ; col++) {
                int i = row * gStageWidth + col;
                buffer[i] = (unsigned int)gStage[i] > 255 ? 255 : gStage[i];
            }
        }
        ofs.write((char*)buffer, gStageWidth * gStageHeight);
        ofs.close();

        delete[] buffer;
    }

    void LoadStageFile(TCHAR *filename) {
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

        ifstream ifs(str, ios::in | ios::binary);

        if(!ifs) {
            cout << "error: faild to saving stage file" << endl;
            return;
        }

        unsigned char width, height;
        ifs.read((char*)&width, sizeof(char));
        ifs.read((char*)&height, sizeof(char));

        gStageWidth = width;
        gStageHeight = height;

        if(gStage != nullptr) {
            delete[] gStage;
        }

        gStage = new BLOCK_ID[gStageWidth * gStageHeight];

        unsigned char* buffer = new unsigned char[gStageWidth * gStageHeight];
        ifs.read((char*)buffer, gStageWidth * gStageHeight);
        ifs.close();

        for(int row = 0; row < gStageHeight; row++) {
            for(int col = 0; col < gStageWidth; col++) {
                gStage[row * gStageWidth + col] = (BLOCK_ID)buffer[row * gStageWidth + col];
            }
        }

        delete[] buffer;
    }

    void LoadImages() {
        if(!gBlockGHandles.empty()) return;

        for(string path : gFilePaths) {
            GHandle gh = gPGame->LoadImageFile(path, true);

            if(gh == ILFAILED) {
                cerr << "LoadImage(): Failed to load file " << path << endl;
                return;
            }

            gBlockGHandles.push_back(gh);
        }
    }

    GHandle GetBlockGHandle(BLOCK_ID blockId) {
        if(!(BLOCK_ID::BID_NONE < blockId && blockId < BLOCK_ID::BID_NUM)) {
            return 0;
        }
        return gBlockGHandles[blockId - 1];
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

                if(gStage[index] == BLOCK_ID::BID_NONE) {
                    gPGame->DrawBox(bx, by, bx + bsize, by + bsize, 0x888888, false);
                } else {
                    gPGame->DrawResizedImage(GetBlockGHandle(gStage[index]), bx, by, bx + bsize, by + bsize, true);
                }

                if(index == gSelectedBlock) {
                    gPGame->DrawBox(bx, by, bx + bsize, by + bsize, 0xFFFFFF, true, 0.5);
                }
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
        for(int i = 0; i < gBlockGHandles.size(); i++) {
            int x = sX + i % PALETTE_BLOCK_COL * (int)((1 + PALETTE_BLOCK_SPACE) * blockSize);
            int y = sY + i / PALETTE_BLOCK_COL * (int)((1 + PALETTE_BLOCK_SPACE) * blockSize);
            GHandle gh = gBlockGHandles[i];

            gPGame->DrawImage(gh, x, y, true);
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
        if(gPGame->IsDown('o')) { // open file
            TCHAR szFileName[512];
            ShowOpenFileDialog(szFileName);
            if(szFileName[0] != '\0') {
                LoadStageFile(szFileName);
            }
        }
        if(gPGame->IsDown('p')) { // save file
            TCHAR szFileName[512];
            ShowSaveFileDialog(szFileName);
            if(szFileName[0] != '\0') {
                WriteStageFile(szFileName);
            }
        }
        
    }

    void Update() {
        Move();
        Draw();
        gPGame->Update();
    }

    void SetCurrentBlock(BLOCK_ID bid) {
        gCurrentBlockType  = bid;
    }

    void PutBlock(unsigned int index, BLOCK_ID bid) {
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

    void ClickBlockPanel(int button, int x, int y) {
        if(x > (gWidth * (1 - PALETTE_W))){
            return;
        }

        MoveOnBlockPanel(x, y);

        if(button == GLUT_LEFT_BUTTON) {
            PutBlock(gSelectedBlock, gCurrentBlockType);
        } else if(button == GLUT_RIGHT_BUTTON) {
            PutBlock(gSelectedBlock, BID_NONE);
        }
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

        int i = y / (blockSize + blockSpace) * PALETTE_BLOCK_COL + x / (blockSize + blockSpace) + 1;

        if(i >= BLOCK_ID::BID_NUM) {
            return;
        }

        gCurrentBlockType = (BLOCK_ID)i;
    }

    void Mouse(int button, int state, int x, int y) {
        gPressedMouseButton = button;
        if(state == GLUT_DOWN) {
            ClickBlockPanel(button, x, y);
            ClickPalette(x, y);
        }
    }

    void MouseMove(int x, int y) {
        MoveOnBlockPanel(x, y);
        MoveOnPalette(x, y);
    }

    void MouseDrag(int x, int y) {
        ClickBlockPanel(gPressedMouseButton, x, y);
        ClickPalette(x, y);
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
        delete[] gStage;
    }
    gPGame->Destroy();
}

void Editor:: SetStageSize(unsigned int width, unsigned int height) {
    gStageWidth = width;
    gStageHeight = height;

    if(gStage != nullptr) {
        delete[] gStage;
    }

    gStage = new BLOCK_ID[width * height];
    this->ResetStage();
}

void Editor::ResetStage() {
    for(int i = 0; i < gStageWidth * gStageHeight; i++) {
        gStage[i] = BID_NONE;
    }
}
