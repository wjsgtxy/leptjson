#ifndef LEPTJSON_H
#define LEPTJSON_H

typedef enum {LEPT_NULL, LEPT_FALSE, LEPT_TRUE, LEPT_NUMBER, LEPT_STRING, LEPT_ARRAY, LEPT_OBJECT} lept_type;

#define lept_init(v)        do{ (v) -> type = LEPT_NULL; } while(0)

typedef struct lept_value lept_value; // 因为后面lept_value 结构体中用到了lept_value自己,所以这里要前项声明 （forward declare）
struct lept_value // 注意这里是结构体名称，不是变量，只有名称才能在上面的typedef里面使用
{
    //double n;
    union // 结构体的各个成员会占用不同的内存，互相之间没有影响；而共用体的所有成员占用同一段内存，修改一个成员会影响其余所有成员 因为一个值不可能同时是数字和字符，所以这里使用unioin
    {
        struct {lept_value* e; size_t size; } a; /* array，size 表示元素个数 */ 
        struct {char* s; size_t len;} s;
        double n;
    } u;
    
    lept_type type;
};

enum {
    LEPT_PARSE_OK = 0,
    LEPT_PARSE_EXPECT_VALUE,
    LEPT_PARSE_INVALID_VALUE,
    LEPT_PARSE_ROOT_NOT_SINGULAR,
    LEPT_PARSE_MISS_QUOTATION_MARK,
    LEPT_PARSE_INVALID_STRING_ESCAPE,
    LEPT_PARSE_INVALID_STRING_CHAR,
    LEPT_PARSE_INVALID_UNICODE_SURROGATE,
    LEPT_PARSE_INVALID_UNICODE_HEX,
    LEPT_PARSE_MISS_COMMA_OR_SQUARE_BRACKET
};

int lept_parse(lept_value* v, const char* json);

void lept_free(lept_value* v);

lept_type lept_get_type(const lept_value* v);

int lept_get_boolean(const lept_value* v);
void lept_set_boolean(lept_value* v, int b);

double lept_get_number(const lept_value* v); // 注意，所有在test.c中引用的函数，都要在这里导出 否则编译提示 implicit declaration of function 'lept_get_number'
void lept_set_number(lept_value* v, double n);

const char* lept_get_string(const lept_value* v);
size_t lept_get_string_length(const lept_value* v);
void lept_set_string(lept_value* v, const char* s, size_t len);

size_t lept_get_array_size(const lept_value* v);
lept_value* lept_get_array_element(const lept_value* v, size_t index);


#endif /* LEPTJSON_H */
