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

#include "../include/lowel.h"

#define LW_MIN(x, y) (((x) < (y)) ? (x) : (y))
#define LW_MAX(x, y) (((x) > (y)) ? (x) : (y))

/* 
    청크 (일정하게 분할된 맵의 일부분)를 나타내는 구조체.
    
    `data`: 청크의 타일 데이터를 나타낸다.
*/
struct LwChunk {
    bool _valid;
    int *data;
};

/* 
    청크를 관리하는 역할을 하는 구조체.
    
    `tilemap`:    게임 맵 전체의 타일 데이터를 나타낸다.
    `indexes`:    다음에 그릴 모든 청크의 고유 번호를 나타낸다.
    `temp_index`: 플레이어의 위치가 속한 청크의 고유 번호를 나타낸다.
    `chunks`:     청크의 배열을 나타낸다.
*/
typedef struct LwChunkSet {
    int *tilemap;
    int *indexes;
    int temp_index;
    LwChunk *chunks;
} LwChunkSet;

/* 
    개체를 나타내는 구조체.
    
    `_valid`:     라이브러리 내부에서 사용되는 변수이다.
    `id`:         개체의 고유 번호이다.
    `image_path`: 이미지 파일의 경로이다.
    `texture`:    개체의 텍스처를 나타낸다.
    `width`:      개체의 가로 길이를 나타내며, 단위는 `픽셀`, `청크` 또는 `타일`이다.
    `height`:     개체의 세로 길이를 나타내며, 단위는 `픽셀`, `청크` 또는 `타일`이다.
    `auto_split`: `tileset`의 값이 `false`일 때만 적용된다. 
                  `true`일 경우 텍스처를 자동으로 분할하여 `[ 0, 1, 2, 3, 4, ... ]`와 같이
                  `tiledata`에 값을 순차적으로 채운다.
    `tileset`:    개체의 텍스처가 타일셋이면 `true`, 아니라면 `false`이다.
    `scale`:      개체의 크기를 몇 배 늘리거나 줄일 것인지를 나타낸다.
    `rotation`:   개체를 몇 도 (`deg.`)만큼 회전시킬지를 결정한다.
    `position`:   개체의 위치를 나타낸다.
    `chunkset`:   개체의 청크를 관리하는 구조체이다.
*/
struct LwObject {
    bool _valid;
    int id;
    char *image_path;
    Texture2D texture;
    LwMapUnit width;
    LwMapUnit height;
    bool tileset;
    bool auto_split;
    double scale;
    double rotation;
    Vector2 position;
    LwChunkSet chunkset;
};

/* ::: 소스 파일 내부 함수 ::: */

/* 게임 맵의 `header` 노드에 포함된 데이터를 불러온다. */
static bool LoadHeaderData(LwMap *map, JsonNode **node_current) {
    JsonNode *node_header;
    
    map->name = (char *) RL_CALLOC(
        MAX_STRING_LENGTH, 
        sizeof(char)
    );
    
    json_foreach(node_header, *node_current) {
        if (TextIsEqual(node_header->key, "name")) {
            if (node_header->string_ == NULL || TextLength(node_header->string_) == 0) {
                TraceLog(
                    LOG_ERROR, 
                    "LOWEL: Failed to load map data: invalid value for `name` in `header`"
                );

                return false;
            }
            
            TextCopy(map->name, node_header->string_);
        } else if (TextIsEqual(node_header->key, "format_version")) {
            if (node_header->string_ == NULL || TextLength(node_header->string_) == 0) {
                TraceLog(
                    LOG_ERROR, 
                    "LOWEL: Failed to load map data: invalid value for `format_version` in `header`"
                );

                return false;
            } else if (!TextIsEqual(node_header->string_, MAP_FORMAT_VERSION)) {
                TraceLog(
                    LOG_ERROR, 
                    "LOWEL: Failed to load map data: map format version mismatch"
                );

                return false;
            }
        }
    }
    
    *node_current = (*node_current)->next;
    
    return true;
}

/* 게임 맵의 `options` 노드에 포함된 데이터를 불러온다. */
static bool LoadOptionsData(LwMap *map, JsonNode **node_current) {
    JsonNode *node_options;
    
    json_foreach(node_options, *node_current) {
        if (TextIsEqual(node_options->key, "width")) {
            map->width.px = (int) node_options->number_;
        } else if (TextIsEqual(node_options->key, "height")) {
            map->height.px = (int) node_options->number_; 
        } else if (TextIsEqual(node_options->key, "tile_width")) {
            map->tile_width = (int) node_options->number_;
        } else if (TextIsEqual(node_options->key, "tile_height")) {
            map->tile_height = (int) node_options->number_;
        } else if (TextIsEqual(node_options->key, "chunk_width_t")) {
            map->chunk_width = (int) node_options->number_; 
        } else if (TextIsEqual(node_options->key, "chunk_height_t")) {
            map->chunk_height = (int) node_options->number_;
        } else if (TextIsEqual(node_options->key, "draw_distance_c")) {
            map->draw_distance = (int) node_options->number_;
        }
    }
    
    if (map->width.px <= 0 || map->height.px <= 0
       || map->tile_width <= 0 || map->tile_height <= 0
       || map->chunk_width <= 0 || map->chunk_height <= 0) {
        TraceLog(
            LOG_ERROR, 
            "LOWEL: [MAP '%s'] Failed to load map data: "
            "the value of width/height property must not be negative",
            map->name
        );
        
        return false;
    }
    
    if (map->draw_distance < 0) {
        TraceLog(
            LOG_ERROR, 
            "LOWEL: [MAP '%s'] Failed to load map data: "
            "the value of `draw_distance` must not be negative",
            map->name
        );
        
        return false;
    } else if (map->draw_distance > MAX_DRAW_DISTANCE) {
        TraceLog(
            LOG_WARNING, 
            "LOWEL: [MAP '%s'] The value of `draw_distance` must be less than or equal to %d",
            map->name,
            MAX_DRAW_DISTANCE
        );
        TraceLog(
            LOG_WARNING, 
            "LOWEL: [MAP '%s'] Setting the value of `draw_distance` to %d automatically",
            map->name,
            MAX_DRAW_DISTANCE
        );
        
        map->draw_distance = 16;
    }
    
    map->width.t = map->width.px / map->tile_width; 
    map->height.t = map->height.px / map->tile_height;
    
    map->width.c = (map->width.t % map->chunk_width) 
        ? (map->width.t / map->chunk_width) + 1
        : (map->width.t / map->chunk_width);
    map->height.c = (map->height.t % map->chunk_height)
        ? (map->height.t / map->chunk_height) + 1
        : (map->height.t / map->chunk_height);
    
    *node_current = (*node_current)->next;
    
    return true;
}

/* 개체의 타일 데이터를 청크 배열로 변환하여, `object.chunkset.chunks`에 저장한다. */
static bool LoadTileData(LwMap *map, LwObject *object, int *tiledata) {
    int chunk_index, relative_tile_index;
    
    if (!object->tileset && !object->auto_split) {
        return true;
    } else if (object->tileset && !object->auto_split) {
        object->position = (Vector2) { 0 };
        
        object->chunkset.tilemap = (int *) RL_CALLOC(
            map->width.t * map->height.t,
            sizeof(int)
        );
        
        for (int i = 0; i < map->width.t * map->height.t; i++)
            object->chunkset.tilemap[i] = tiledata[i];
        
        RL_FREE(tiledata);
        
        object->chunkset.indexes = (int *) RL_CALLOC(
            GetAdjacentChunkCount(map),
            sizeof(int)
        );
        
        object->chunkset.temp_index = -1;
        
        object->chunkset.chunks = (LwChunk *) RL_CALLOC(
            map->width.c * map->height.c,
            sizeof(LwChunk)
        );
        
        for (int i = 0; i < (map->width.c * map->height.c); i++)
            object->chunkset.chunks[i].data = (int *) RL_CALLOC(
                map->chunk_width * map->chunk_height,
                sizeof(int)
            );
        
        for (int abs_tile_index = 0; 
             abs_tile_index < (map->width.t * map->height.t);
             abs_tile_index++) {
            if (object->chunkset.tilemap[abs_tile_index] < 0)
                continue;
            
            chunk_index = TileIndexToChunkIndexMap(map, abs_tile_index);
            relative_tile_index = (((abs_tile_index / map->width.t) % map->chunk_width) * map->chunk_width)
                + ((abs_tile_index % map->width.t) % map->chunk_width);
            
            if (!object->chunkset.chunks[chunk_index]._valid)
                object->chunkset.chunks[chunk_index]._valid = true;
            
            object->chunkset.chunks[chunk_index]
                .data[relative_tile_index] = object->chunkset.tilemap[abs_tile_index];
        }
        
        return true;
    } else if (!object->tileset && object->auto_split) {
        object->chunkset.tilemap = (int *) RL_CALLOC(
            object->width.t * object->height.t,
            sizeof(int)
        );
        
        for (int i = 0; i < object->width.t * object->height.t; i++)
            object->chunkset.tilemap[i] = i;
        
        object->chunkset.indexes = (int *) RL_CALLOC(
            GetAdjacentChunkCount(map),
            sizeof(int)
        );
        
        object->chunkset.temp_index = -1;
        
        object->chunkset.chunks = (LwChunk *) RL_CALLOC(
            object->width.c * object->height.c,
            sizeof(LwChunk)
        );
        
        for (int i = 0; i < (object->width.c * object->height.c); i++)
            object->chunkset.chunks[i].data = (int *) RL_CALLOC(
                map->chunk_width * map->chunk_height,
                sizeof(int)
            );
        
        for (int abs_tile_index = 0; 
             abs_tile_index < (object->width.t * object->height.t); 
             abs_tile_index++) {
            if (object->chunkset.tilemap[abs_tile_index] < 0)
                continue;
            
            chunk_index = TileIndexToChunkIndexObject(map, object, abs_tile_index);
            relative_tile_index = (((abs_tile_index / object->width.t) % map->chunk_width) * map->chunk_width)
                + ((abs_tile_index % object->width.t) % map->chunk_width);
            
            if (!object->chunkset.chunks[chunk_index]._valid)
                object->chunkset.chunks[chunk_index]._valid = true;
            
            object->chunkset.chunks[chunk_index]
                .data[relative_tile_index] = object->chunkset.tilemap[abs_tile_index];
        }
        
        return true;
    } else {
        return false;
    }
}

/* 게임 맵 레이어의 `objects` 노드에 포함된 데이터를 불러온다. */
static bool LoadObjectsData(LwMap *map, JsonNode *node_layer, int layer_id) {
    JsonNode *node_objects, *node_object;
    JsonNode *node_offset, *node_tiledata;
    
    LwObject *object;
    
    int *tiledata;
    
    int object_id = -1;
    int tile_index = 0;
    
    map->object_table = (int *) RL_CALLOC(
        MAX_OBJECT_COUNT,
        sizeof(int)
    );
    
    json_foreach(node_objects, node_layer) {
        json_foreach(node_object, node_objects) {
            if (TextIsEqual(node_object->key, "id")) {
                object_id = (int) node_object->number_;
                
                map->object_table[object_id] = layer_id;
                                    
                map->layers[layer_id].objects[object_id]._valid = true;
                map->layers[layer_id].objects[object_id].id = object_id;
            } else {
                object = &map->layers[layer_id].objects[object_id];
                
                if (object_id < 0 || object_id > MAX_OBJECT_COUNT - 1) {
                    TraceLog(
                        LOG_ERROR, 
                        "LOWEL: [MAP '%s'] Failed to load map data: invalid value "
                        "for `object_id` in `objects`",
                        map->name
                    );

                    return false;
                } else {
                    if (TextIsEqual(node_object->key, "image")) {
                        object->image_path = (char *) RL_CALLOC(
                            MAX_STRING_LENGTH,
                            sizeof(char)
                        );

                        TextCopy(object->image_path, node_object->string_);

                        if (map->load_texture != NULL) {
                            TraceLog(
                                LOG_INFO, 
                                "LOWEL: [MAP '%s': %s] Attempting to load texture for object #%d",
                                map->name,
                                object->image_path,
                                object_id
                            );

                            map->layers[layer_id].objects[object_id].texture = map->load_texture(
                                object->image_path
                            );
                        }
                    } else if (TextIsEqual(node_object->key, "tileset")) {
                        object->tileset = node_object->bool_;
                    } else if (TextIsEqual(node_object->key, "auto_split")) {
                        object->auto_split = node_object->bool_;
                    } else if (TextIsEqual(node_object->key, "scale_mul")) {
                        object->scale = node_object->number_;
                        
                        object->width.px = object->texture.width * object->scale;
                        object->height.px = object->texture.height * object->scale;

                        object->width.t = object->width.px / map->tile_width;
                        object->height.t = object->height.px / map->tile_height;

                        object->width.c = (object->width.t % map->chunk_width) 
                            ? (object->width.t / map->chunk_width) + 1
                            : (object->width.t / map->chunk_width);
                        object->height.c = (object->height.t % map->chunk_height)
                            ? (object->height.t / map->chunk_height) + 1
                            : (object->height.t / map->chunk_height);
                    } else if (TextIsEqual(node_object->key, "rotation_deg")) {
                        object->rotation = node_object->number_;
                    } else if (TextIsEqual(node_object->key, "position")) {
                        json_foreach(node_offset, node_object) {
                            if (TextIsEqual(node_offset->key, "x"))
                                object->position.x = node_offset->number_;
                            else if (TextIsEqual(node_offset->key, "y"))
                                object->position.y = node_offset->number_;
                        }
                    } else if (TextIsEqual(node_object->key, "tiledata")) {
                        tiledata = NULL;
                        tile_index = 0;
                        
                        if (object->tileset && !object->auto_split) {
                            tiledata = (int *) RL_CALLOC(
                                map->width.t * map->height.t, 
                                sizeof(int)
                            );
                            
                            for (int i = 0; i < map->width.t * map->height.t; i++)
                                tiledata[i] = -1;

                            json_foreach(node_tiledata, node_object)
                                tiledata[tile_index++] = (int) node_tiledata->number_;
                        }

                        if (!LoadTileData(map, object, tiledata)) {
                            TraceLog(
                                LOG_ERROR, 
                                "LOWEL: [MAP '%s'] Failed to load map data: unable to load "
                                "`tiledata` for object #%d",
                                map->name,
                                object_id
                            );

                            return false;
                        }
                    }
                }
            }
        }
    }
}

/* 게임 맵의 `layers` 노드에 포함된 데이터를 불러온다. */
static bool LoadLayersData(LwMap *map, JsonNode **node_current) {
    JsonNode *node_layers, *node_layer;
    
    int layer_id = -1;
    
    map->layers = (LwLayer *) RL_CALLOC(
        MAX_LAYER_COUNT, 
        sizeof(LwLayer)
    );
    
    json_foreach(node_layers, *node_current) {
        json_foreach(node_layer, node_layers) {
            if (TextIsEqual(node_layer->key, "id")) {
                layer_id = (int) node_layer->number_;
                
                map->layers[layer_id]._valid = true;
            } else {
                if (layer_id < 0 || layer_id > MAX_LAYER_COUNT) {
                    TraceLog(
                        LOG_ERROR, 
                        "LOWEL: [MAP '%s'] Failed to load map data: invalid value "
                        "for `layer_id` in `layers`",
                        map->name
                    );
                    
                    return false;
                } else {
                    if (TextIsEqual(node_layer->key, "objects")) {
                        map->layers[layer_id].objects = (LwObject *) RL_CALLOC(
                            MAX_OBJECT_COUNT, 
                            sizeof(LwObject)
                        );
                        
                        LoadObjectsData(map, node_layer, layer_id);
                    }
                }
            }
        }
    }
    
    return true;
}

/* 게임 맵의 `header` 노드에 해당하는 부분을 저장한다. */
static bool SaveHeaderData(LwMap *map, JsonNode *node_root) {
    JsonNode *node_header;
    
    node_header = json_mkobject();
    
    json_append_member(node_header, "name", json_mkstring(map->name));
    json_append_member(node_header, "format_version", json_mkstring(MAP_FORMAT_VERSION));
    
    json_append_member(node_root, "header", node_header);
    
    return true;
}

/* 게임 맵의 `options` 노드에 해당하는 부분을 저장한다. */
static bool SaveOptionsData(LwMap *map, JsonNode *node_root) {
    JsonNode *node_options;
    
    node_options = json_mkobject();
    
    json_append_member(node_options, "width", json_mknumber(map->width.px));
    json_append_member(node_options, "height", json_mknumber(map->height.px));
    json_append_member(node_options, "tile_width", json_mknumber(map->tile_width));
    json_append_member(node_options, "tile_height", json_mknumber(map->tile_height));
    json_append_member(node_options, "chunk_width_t", json_mknumber(map->chunk_width));
    json_append_member(node_options, "chunk_height_t", json_mknumber(map->chunk_height));
    json_append_member(node_options, "draw_distance_c", json_mknumber(map->draw_distance));
    
    json_append_member(node_root, "options", node_options);
    
    return true;
}

/* 게임 맵의 `layers` 노드에 해당하는 부분을 저장한다. */
static bool SaveLayersData(LwMap *map, JsonNode *node_root) {
    JsonNode *node_layers, *node_layer;
    JsonNode *node_objects, *node_object;
    JsonNode *node_position, *node_tiledata;
    JsonNode *node_center, *node_vertices, *node_vertex;
    
    node_layers = json_mkarray();
    
    for (int i = 0; i < MAX_LAYER_COUNT; i++) {
        if (!map->layers[i]._valid)
            continue;
        
        node_layer = json_mkobject();
        node_objects = json_mkarray();
        
        for (int j = 0; j < MAX_OBJECT_COUNT; j++) {
            if (!map->layers[i].objects[j]._valid)
                continue;
            
            node_object = json_mkobject();
            
            node_position = json_mkobject();
            node_tiledata = json_mkarray();
            
            json_append_member(
                node_position, 
                "x", 
                json_mknumber(map->layers[i].objects[j].position.x)
            );
            json_append_member(
                node_position, 
                "y", 
                json_mknumber(map->layers[i].objects[j].position.y)
            );
            
            if (map->layers[i].objects[j].tileset
               && !map->layers[i].objects[j].auto_split)
                for (int k = 0; k < map->width.t * map->height.t; k++)
                    json_append_element(
                        node_tiledata, 
                        json_mknumber(map->layers[i].objects[j].chunkset.tilemap[k])
                    );
            
            json_append_member(
                node_object, 
                "id", 
                json_mknumber(j)
            );
            json_append_member(
                node_object, 
                "image", 
                json_mkstring(map->layers[i].objects[j].image_path)
            );
            json_append_member(
                node_object, 
                "tileset",
                json_mkbool(map->layers[i].objects[j].tileset)
            );
            json_append_member(
                node_object, 
                "auto_split",
                json_mkbool(map->layers[i].objects[j].auto_split)
            );
            json_append_member(
                node_object, 
                "scale_mul",
                json_mknumber(map->layers[i].objects[j].scale)
            );
            json_append_member(
                node_object, 
                "rotation_deg",
                json_mknumber(map->layers[i].objects[j].rotation)
            );
            
            json_append_member(
                node_object, 
                "position",
                node_position
            );
            json_append_member(
                node_object,
                "tiledata",
                node_tiledata
            );
            
            json_append_element(node_objects, node_object);
        }
        
        json_append_member(node_layer, "id", json_mknumber(i));
        json_append_member(node_layer, "objects", node_objects);
        
        json_append_element(node_layers, node_layer);
    }
    
    json_append_member(node_root, "layers", node_layers);
    
    return true;
}

/* ::: 게임 맵 관련 함수 ::: */

/* 파일에서 게임 맵 데이터를 불러온다. */
bool LoadMap(LwMap *map, const char *file_path) {
    char *map_data;
    
    if ((map_data = (char *) LoadFileText(file_path)) == NULL)
        return false;
    
    return LoadMapFromMemory(map, map_data);
}

/* 메모리에서 게임 맵 데이터를 불러온다. */
bool LoadMapFromMemory(LwMap *map, char *map_data) {
    JsonNode *node_root, *node_current;
    
    node_root = json_decode(map_data);

    if (node_root == NULL) {
        TraceLog(
            LOG_ERROR, 
            "LOWEL: Failed to load map data: `json_decode()` error"
        );
        
        return false;
    }
    
    node_current = json_first_child(node_root);
    
    if (!LoadHeaderData(map, &node_current) 
        || !LoadOptionsData(map, &node_current)
        || !LoadLayersData(map, &node_current))
        return false;
  
    TraceLog(
        LOG_INFO, 
        "LOWEL: [MAP '%s'] Loaded map data successfully",
        map->name
    );
    
    return true;
}

/* 위치 `position`을 기준으로 게임 맵을 화면에 그린다. */
void DrawMap(LwMap *map, Vector2 position) {
    LwObject *object;
    
    for (int i = 0; i < MAX_LAYER_COUNT; i++) {
        if (!map->layers[i]._valid)
            continue;
        
        for (int j = 0; j < MAX_OBJECT_COUNT; j++) {
            object = &map->layers[i].objects[j];
            
            if (!object->_valid || !object->texture.id)
                continue;
            
            if (object->tileset && !object->auto_split
                || !object->tileset && object->auto_split) {
                LoadChunks(map, object, position);
            } else {
                DrawTextureEx(
                    object->texture,
                    object->position,
                    object->rotation,
                    object->scale,
                    WHITE
                );
            }
        }
    }
}

/* 게임 맵 데이터를 파일에 저장한다. */
bool SaveMap(LwMap *map, const char *file_path) {
    char *map_data;
    
    if (!SaveMapToMemory(map, &map_data))
        return false;
    
    return SaveFileText(file_path, map_data);
}

/* 게임 맵 데이터를 메모리에 저장한다. */
bool SaveMapToMemory(LwMap *map, char **map_data) {
    JsonNode *node_root, *node_current;
    
    node_root = json_mkobject();
    
    if (!SaveHeaderData(map, node_root)
        || !SaveOptionsData(map, node_root)
        || !SaveLayersData(map, node_root))
        return false;
    
    *map_data = json_encode(node_root);
    
    return true;
}

/* 게임 맵 데이터의 메모리를 해제한다. */
void UnloadMap(LwMap *map) {
    LwObject *object;
    
    int max_chunk_count;
    
    for (int i = 0; i < MAX_LAYER_COUNT; i++) {
        if (!map->layers[i]._valid)
            continue;
        
        for (int j = 0; j < MAX_OBJECT_COUNT; j++) {
            if (!map->layers[i].objects[j]._valid)
                continue;
                
            object = &map->layers[i].objects[j];
            
            RL_FREE(object->image_path);
            
            if ((object->tileset && !object->auto_split)
                || (!object->tileset && object->auto_split)) {
                RL_FREE(object->chunkset.tilemap);
                RL_FREE(object->chunkset.indexes);
                
                max_chunk_count = (object->tileset && !object->auto_split)
                    ? (map->width.c * map->height.c)
                    : (object->width.c * object->height.c);

                for (int k = 0; k < max_chunk_count; k++) {
                    if (!object->chunkset.chunks[k]._valid)
                        continue;

                    RL_FREE(object->chunkset.chunks[k].data);
                }
                
                RL_FREE(object->chunkset.chunks);
            }
            
            TraceLog(
                LOG_INFO, 
                "LOWEL: [MAP '%s'] Unloaded object #%d",
                map->name,
                j
            );
        }
        
        RL_FREE(map->layers[i].objects);
        
        TraceLog(
            LOG_INFO, 
            "LOWEL: [MAP '%s'] Unloaded layer #%d",
            map->name,
            i
        );
    }
    RL_FREE(map->object_table);
    RL_FREE(map->layers);
    RL_FREE(map->name);
    
    TraceLog(
        LOG_INFO, 
        "LOWEL: Unloaded map data successfully"
    );
}

/* ::: 개체 관련 함수 ::: */

/* 고유 번호가 `index`인 개체의 메모리 주소를 반환한다. */
LwObject *GetObject(LwMap *map, int index) {
    int layer_id;
    
    layer_id = map->object_table[index];
    
    if (!map->layers[layer_id].objects[index]._valid)
        return NULL;
    
    return &map->layers[layer_id].objects[index];
}

/* 개체의 현재 위치를 반환한다. */
Vector2 GetObjectPosition(LwObject *object) {
    return object->position;
}

/* 개체 텍스처의 가로 길이를 반환한다. */
double GetObjectWidth(LwObject *object) {
    if (!object->_valid || !object->texture.id)
        return 0;
    
    return object->texture.width * object->scale; 
}

/* 개체 텍스처의 세로 길이를 반환한다. */
double GetObjectHeight(LwObject *object) {
    if (!object->_valid || !object->texture.id)
        return 0;
    
    return object->texture.height * object->scale;
}

/* 개체의 현재 위치를 `position`으로 변경한다. */
void SetObjectPosition(LwObject *object, Vector2 position) {
    object->position = position;
}

/* ::: 청크 관련 함수 ::: */

/* 게임 맵 또는 개체 텍스처에서 고유 번호가 `index`인 청크를 게임 화면에 그린다. */
void DrawChunk(LwMap *map, LwObject *object, int index) {
    Vector2 tile_position, orig_texture_position;
    
    int tile_id;
    
    for (int i = 0; i < (map->chunk_width * map->chunk_height); i++) {
        tile_id = object->chunkset.chunks[index].data[i];
        
        tile_position = (object->tileset && !object->auto_split)
            ? RelativeTileIndexToPositionMap(map, index, i)
            : RelativeTileIndexToPositionObject(map, object, index, i);
        
        orig_texture_position = TileIndexToPositionObject(map, object, tile_id);
        
        DrawTextureRec(
            object->texture,
            (Rectangle) {
                orig_texture_position.x - object->position.x,
                orig_texture_position.y - object->position.y,
                map->tile_width,
                map->tile_height
            },
            tile_position,
            WHITE
        );
    }
}

/* 위치 `position`의 주변에 있는 청크를 모두 로드한다. */
void LoadChunks(LwMap *map, LwObject *object, Vector2 position) {
    int chunk_index;
    
    chunk_index = (object->tileset && !object->auto_split)
        ? PositionToChunkIndexMap(map, position)
        : PositionToChunkIndexObject(map, object, position);
        
    if (chunk_index >= 0) {
        UpdateAdjacentChunkIndexes(map, object, chunk_index);

        for (int adjacent_index = 0; adjacent_index < GetAdjacentChunkCount(map); adjacent_index++) {
            DrawChunk(map, object, object->chunkset.indexes[adjacent_index]);

            object->chunkset.indexes[adjacent_index] = chunk_index;
        }
    }
}

/* 거리가 `map.draw_distance` 이하인 청크의 최대 개수를 구한다. */
int GetAdjacentChunkCount(LwMap *map) {
    return (2 * map->draw_distance + 1) * (2 * map->draw_distance + 1);
}

/* 
    고유 번호가 `index`인 청크에서 거리가 `map.draw_distance` 이하인 모든 청크의 고유 번호를 
    구한 다음, `object.chunk_indexes`에 저장한다.
*/
void UpdateAdjacentChunkIndexes(LwMap *map, LwObject *object, int index) {
    int adjacent_index;
    int adjacent_chunk_x, adjacent_chunk_y;
    
    adjacent_index = 0;
    
    if (object->chunkset.temp_index == index)
        return;
        
    object->chunkset.temp_index = index;
    
    for (int dy = (map->draw_distance); dy >= -(map->draw_distance); dy--) {
        for (int dx = (map->draw_distance); dx >= -(map->draw_distance); dx--) {
            if (object->tileset && !object->auto_split) {
                adjacent_chunk_x = GetMapChunkX(map, index) - dx;
                adjacent_chunk_y = GetMapChunkY(map, index) - dy;

                if (adjacent_chunk_x < 0 || adjacent_chunk_x > map->width.c - 1
                   || adjacent_chunk_y < 0 || adjacent_chunk_y > map->height.c - 1)
                    continue;

                object->chunkset.indexes[adjacent_index++] = (adjacent_chunk_y * map->width.c)
                    + adjacent_chunk_x;
            } else {
                adjacent_chunk_x = GetObjectChunkX(object, index) - dx;
                adjacent_chunk_y = GetObjectChunkY(object, index) - dy;

                if (adjacent_chunk_x < 0 || adjacent_chunk_x > object->width.c - 1
                   || adjacent_chunk_y < 0 || adjacent_chunk_y > object->height.c - 1)
                    continue;

                object->chunkset.indexes[adjacent_index++] = (adjacent_chunk_y * object->width.c)
                    + adjacent_chunk_x;
            }
        }
    }
}

/* 게임 맵에서 고유 번호가 `index`인 청크의 청크 기준 X좌표를 반환한다. */
int GetMapChunkX(LwMap *map, int index) {
    return (index % map->width.c);
}

/* 게임 맵에서 고유 번호가 `index`인 청크의 청크 기준 Y좌표를 반환한다. */
int GetMapChunkY(LwMap *map, int index) {
    return (index / map->width.c);
}

/* 게임 맵에서 고유 번호가 `index`인 타일의 타일 기준 X좌표를 반환한다. */
int GetMapTileX(LwMap *map, int index) {
    return (index % map->width.t);
}

/* 게임 맵에서 고유 번호가 `index`인 타일의 타일 기준 Y좌표를 반환한다. */
int GetMapTileY(LwMap *map, int index) {
    return (index / map->width.t);
}

/* 개체 텍스처에서 고유 번호가 `index`인 청크의 청크 기준 X좌표를 반환한다. */
int GetObjectChunkX(LwObject *object, int index) {
    return (index % object->width.c);
}

/* 개체 텍스처에서 고유 번호가 `index`인 청크의 청크 기준 Y좌표를 반환한다. */
int GetObjectChunkY(LwObject *object, int index) {
    return (index / object->width.c);
}

/* 개체 텍스처에서 고유 번호가 `index`인 타일의 타일 기준 X좌표를 반환한다. */
int GetObjectTileX(LwObject *object, int index) {
    return (index % object->width.t);
}

/* 개체 텍스처에서 고유 번호가 `index`인 타일의 타일 기준 Y좌표를 반환한다. */
int GetObjectTileY(LwObject *object, int index) {
    return (index / object->width.t);
}

/* 게임 맵에서 고유 번호가 `index`인 청크의 시작 위치를 구한다. */
Vector2 ChunkIndexToPositionMap(LwMap *map, int index) {
    return (Vector2) { 
        GetMapChunkX(map, index) * (map->chunk_width * map->tile_width),
        GetMapChunkY(map, index) * (map->chunk_height * map->tile_height)
    };
}

/* 개체 텍스처에서 고유 번호가 `index`인 청크의 시작 위치를 구한다. */
Vector2 ChunkIndexToPositionObject(LwMap *map, LwObject *object, int index) {
    return (Vector2) { 
        object->position.x + (GetObjectChunkX(object, index) * (map->chunk_width * map->tile_width)),
        object->position.y + (GetObjectChunkY(object, index) * (map->chunk_height * map->tile_height))
    };
}

/* 게임 맵에서 위치 `position`이 속한 청크의 인덱스를 구한다. */
int PositionToChunkIndexMap(LwMap *map, Vector2 position) {
    int tile_index;
    
    if ((tile_index = PositionToTileIndexMap(map, position)) < 0)
        return -1;
    
    return TileIndexToChunkIndexMap(map, tile_index);
}

/* 개체 텍스처에서 위치 `position`이 속한 청크의 인덱스를 구한다. */
int PositionToChunkIndexObject(LwMap *map, LwObject *object, Vector2 position) {
    int tile_index;
    
    if ((tile_index = PositionToTileIndexObject(map, object, position)) < 0)
        return -1;
    
    return TileIndexToChunkIndexObject(map, object, tile_index);
}

/* 게임 맵에서 위치 `position`이 속한 타일의 인덱스를 구한다. */
int PositionToTileIndexMap(LwMap *map, Vector2 position) {
    int tile_x, tile_y;
    
    tile_x = ((int) position.x / map->tile_width) % map->width.t;
    tile_y = ((int) position.y / map->tile_height) % map->height.t;
    
    if (tile_x <= 0) tile_x = 0;
    if (tile_y <= 0) tile_y = 0;
    
    return (tile_y * map->width.t) + tile_x;
}

/* 개체 텍스처에서 위치 `position`이 속한 타일의 인덱스를 구한다. */
int PositionToTileIndexObject(LwMap *map, LwObject *object, Vector2 position) {
    int tile_x, tile_y;
    
    tile_x = ((int) (position.x - object->position.x) / map->tile_width) % object->width.t;
    tile_y = ((int) (position.y - object->position.y) / map->tile_height) % object->height.t;
    
    if (tile_x <= 0) tile_x = 0;
    if (tile_y <= 0) tile_y = 0;
    
    return (tile_y * object->width.t) + tile_x;
}

/* 게임 맵에서 고유 번호가 `chunk_index`인 청크의 `relative_tile_index + 1`번째 타일의 시작 위치를 구한다. */
Vector2 RelativeTileIndexToPositionMap(
    LwMap *map, 
    int chunk_index, int relative_tile_index
) {
    Vector2 chunk_position = ChunkIndexToPositionMap(map, chunk_index);
    
    return (Vector2) {
        chunk_position.x + ((relative_tile_index % map->chunk_width) * map->tile_width),
        chunk_position.y + ((relative_tile_index / map->chunk_width) * map->tile_height)
    };
}

/* 개체 텍스처에서 고유 번호가 `chunk_index`인 청크의 `relative_tile_index + 1`번째 타일의 시작 위치를 구한다. */
Vector2 RelativeTileIndexToPositionObject(
    LwMap *map, LwObject *object, 
    int chunk_index, int relative_tile_index
) {
    Vector2 chunk_position = ChunkIndexToPositionObject(map, object, chunk_index);
    
    return (Vector2) {
        chunk_position.x + ((relative_tile_index % map->chunk_width) * map->tile_width),
        chunk_position.y + ((relative_tile_index / map->chunk_width) * map->tile_height)
    };
}

/* 고유 번호가 `index`인 타일이 속한 청크의 인덱스를 구한다. */
int TileIndexToChunkIndexMap(LwMap *map, int index) {
    int chunk_x, chunk_y;
    
    chunk_x = GetMapTileX(map, index) / map->chunk_width;
    chunk_y = GetMapTileY(map, index) / map->chunk_height;
    
    return (chunk_y * map->width.c) + chunk_x;
}

/* 고유 번호가 `index`인 타일이 속한 청크의 인덱스를 구한다. */
int TileIndexToChunkIndexObject(LwMap *map, LwObject *object, int index) {
    int chunk_x, chunk_y;
    
    chunk_x = GetObjectTileX(object, index) / map->chunk_width;
    chunk_y = GetObjectTileY(object, index) / map->chunk_height;
    
    return (chunk_y * object->width.c) + chunk_x;
}

/* 고유 번호가 `index`인 타일의 시작 위치를 구한다. */
Vector2 TileIndexToPositionMap(LwMap *map, int index) {
    return (Vector2) {
        GetMapTileX(map, index) * map->tile_width,
        GetMapTileY(map, index) * map->tile_height
    };
}

/* 고유 번호가 `index`인 타일의 시작 위치를 구한다. */
Vector2 TileIndexToPositionObject(LwMap *map, LwObject *object, int index) {
    return (Vector2) {
        object->position.x + (GetObjectTileX(object, index) * map->tile_width),
        object->position.y + (GetObjectTileY(object, index) * map->tile_height)
    };
}