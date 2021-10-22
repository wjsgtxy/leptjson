#include <stdlib.h> /* NULL strtod() */
#include <assert.h> // "assert()"
#include <memory.h>
#include "leptjson.h"

typedef struct 
{
    const char* json;
    char* stack;
    size_t size, top;
} lept_context;

#define EXPECT(c, ch)       do { assert(*c->json == (ch)); c->json++; } while(0)
#define ISDIGIT(ch)         ((ch) >= '0' && (ch) <= '9')
#define ISDIGIT1TO9(ch)     ((ch) >= '1' && (ch) <= '9')

#define lept_set_null(v)    lept_free(v)
/*
    获取结果
*/
lept_type lept_get_type(const lept_value* v){
    assert(v != NULL);
    return v->type;
}



#define EXPECT(c, ch) do { assert(*c->json == (ch)); c->json++; } while(0) 

/*  ws = *(%x20 / %x09 / %x0A / %x0D) */
// 因为不会出现错误，所以返回void
static void lept_parse_whitespace(lept_context* c){
    const char *p = c->json;
    while(*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r'){
        p++;
    }
    c->json = p;
}

#if 0 // 通过这种方式实现注释的嵌套
/* null = "null" */
static int lept_parse_null(lept_context* c, lept_value* v){
    EXPECT(c, 'n'); // 首先期待一个 n 
    if(c->json[0] != 'u' || c->json[1] != 'l' || c->json[2] != 'l'){
        return LEPT_PARSE_INVALID_VALUE;
    }
    c->json += 3; // n已经加了， 这里加上 ull
    v->type = LEPT_NULL;
    return LEPT_PARSE_OK;
}

static int lept_parse_true(lept_context* c, lept_value* v){
    EXPECT(c, 't');
    if(c->json[0] != 'r' || c->json[1] != 'u' || c->json[2] != 'e'){
        return LEPT_PARSE_INVALID_VALUE;
    }
    c->json += 3;
    v->type = LEPT_TRUE;
    return LEPT_PARSE_OK;
}

static int lept_parse_false(lept_context* c, lept_value* v){
    EXPECT(c, 'f');
    if(c->json[0] != 'a' || c->json[1] != 'l' || c->json[2] != 's' || c->json[3] != 'e'){
        return LEPT_PARSE_INVALID_VALUE;
    }
    c->json += 4;
    v->type = LEPT_FALSE;
    return LEPT_PARSE_OK;
}
#endif

static int lept_parse_literal(lept_context* c, lept_value* v, const char* literal, lept_type type){
    size_t i; // 注意表示长度的，用size_t, 不要用int
    EXPECT(c, literal[0]);
    for(i=0; literal[i+1]; i++){
        if(c->json[i] != literal[i+1]){
            return LEPT_PARSE_INVALID_VALUE;
        }
    }
    c->json += i;
    v->type = type;
    return LEPT_PARSE_OK;
}

static int lept_parse_number(lept_context* c, lept_value* v){
    char* end;
    const char* p = c->json;
    /* 校验数字 */
    /* 负号 */
    if(*p == '-') p++;
    /* 整数 */
    if(*p == '0'){ // 只有一个0
        p++;
    } else {
        if(!ISDIGIT1TO9(*p)) 
            return LEPT_PARSE_INVALID_VALUE;
        for(; ISDIGIT(*p); p++); // 有多少个数字就跳过多少个
    }
    /* 小数 */
    if(*p == '.'){
        p++;
        if(!ISDIGIT(*p)) return LEPT_PARSE_INVALID_VALUE; // 小数点后至少要有一个数字
        for(; ISDIGIT(*p); p++);
    }
    /* 指数 */
    if(*p == 'e' || *p == 'E'){
        p++;
        if(*p == '-' || *p == '+') p++;
        if(!ISDIGIT(*p)) return LEPT_PARSE_INVALID_VALUE;
        for(; ISDIGIT(*p); p++);
    }
    /* 校验完成 */

    v->u.n = strtod(c->json, &end); // 第二个参数 char** 如果遇到不符合条件而终止的字符，由end返回
    /* 例子： 
    char b[] = "1234.567qwer"; 
    printf( "b=%lf\n", strtod(b,&endptr) );
    printf( "endptr=%s\n", endptr );  //b=1234.567000 endptr=qwer
    */
    if (c->json == end) { // ??
        return LEPT_PARSE_INVALID_VALUE;
    }
    c->json = end; // ??
    v->type = LEPT_NUMBER;
    return LEPT_PARSE_OK;

}

void lept_free(lept_value* v){
    assert(v != NULL);
    switch (v->type) {
        case LEPT_STRING:
            free(v->u.s.s); // malloc 分配的内存使用free释放
            break;
        case LEPT_ARRAY: // todo 屏蔽之后 用内存泄露工具查看一下
            /* 先把array里面的元素释放，最后释放自己 */
            for(int i=0; i < v->u.a.size; i++){
                free(&v->u.a.e[i]);
            }
            free(v->u.a.e); // 这个数组也是memcpy分配的，也要释放自己
            break;
        default:
            break;
    }
    
    v->type = LEPT_NULL; // TODO 避免重复释放
}

/* 复制一份字符串 */
void lept_set_string(lept_value* v, const char* s, size_t len){
    assert(v != NULL && (s != NULL || len == 0)); // 注意这里，len=0是可以的
    lept_free(v); // 注意，先清空v中可能分配到的内存，比如原来有一串字符了
    v->u.s.s = (char*)malloc(len+1); // +1 因为多了一个结尾的0
    memcpy(v->u.s.s, s, len); // 将字符复制到 v 中
    v->u.s.s[len] = '\0'; // 字符串结尾添加一个0
    v->u.s.len = len;
    v->type = LEPT_STRING;
}

const char* lept_get_string(const lept_value* v){
    assert(v != NULL && v->type == LEPT_STRING);
    return v->u.s.s;
}

size_t lept_get_string_length(const lept_value* v){
    assert(v != NULL && v->type == LEPT_STRING);
    return v->u.s.len;
}

#ifndef LEPT_PARSE_STACK_INIT_SIZE 
#define LEPT_PARSE_STACK_INIT_SIZE 256
#endif /* 好处是，使用者可在编译选项中自行设置宏，没设置的话就用缺省值。 */

static void* lept_context_push(lept_context* c, size_t size){
    void* ret;
    assert(size > 0);
    if (c->top + size >= c->size){
        if(c->size == 0){
            c->size = LEPT_PARSE_STACK_INIT_SIZE;
        }

        while (c->top + size >= c->size) {
            c->size += c->size >> 1; // c->size * 1.5
        }
        c->stack = (char*)realloc(c->stack, c->size); /* c->stack 在初始化时为 NULL，realloc(NULL, size) 的行为是等价于 malloc(size) 的 */
    }
    ret = c->stack + c->top; // 返回起始的指针
    c->top += size; // 变更新的top位置
    return ret;
}

static void* lept_context_pop(lept_context* c, size_t size){
    assert(c->top >= size);
    return c->stack + (c->top -= size); 
}

#define PUTC(c, ch)     do{ *(char*)lept_context_push(c, sizeof(char)) = (ch);} while(0)
#define STRING_ERROR(ret) do{ c->top = head; return ret;} while(0)

/* 解析4位16进制字符 */
static const char* lept_parse_hex4(const char* p, unsigned* u){
    *u = 0;
    for(int i=0; i<4; i++){
        char ch = *p++;
        *u <<= 4; // 每次向左移动4位
        if(ch >= '0' && ch <= '9'){
            *u |= ch - '0';
        }else if(ch >= 'A' && ch <= 'F'){
            *u |= ch - ('A' - 10);
        }else if(ch >= 'a' && ch <= 'f'){
            *u |= ch - ('a' - 10);
        }else{
            return NULL;
        }
        return p;
    }
}

/* 编码成Unicode编码 */
static void lept_encode_utf8(lept_context* c, unsigned u){
    if(u <= 0x7F){
        PUTC(c, u & 0xFF);
    } else if(u <= 0x7FF){
        PUTC(c, 0xC0 | ((u>>6) & 0xFF));
        PUTC(c, 0x80 | ( u     & 0x3F));
    } else if(u <= 0xFFFF){
        PUTC(c, 0xE0 | ((u >> 12) & 0xFF));
        PUTC(c, 0x80 | ((u >>  6) & 0x3F));
        PUTC(c, 0x80 | ( u        & 0x3F));
    } else {
        assert(u <= 0x10FFFF);
        PUTC(c, 0xF0 | ((u >> 18) & 0xFF));
        PUTC(c, 0x80 | ((u >> 12) & 0x3F));
        PUTC(c, 0x80 | ((u >>  6) & 0x3F));
        PUTC(c, 0x80 | ( u        & 0x3F));
    }
}



static int lept_parse_string(lept_context* c, lept_value* v){
    unsigned u, u2; // Unicode存储的码点 单独使用代表 unsigned int
    size_t head = c->top, len; // 记录最开始top的位置，方便后面计算len
    EXPECT(c, '\"'); // 字符串应该要以 " 开头，这里c要用转义 expect里面会把c->json++;
    const char* p = c->json; // 要解析的json字符串  dz: 之前因为这行代码放在了上面一行的上面，导致字符串的测试无法通过！！！因为那里的p还是指向json没有改变之前的一个位置！
    for(;;){
        char ch = *p++; // * 和++ 优先级同，从右向左结合 等价于*(p++), p++先使用p
        switch (ch) {
        case '\"': // 结尾的 "
            len = c->top - head;
            lept_set_string(v, (const char*)lept_context_pop(c, len), len);
            c->json = p;
            return LEPT_PARSE_OK;
            break;
        case '\0':
            /* c->top = head; // 指针复原
            return LEPT_PARSE_MISS_QUOTATION_MARK;  */
            STRING_ERROR(LEPT_PARSE_MISS_QUOTATION_MARK);
            break;
        case '\\': // 转义序列
            switch(*p++) {
                case '\"': PUTC(c, '\"'); break;
                case '\\': PUTC(c, '\\'); break;
                case '/':  PUTC(c, '/' ); break;
                case 'b':  PUTC(c, '\b'); break; // 退格
                case 'f':  PUTC(c, '\f'); break; // 换页
                case 'n':  PUTC(c, '\n'); break; // 换行
                case 'r':  PUTC(c, '\r'); break; // 回车
                case 't':  PUTC(c, '\t'); break; // 制表符
                case 'u': // 处理Unicode字符 \uXXXX这种 或者 \uXXXX \uXXXX
                    if(!(p = lept_parse_hex4(p, &u))){
                        STRING_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX);
                    }   
                    /* surrogate handling */
                    if(u >= 0xD800 && u <= 0xDBFF){ // 代理对的处理，u在这个范围内，表示和后面的一起，表示一个字符，为非基本多文种平面
                        /*码点计算公式为 codepoint = 0x10000 + (H − 0xD800) × 0x400 + (L − 0xDC00) */ 
                        if(*p++ != '\\'){
                            STRING_ERROR(LEPT_PARSE_INVALID_UNICODE_SURROGATE);
                        }
                        if(*p++ != 'u'){
                            STRING_ERROR(LEPT_PARSE_INVALID_UNICODE_SURROGATE);
                        }
                        if(!(p = lept_parse_hex4(p, &u2))){
                            STRING_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX);
                        }
                        if(u2 < 0xDC00 || u2 > 0xDFFF){
                            STRING_ERROR(LEPT_PARSE_INVALID_UNICODE_SURROGATE);
                        }
                        u = 0x10000 + (((u - 0xD800) << 10) | (u2 - 0xDC00)); // 左移10位表示 *0x400 为什么中间是 | 不是+ ？？
                    }
                    lept_encode_utf8(c, u);
                    break;
                 default:
                    STRING_ERROR(LEPT_PARSE_INVALID_STRING_ESCAPE);  // 无效转义字符
            }
            break;
        default:
            if((unsigned char)ch < 0x20){ // todo ??
                // 不合法的字符
                STRING_ERROR(LEPT_PARSE_INVALID_STRING_CHAR);
            }
            PUTC(c, ch);
        }
    }
}

int lept_get_boolean(const lept_value* v){
    assert(v != NULL && (v->type == LEPT_TRUE || v->type == LEPT_FALSE));
    return v->type == LEPT_TRUE;
}

void lept_set_boolean(lept_value* v, int b){
    //assert(v != NULL && (v->type == LEPT_TRUE || v->type == LEPT_FALSE)); // dz 注意，set的时候，不用判断类型
    lept_free(v);
    v->type = b ? LEPT_TRUE : LEPT_FALSE;
}

// 获取值
double lept_get_number(const lept_value* v){
    assert(v != NULL && v->type == LEPT_NUMBER);
    return v->u.n;
}

void lept_set_number(lept_value* v, double n) {
    lept_free(v);
    v->u.n = n;
    v->type = LEPT_NUMBER;
}

/* 2021年10月22日11:17:33 */
size_t lept_get_array_size(const lept_value* v){
    assert(v != NULL && v->type == LEPT_ARRAY);
    return v->u.a.size;
}

lept_value* lept_get_array_element(const lept_value* v, size_t index){
    assert(v != NULL && v->type == LEPT_ARRAY);
    assert(index < v->u.a.size);
    //return &v->u.a.e[index]; // 注意，这里有个 & todo 
    return &(v->u.a.e[index]);
}

static int lept_parse_value(lept_context* c, lept_value* v); // 前项声明, 因为lept_pass_array用到了这个，这个又用到了array，循环引用

static int lept_parse_array(lept_context* c, lept_value* v){
    size_t size = 0;
    int ret;
    EXPECT(c, '[');
    lept_parse_whitespace(c); // dz 解析[ 后面的空白字符： " [ null , false , true , 123 , \"abc\" ] "

    if(*c->json == ']'){
        // 空数组
        c->json++;
        v->type = LEPT_ARRAY;
        v->u.a.size = 0;
        v->u.a.e = NULL;
        return LEPT_PARSE_OK;
    }
    for(;;) {
        lept_value e; /* 要保存的元素，先保存到e里面，然后压入到内存中 */
        lept_init(&e);
        if((ret = lept_parse_value(c, &e)) != LEPT_PARSE_OK){
            // dz解析发生了错误，要弹出堆栈
            // lept_context_pop(c, size*sizeof(lept_value));
            // return ret;
            break; 
        }
            
        memcpy(lept_context_push(c, sizeof(lept_value)), &e, sizeof(lept_value)); // 前面是dst 也就是压栈的起始位置, 后面是src, 复制内存
        size++;

        lept_parse_whitespace(c); // dz去掉逗号前面的字符，后面就应该是','或者']', 否则错误： " [ null , false , true , 123 , \"abc\" ] "
        if(*c->json == ','){
            c->json++;
            lept_parse_whitespace(c); // 解析逗号后面的字符，因为下一次循环的时候，lept_parse_value里面并不去掉空白字符
        }else if(*c->json == ']'){
            c->json++;
            v->type = LEPT_ARRAY;
            v->u.a.size = size;
            size *= sizeof(lept_value); // size 一开始表示元素个数，现在表示分配的字节数
            memcpy(v->u.a.e = (lept_value*)malloc(size), lept_context_pop(c, size), size); // 将所有入栈的元素弹出，放到 array的e指针里面
            return LEPT_PARSE_OK;
        }else{
            // dz发生了错误，此时要弹出堆栈里面的内容才行！
            // lept_context_pop(c, size*sizeof(lept_value));
            ret = LEPT_PARSE_MISS_COMMA_OR_SQUARE_BRACKET;
            break; // 遇到错误，跳出循环
        }
    }
    for (int i=0; i < size; i++){
        lept_free((lept_value*)lept_context_pop(c, sizeof(lept_value))); // 我之前只写了弹出堆栈，但是没有释放分配的内存！
    }
    return ret;
}

static int lept_parse_value(lept_context* c, lept_value* v){
    switch (*c->json) {
        case 'n': 
            //return lept_parse_null(c, v);
            return lept_parse_literal(c, v, "null", LEPT_NULL);
            break;
        case 't':
            //return lept_parse_true(c, v);
            return lept_parse_literal(c, v, "true", LEPT_TRUE);
            break;
        case 'f':
            //return lept_parse_false(c, v);
            return lept_parse_literal(c, v, "false", LEPT_FALSE);
            break;
        default: // 注意，defalut不写在最后也行，不影响功能，都是其他不满足时候执行
            return lept_parse_number(c, v);
        case '"':
            return lept_parse_string(c, v);
        case '[':
            return lept_parse_array(c, v);
        case '\0':
            return LEPT_PARSE_EXPECT_VALUE;
    }
}



int lept_parse(lept_value* v, const char* json){ // todo static ??
    lept_context c;
    int ret;
    assert(v != NULL);
    c.json = json;
    c.stack = NULL;
    c.size = c.top = 0;
    lept_init(v);
    v->type = LEPT_NULL;
    lept_parse_whitespace(&c); // json最左边的空白字符已经去掉了

    if((ret = lept_parse_value(&c, v)) == LEPT_PARSE_OK){
        lept_parse_whitespace(&c); // 这里的空白字符只有4中，不包含 \0
        if(*c.json != '\0'){ // 右端的空白字符解析完成之后，结尾还有字符，则错误
            v->type = LEPT_NULL;
            ret = LEPT_PARSE_ROOT_NOT_SINGULAR;
        }
    }

    assert(c.top == 0); // 确保所有数据被弹出
    free(c.stack);
    return ret;
}
