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

int main(){
    char str[256];
    ObjString* a = NULL;
    ObjString* b = NULL;
    ObjString* c = NULL;

    printf("================== Move semantics ==================\n");
    printf("input a: ");
    fgets(str, 255, stdin);
    str[strcspn(str, "\n")] = '\0'; 
    a = newObjString(str);

    printf("input b: ");
    fgets(str, 255, stdin);
    str[strcspn(str, "\n")] = '\0';
    b = newObjString(str);

    c = appendObjString(a, b);      /* move */
    printf("c: %s\n", c->c_str);
    releaseObj(c);
    
    ObjString* d = NULL;
    ObjString* e = NULL;
    ObjString* f = NULL;

    printf("================== Clone semantics ==================\n");

    printf("input d: ");
    fgets(str, 255, stdin);
    str[strcspn(str, "\n")] = '\0'; 
    d = newObjString(str);

    printf("input e: ");
    fgets(str, 255, stdin);
    str[strcspn(str, "\n")] = '\0'; 
    e = newObjString(str);

    f = appendObjString(clone(ObjString, d), clone(ObjString, e));      /* clone */
    printf("f: %s\n", f->c_str);

    releaseObj(d);
    releaseObj(e);
    releaseObj(f);

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
