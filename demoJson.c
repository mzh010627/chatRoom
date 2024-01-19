#include <json-c/json.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* json 库测试 */
/* 
    json是外来库，编译时要+ -ljson-c
*/

int main(int argc, char *argv[])
{
    char name[10] = "zhangsan";
    int age = 18;
    char adder[50] = "jiangsuhengnanjingshi";
    json_object *jobj = json_object_new_object();
    json_object_object_add(jobj, "name", json_object_new_string(name));
    json_object_object_add(jobj, "age", json_object_new_int(age));
    json_object_object_add(jobj, "adder", json_object_new_string(adder));
    const char *str = json_object_to_json_string(jobj);
    printf("%s\n", str);

    /* 打印 json中的name */
    json_object *jname = json_object_object_get(jobj, "name");
    const char *namestr = json_object_get_string(jname);
    printf("%s\n", namestr);

    /* 判断 json中是否有phonenum */
    json_object *jphone = json_object_object_get(jobj, "phonenum");
    if (jphone == NULL)
    {
        printf("no phonenum\n");
    }

    /* 修改json中的name */
    json_object_object_add(jobj, "name", json_object_new_string("lisi"));


    /* 删除json中的name */
    json_object_object_del(jobj, "name");
    namestr = json_object_get_string(jname);
    if (namestr == NULL)
    {
        printf("name is NULL\n");
    }

    /* 修改json中的age */
    json_object_object_add(jobj, "age", json_object_new_int(20));
    int age2 = json_object_get_int(jphone);
    printf("%d\n", age2);

    /* 获取json的长度 */
    int len = json_object_object_length(jobj);
    printf("jsonlen = %d\n", len);
    
    /* 遍历json */
    json_object_object_foreach(jobj, key, val)
    {
        printf("key = %s, val = %s\n", key, json_object_get_string(val));
        /* 
        // other
        if (strcmp(key, "name") == 0)
        {
            printf("name = %s\n", json_object_get_string(val));
            strcpy(name, json_object_get_string(val));
            printf("name = %s\n", name);
        }
        if (strcmp(key, "age") == 0)
        {
            printf("age = %d\n", json_object_get_int(val));
            age = json_object_get_int(val);
            printf("age = %d\n", age);
        }
        if (strcmp(key, "adder") == 0)
        {
            printf("adder = %s\n", json_object_get_string(val));
            strcpy(adder, json_object_get_string(val));
            printf("adder = %s\n", adder);
        }
        */ 
    }

    /* 释放json */
    json_object_put(jobj);

    return 0;
}