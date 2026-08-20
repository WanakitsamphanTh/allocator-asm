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


int main(){

    printf("========================== primitive value vector ==========================\n\n");

    int len;
    printf("Input size: ");
    scanf("%d", &len);
    ObjVector* vector = newObjVector(sizeof(int), len, 0);
    
    for(uint32_t i = 0; i < vector->len; i++){
        int* ptr = (int*) accessObjVector(vector, i);
        if(ptr) *ptr = i;
    }

    putchar('[');
    for(uint32_t i = 0; i < vector->len; i++){
        int* ptr = (int*) accessObjVector(vector, i);
        if(ptr) printf("%d", *ptr);
        if(i < vector->len - 1) putchar(',');
    }
    putchar(']');
    putchar('\n');

    releaseObj(vector);

    printf("\n========================== reference value vector ==========================\n\n");
    
    vector = newObjVector(sizeof(ObjString*), 5, 1);
    const char* src[5] = {"Hello", "World", "It's", "Me", "Nice to meet you"};

    for(uint32_t i = 0; i < 5; i++){
        ObjString** str = (ObjString**) accessObjVector(vector, i);
        if(str){
            *str = newObjString(src[i]);
            printf("ObjString{ptr=%p,ref=%u,str=%s}", *str, (*str)->base.refcount, (*str)->c_str);
        }
    }

    putchar('[');
    for(uint32_t i = 0; i < vector->len; i++){
        ObjString** str = (ObjString**) accessObjVector(vector, i);
        if(str) 
            printf("%s", (*str)->c_str);
        if(i < vector->len - 1) 
            putchar(',');
    }
    putchar(']');
    putchar('\n');

    releaseObj(vector);
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