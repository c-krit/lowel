/*
    Copyright (c) 2021 jdeokkim

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
*/

#include <math.h>

#include "raylib.h"
#include "lowel.h"

#define TARGET_FPS 60

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480

#define _DEG2RAD(x) ((x) * DEG2RAD)
#define _RAD2DEG(x) ((x) * RAD2DEG)

typedef struct _semo {
    float width;
    float height;
    float angle;
    Vector2 pos;
} Semo;

static LwMap map_bermuda = {
    .load_texture = LoadTexture
};

static Semo semo = {
    .width = 32.0f,
    .height = 30.0f,
    .angle = 90.0f
};

static Camera2D cam2d_semo = {
    .offset = { 
        SCREEN_WIDTH / 2.0f,
        SCREEN_HEIGHT / 2.0f
    },
    .rotation = 0.0f,
    .zoom = 1.0f
};

/* 게임에 필요한 리소스 파일을 불러온다. */
void LoadResources(void);

/* 게임에 사용된 리소스 파일의 메모리를 해제한다. */
void UnloadResources(void);

/* 게임 화면에 세모를 그린다. */
void DrawSemo(Semo *semo);

/* 키보드 입력을 받아 세모를 움직인다. */
void HandleMovement(Semo *semo);

/* 게임 맵 카메라를 업데이트한다. */
void UpdateMapCamera(void);

/* 게임 화면을 업데이트한다. */
void UpdateCurrentScreen(void);

int main(void) {
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    SetTargetFPS(TARGET_FPS);
    
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "[lowel] example: bermuda");
    
    LoadResources();
    
    while (!WindowShouldClose())
        UpdateCurrentScreen();
    
    UnloadResources();
    
    CloseWindow();

    return 0;
}

/* 게임에 필요한 리소스 파일을 불러온다. */
void LoadResources(void) {
    LoadMap(&map_bermuda, "../res/maps/non_tiled.json");
    
    semo.pos = cam2d_semo.target = (Vector2) {
        (map_bermuda.width.px) / 2.0f,
        (map_bermuda.height.px) / 2.0f
    };
}

/* 게임에 사용된 리소스 파일의 메모리를 해제한다. */
void UnloadResources(void) {
    UnloadMap(&map_bermuda);
}

/* 게임 화면에 세모를 그린다. */
void DrawSemo(Semo *semo) {
    DrawTriangle(
        (Vector2) {
            SCREEN_WIDTH / 2,
            SCREEN_HEIGHT / 2 - (semo->height / 2.0f)
        },
        (Vector2) {
            SCREEN_WIDTH / 2 - (semo->width / 2.0f),
            SCREEN_HEIGHT / 2 + (semo->height / 2.0f)
        },
        (Vector2) {
            SCREEN_WIDTH / 2 + (semo->width / 2.0f),
            SCREEN_HEIGHT / 2 + (semo->height / 2.0f)
        },
        WHITE
    );
}

/* 키보드 입력을 받아 세모를 움직인다. */
void HandleMovement(Semo *semo) {
    static float speed;
    
    if (IsKeyDown(KEY_UP))
        speed = 2.45f;
    else if (IsKeyDown(KEY_DOWN))
        speed = -2.45f;
    else
        speed = 0.0f;
    
    if (IsKeyDown(KEY_LEFT))
        semo->angle += 1.45f;
    else if (IsKeyDown(KEY_RIGHT))
        semo->angle -= 1.45f;
    else;
    
    cam2d_semo.rotation = semo->angle - 90.0f;
    
    semo->pos.x += speed * cos(_DEG2RAD(semo->angle));
    semo->pos.y -= speed * sin(_DEG2RAD(semo->angle));
}

/* 게임 맵 카메라를 업데이트한다. */
void UpdateMapCamera(void) {
    cam2d_semo.offset = (Vector2) { 
        SCREEN_WIDTH / 2.0f,
        SCREEN_HEIGHT / 2.0f
    };
    cam2d_semo.target = semo.pos;
}

/* 게임 화면을 업데이트한다. */
void UpdateCurrentScreen(void) {
    BeginDrawing();
    
    BeginMode2D(cam2d_semo);
    
    ClearBackground(BLACK);
    DrawMap(&map_bermuda, semo.pos);
    UpdateMapCamera();
    
    EndMode2D();
    
    DrawSemo(&semo);
    HandleMovement(&semo);
    
    DrawFPS(8, 8);
    
    EndDrawing();
}