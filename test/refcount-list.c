#include <float.h>
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

extern void* alloc(size_t);
extern void release(void*);

struct Obj;
typedef struct {
    const char* typename;
    void (*destructor)(struct Obj*);
} ObjVtable;

typedef struct Obj{
    uint32_t refcount;
    const ObjVtable* vtable;
} Obj;

#define OBJ(obj_ptr) ((Obj*) obj_ptr)
Obj* _cloneObj(Obj*);
#define clone(type, obj_ptr) ((type*) _cloneObj(OBJ(obj_ptr)))
void _releaseObj(Obj*);
#define releaseObj(obj_ptr) _releaseObj(OBJ(obj_ptr))

Obj* _allocObj(size_t, const ObjVtable*);
#define allocObj(type, payload, vtable) ((type*) _allocObj(sizeof(type) + payload, vtable))

typedef struct {
    Obj base;
    uint32_t len;
    char c_str[];
} ObjString;

ObjString* newObjString(const char* str);
ObjString* appendObjString(ObjString*, ObjString*);

typedef struct {
    Obj base;
    uint32_t elem_size;
    uint32_t len;
    int isref;
    void* elems;
} ObjVector;

ObjVector* newObjVector(uint32_t, uint32_t, int);
void* accessObjVector(ObjVector*, uint32_t);
#define getElemVector(type, vec, index) (*(type*) accessObjVector(vec, index))
void destructorObjVector(Obj*);

typedef struct ObjCons {
    Obj base;
    int isref;
    struct ObjCons* next;
    char elem[];
} ObjCons, ObjList;

ObjList* newObjList(uint32_t, int);
ObjList* accessObjList(ObjList*, uint32_t);
ObjList* insertObjList(ObjList*, ObjList*);
void destructorObjList(Obj*);

int main(){
    int len = 4;
    ObjList* list = NULL;
    printf("sizeof(ObjList) = %zu\n", sizeof(ObjList));

    printf("========================== primitive value list ==========================\n\n");

    for(int i = 0; i < len; i++){
        ObjCons* node = newObjList(sizeof(int), 0);
        *(int*) node->elem = i;
        list = insertObjList(list, node);
    }

    for(int i = 0; i < len; i++){
        ObjCons* node = accessObjList(list, i);
        printf("list[%d] = %d\n", i, *(int*)node->elem);
    }

    releaseObj(list);
    list = NULL;

    printf("\n========================== reference value list ==========================\n\n");
    const char* str[] = {"Hello World It's me", "!","Good Bye World", "!"};
    for(int i = 0; i < len; i++){
        ObjCons* node = newObjList(sizeof(ObjString*), 1);
        *(ObjString**) node->elem = newObjString(str[i]);
        list = insertObjList(list, node);
    }

    for(int i = 0; i < len; i++){
        ObjCons* node = accessObjList(list, i);
        ObjString* s = *(ObjString**) node->elem;
        printf("list[%d] = %s\n", i, s->c_str);
    }

    releaseObj(list);

    return 0;
}

Obj* _allocObj(size_t size, const ObjVtable* vtable){
    Obj* obj = alloc(size);
    if(!obj) return obj;
    obj->vtable = vtable;
    obj->refcount = 1;
    return obj;
}

ObjVtable obj_string_vtable = {.typename = "ObjString", .destructor = NULL};
ObjString* newObjString(const char* str){
    size_t len = strlen(str);
    ObjString* obj = allocObj(ObjString, len + 1, &obj_string_vtable);
    if(!obj) return NULL;
    obj->len = (uint32_t) len;
    memcpy(obj->c_str, str, obj->len);
    obj->c_str[obj->len] = '\0';
    printf("Allocated a new string: %p \"%s\"\n", obj, obj->c_str);
    return obj;
}

ObjString* appendObjString(ObjString* a, ObjString* b){
    uint32_t len = a->len + b->len;
    ObjString* obj = allocObj(ObjString, len, &obj_string_vtable);
    if(!obj) return NULL;
    obj->len = len;
    memcpy(obj->c_str, a->c_str, a->len);
    memcpy(obj->c_str + a->len, b->c_str, b->len);
    obj->c_str[len] = 0;
    printf("Allocated a new string: %p \"%s\"\n", obj, obj->c_str);
    releaseObj(a);
    releaseObj(b);
    return obj;
}

void _releaseObj(Obj* obj){
    if(obj){
        obj->refcount--;
        if(obj->refcount == 0){
            if(obj->vtable->destructor)
                obj->vtable->destructor(obj);
            else
                release(obj);
        }
    }
}

Obj* _cloneObj(Obj* obj){
    if(obj)
        obj->refcount++;
    return obj;
}

ObjVtable obj_vector_vtable = {.typename = "ObjVector", .destructor = &destructorObjVector};
ObjVector* newObjVector(uint32_t size, uint32_t len, int isref){
    ObjVector* v = allocObj(ObjVector, 0, &obj_vector_vtable);
    v->elems = alloc(size * len);
    v->elem_size = size;
    v->len = len;
    v->isref = isref;
    return v;
}
void* accessObjVector(ObjVector* v, uint32_t ind){
    if(ind >= v->len) return NULL;
    return ((char*) v->elems) + ind * v->elem_size;
}

void destructorObjVector(Obj* obj){
    ObjVector* v = (ObjVector*) obj;
    if(v->isref){
        for(uint32_t i = 0; i < v->len; i++){
            Obj* obj = *(Obj**) accessObjVector(v, i);
            printf("releasing vec[%d] (%p) (ref=%u)\n", i, obj, obj->refcount);
            releaseObj(obj);
        }
    }
    release(v->elems);
    release(obj);
}

ObjVtable obj_list_vtable = {.typename = "ObjList", .destructor = &destructorObjList};
ObjList* newObjList(uint32_t size, int isref){
    ObjList* obj = allocObj(ObjList, size, &obj_list_vtable);
    obj->next = NULL;
    obj->isref = isref;
    printf("Allocate a new list ptr=%p\n", obj);
    return obj;
}

ObjList* accessObjList(ObjList* list, uint32_t ind){
    ObjCons* node = list;
    for(uint32_t i = 0; i < ind; i++){
        if(node == NULL) break;
        node = node->next;
    }
    return node;
}

ObjList* insertObjList(ObjList* list, ObjList* node){
    ObjList* prev = NULL;
    ObjList* cur = list;
    while(cur) {
        prev = cur;
        cur = cur->next;
    }
    if(prev) {
        prev->next = node;
        return list;
    } else {
        return node;
    }
}

void destructorObjList(Obj* obj){
    if(obj){
        ObjList* lst = (ObjList*) obj;
        ObjList* next = lst->next;
        if(lst->isref){
            releaseObj(OBJ(*(Obj**)lst->elem));
        }
        release(lst);
        releaseObj(next);
    }
}