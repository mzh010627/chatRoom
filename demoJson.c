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
    json_object *jobj;
    jobj = json_object_new_object();
    json_object_object_add(jobj, "name", json_object_new_string(name));
    json_object_object_add(jobj, "age", json_object_new_int(age));
    json_object_object_add(jobj, "adder", json_object_new_string(adder));
    const char *str = json_object_to_json_string(jobj);

    printf("%s\n", str);
    char *str2 = "{ \"name\": \"lisi\", \"ages\": 25, \"adder\": \"jiangsuhengnanjingshi\" }";

    printf("%s\n", str2);
    // json_object_put(jobj);
    json_object * jobj2 = json_tokener_parse(str2);

    // // /* 打印 json中的name */
    // // json_object *jname = json_object_object_get(jobj, "name");
    // // const char *namestr = json_object_get_string(jname);
    // // printf("%s\n", namestr);

    // // /* 判断 json中是否有phonenum */
    // // json_object *jphone = json_object_object_get(jobj, "phonenum");
    // // if (jphone == NULL)
    // // {
    // //     printf("no phonenum\n");
    // // }

    // // /* 修改json中的name */
    // // json_object_object_add(jobj, "name", json_object_new_string("lisi"));


    // // /* 删除json中的name */
    // // json_object_object_del(jobj, "name");
    // // namestr = json_object_get_string(jname);
    // // if (namestr == NULL)
    // // {
    // //     printf("name is NULL\n");
    // // }

    // // /* 修改json中的age */
    // // json_object_object_add(jobj, "age", json_object_new_int(20));
    // // int age2 = json_object_get_int(jphone);
    // // printf("%d\n", age2);

    // // /* 获取json的长度 */
    // // int len = json_object_object_length(jobj);
    // // printf("jsonlen = %d\n", len);
    
    // /* 遍历json */
    // json_object_object_foreach(jobj, key, val)
    // {
    //     printf("key = %s, val = %s\n", key, json_object_get_string(val));
    //     /* 
    //     // other
    //     if (strcmp(key, "name") == 0)
    //     {
    //         printf("name = %s\n", json_object_get_string(val));
    //         strcpy(name, json_object_get_string(val));
    //         printf("name = %s\n", name);
    //     }
    //     if (strcmp(key, "age") == 0)
    //     {
    //         printf("age = %d\n", json_object_get_int(val));
    //         age = json_object_get_int(val);
    //         printf("age = %d\n", age);
    //     }
    //     if (strcmp(key, "adder") == 0)
    //     {
    //         printf("adder = %s\n", json_object_get_string(val));
    //         strcpy(adder, json_object_get_string(val));
    //         printf("adder = %s\n", adder);
    //     }
    //     */ 
    // }

    /* 创建数组类型的json */
    json_object * jarr = json_object_new_array();
    /* 嵌套json */
    json_object_array_add(jarr, jobj);
    json_object_array_add(jarr, jobj2);


    /* 修改jarr中jobj的name的值 */
    json_object * jitem = json_object_array_get_idx(jarr, 0);
    json_object * jname = json_object_object_get(jitem, "name");
    json_object_object_add(jitem, "name", json_object_new_string("王五"));
    // const char * str1 = json_object_to_json_string(jitem);
    // printf("arr:%s\n", str1);

    /* 获取jarr中name="lisi"的json的位置 */
    
    /* 遍历jarr */
    int len = json_object_array_length(jarr);
    for (int i = 0; i < len; i++)
    {
        jitem = json_object_array_get_idx(jarr, i);
        const char * str = json_object_to_json_string(jitem);
        printf("arr:%s\n", str);
        // json_object_put(jitem);
    }

    /* 释放json */
    // json_object_put(jobj);
    // json_object_put(jobj2);
    json_object_put(jarr);      // jobj&jobj2变成了jarr的子节点不需要释放，释放jarr即可

    jarr = json_object_new_array();

    for (int i = 0; i < 10; i++)
    {
        json_object_array_add(jarr, json_object_new_string("123"));
    }


    len = json_object_array_length(jarr);
    for (int i = 0; i < len; i++)
    {
        json_object * jitem = json_object_array_get_idx(jarr, i);
#if 1
        const char * str = json_object_to_json_string(jitem);
        printf("arr:%s\n", str);
#else
        int val = json_object_get_int(jitem);
        printf("arr:%d\n", val);
#endif
    }
    json_object_put(jarr);

    return 0;
}