/* 关于C的简单正则式使用 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>

int main() {
    char *input = "1/Yz";
    regex_t regex;
    regmatch_t matches[strlen(input)];

    // 编译正则表达式
    if (regcomp(&regex, "([0-9]+)/([A-Za-z]*)", REG_EXTENDED) != 0) {
        fprintf(stderr, "Failed to compile regex.\n");
        return EXIT_FAILURE;
    }

    // 尝试匹配正则表达式
    if (regexec(&regex, input, strlen(input), matches, 0) == 0) {
        // 输出匹配到的数字和字符
        if (matches[0].rm_so != -1 && matches[0].rm_eo != -1) {
            printf("Number: %.*s\n", (int)(matches[1].rm_eo - matches[1].rm_so), input + matches[1].rm_so);
            printf("Character: %.*s\n", (int)(matches[2].rm_eo - matches[2].rm_so), input + matches[2].rm_so);
        }
    } else {
        printf("No match found.\n");
    }

    // 释放正则表达式资源
    regfree(&regex);

    return EXIT_SUCCESS;
}
