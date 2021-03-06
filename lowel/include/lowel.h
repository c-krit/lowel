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

#ifndef LOWEL_H
#define LOWEL_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "../src/json.h"
#include "raylib.h"

#define LOWEL_VERSION "1.0.0"
#define MAP_FORMAT_VERSION "1.0.0"

#define MAX_STRING_LENGTH 256

#define MAX_DRAW_DISTANCE 32
#define MAX_LAYER_COUNT 32
#define MAX_OBJECT_COUNT 128

/* 청크 (일정하게 분할된 맵의 일부분)를 나타내는 구조체. */
typedef struct LwChunk LwChunk;

/* 개체를 나타내는 구조체. */
typedef struct LwObject LwObject;

/* 
    레이어를 나타내는 구조체.
    
    `objects`: 게임 맵을 그릴 때 필요한 개체의 배열을 나타낸다.
*/
typedef struct LwLayer {
    bool _valid;
    LwObject *objects;
} LwLayer;

/* 
    게임 맵에서 사용할 단위를 나타내는 구조체.
    
    `px`: 픽셀 단위로 환산된 값을 나타낸다.
    `c`:  청크 단위로 환산된 값을 나타낸다.
    `t`:  타일 단위로 환산된 값을 나타낸다.
*/
typedef struct LwMapUnit {
    int px;
    int c;
    int t;
} LwMapUnit;

/* 
    게임 맵을 나타내는 구조체.
    
    `name`:          게임 맵의 이름을 나타낸다.
    `width`:         게임 맵의 가로 길이를 나타내며, 단위는 `픽셀`, `청크` 또는 `타일`이다.
    `height`:        게임 맵의 세로 길이를 나타내며, 단위는 `픽셀`, `청크` 또는 `타일`이다.
    `chunk_width`:   청크의 가로 길이를 나타내며, 단위는 `타일`이다.
    `chunk_height`:  청크의 세로 길이를 나타내며, 단위는 `타일`이다.
    `tile_width`:    타일의 가로 길이를 나타내며, 단위는 `픽셀`이다.
    `tile_height`:   타일의 세로 길이를 나타내며, 단위는 `픽셀`이다.
    `draw_distance`: 플레이어의 위치를 기준으로 주변 몇 칸까지 떨어진 청크를 로드할 것인지를 나타낸다. 
                     (예: `draw_distance`가 0일 경우, 플레이어의 위치에 해당하는 청크 단 하나만 
                     로드한다. `draw_distance`가 1일 경우, 플레이어의 위치에 해당하는 청크와 
                     그 주변의 청크 1개씩, 총 9개의 청크를 로드한다. 따라서 `draw_distance`가 
                     `k`일 경우, 총 `(2k + 1)^2`개의 청크를 로드하게 된다.)
    `object_table`:  개체의 고유 번호와 개체가 속한 레이어의 고유 번호가 저장된 배열이다.
    `layers`:        게임 맵을 그릴 때 필요한 레이어의 배열을 나타낸다.
    `load_texture`:  게임 맵의 텍스처 데이터를 불러올 때 사용할 함수의 포인터이다.
*/
typedef struct LwMap {
    char *name;
    LwMapUnit width;
    LwMapUnit height;
    int chunk_width;
    int chunk_height;
    int tile_width;
    int tile_height;
    int draw_distance;
    int *object_table;
    LwLayer *layers;
    Texture2D (*load_texture)(const char *);
} LwMap;

/* ::: 게임 맵 관련 함수 ::: */

/* 파일에서 게임 맵 데이터를 불러온다. */
bool LoadMap(LwMap *map, const char *file_path);

/* 메모리에서 게임 맵 데이터를 불러온다. */
bool LoadMapFromMemory(LwMap *map, char *map_data);

/* 위치 `position`을 기준으로 게임 맵을 화면에 그린다. */
void DrawMap(LwMap *map, Vector2 position);

/* 위치 `position`이 게임 맵 안쪽에 해당하는 위치인지 확인한다. */
bool IsInsideMap(LwMap *map, Vector2 position);

/* 게임 맵 데이터를 파일에 저장한다. */
bool SaveMap(LwMap *map, const char *file_path);

/* 게임 맵 데이터를 메모리에 저장한다. */
bool SaveMapToMemory(LwMap *map, char **map_data);

/* 게임 맵 데이터의 메모리를 해제한다. */
void UnloadMap(LwMap *map);

/* ::: 개체 관련 함수 ::: */

/* 고유 번호가 `index`인 개체의 메모리 주소를 반환한다. */
LwObject *GetObject(LwMap *map, int index);

/* 개체의 현재 위치를 반환한다. */
Vector2 GetObjectPosition(LwObject *object);

/* 개체 텍스처의 가로 길이를 반환한다. */
double GetObjectWidth(LwObject *object);

/* 개체 텍스처의 세로 길이를 반환한다. */
double GetObjectHeight(LwObject *object);

/* 
    위치 `position`을 기준으로 개체 `object`가 `map.draw_distance`에 
    해당하는 거리 안에 속해있는지를 확인한다.
*/
bool IsObjectReadyToDraw(LwMap *map, LwObject *object, Vector2 position);

/* 
    위치 `position`을 기준으로 개체 `object`가 `map.draw_distance`에 
    해당하는 거리 안에 속해있는지를 확인한다.
*/
bool IsObjectReadyToDraw(LwMap *map, LwObject *object, Vector2 position);

/* 개체의 현재 위치를 `position`으로 변경한다. */
void SetObjectPosition(LwObject *object, Vector2 position);

/* ::: 청크 관련 함수 ::: */

/* 게임 맵 또는 개체 텍스처에서 고유 번호가 `index`인 청크를 게임 화면에 그린다. */
void DrawChunk(LwMap *map, LwObject *object, int index);

/* 위치 `position`의 주변에 있는 청크를 모두 로드한다. */
void LoadChunks(LwMap *map, LwObject *object, Vector2 position);

/* 거리가 `map.draw_distance` 이하인 청크의 최대 개수를 반환한다. */
int GetAdjacentChunkCount(LwMap *map);

/* 
    고유 번호가 `index`인 청크에서 거리가 `map.draw_distance` 이하인 모든 청크의 고유 번호를 
    구한 다음, `object.chunk_indexes`에 저장한다.
*/
void UpdateAdjacentChunkIndexes(LwMap *map, LwObject *object, int index);

/* 게임 맵에서 고유 번호가 `index`인 청크의 청크 기준 X좌표를 반환한다. */
int GetMapChunkX(LwMap *map, int index);

/* 개체 텍스처에서 고유 번호가 `index`인 청크의 청크 기준 X좌표를 반환한다. */
int GetObjectChunkX(LwObject *object, int index);

/* 게임 맵에서 고유 번호가 `index`인 청크의 청크 기준 Y좌표를 반환한다. */
int GetMapChunkY(LwMap *map, int index);

/* 개체 텍스처에서 고유 번호가 `index`인 청크의 청크 기준 Y좌표를 반환한다. */
int GetObjectChunkY(LwObject *object, int index);

/* 게임 맵에서 고유 번호가 `index`인 타일의 타일 기준 X좌표를 반환한다. */
int GetMapTileX(LwMap *map, int index);

/* 개체 텍스처에서 고유 번호가 `index`인 타일의 타일 기준 X좌표를 반환한다. */
int GetObjectTileX(LwObject *object, int index);

/* 게임 맵에서 고유 번호가 `index`인 타일의 타일 기준 Y좌표를 반환한다. */
int GetMapTileY(LwMap *map, int index);

/* 개체 텍스처에서 고유 번호가 `index`인 타일의 타일 기준 Y좌표를 반환한다. */
int GetObjectTileY(LwObject *object, int index);

/* 게임 맵에서 고유 번호가 `index`인 청크의 시작 위치를 구한다. */
Vector2 ChunkIndexToPositionMap(LwMap *map, int index);

/* 개체 텍스처에서 고유 번호가 `index`인 청크의 시작 위치를 구한다. */
Vector2 ChunkIndexToPositionObject(LwMap *map, LwObject *object, int index);

/* 게임 맵에서 위치 `position`이 속한 청크의 인덱스를 구한다. */
int PositionToChunkIndexMap(LwMap *map, Vector2 position);

/* 개체 텍스처에서 위치 `position`이 속한 청크의 인덱스를 구한다. */
int PositionToChunkIndexObject(LwMap *map, LwObject *object, Vector2 position);

/* 게임 맵에서 위치 `position`이 속한 타일의 인덱스를 구한다. */
int PositionToTileIndexMap(LwMap *map, Vector2 position);

/* 개체 텍스처에서 위치 `position`이 속한 타일의 인덱스를 구한다. */
int PositionToTileIndexObject(LwMap *map, LwObject *object, Vector2 position);

/* 게임 맵에서 고유 번호가 `chunk_index`인 청크의 `relative_tile_index + 1`번째 타일의 시작 위치를 구한다. */
Vector2 RelativeTileIndexToPositionMap(
    LwMap *map, 
    int chunk_index, int relative_tile_index
);

/* 개체 텍스처에서 고유 번호가 `chunk_index`인 청크의 `relative_tile_index + 1`번째 타일의 시작 위치를 구한다. */
Vector2 RelativeTileIndexToPositionObject(
    LwMap *map, LwObject *object, 
    int chunk_index, int relative_tile_index
);

/* 고유 번호가 `index`인 타일이 속한 청크의 인덱스를 구한다. */
int TileIndexToChunkIndexMap(LwMap *map, int index);

/* 고유 번호가 `index`인 타일이 속한 청크의 인덱스를 구한다. */
int TileIndexToChunkIndexObject(LwMap *map, LwObject *object, int index);

/* 고유 번호가 `index`인 타일의 시작 위치를 구한다. */
Vector2 TileIndexToPositionMap(LwMap *map, int index);

/* 고유 번호가 `index`인 타일의 시작 위치를 구한다. */
Vector2 TileIndexToPositionObject(LwMap *map, LwObject *object, int index);

#endif