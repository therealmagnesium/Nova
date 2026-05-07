#include <Nova.h>
#include <stdio.h>

int main(int argc, char** argv)
{
    string text_a = "Some text";
    string text_b = "Some text";
    printf("Text A: %s\n", text_a.c_str());
    printf("Text B: %s\n", text_b.c_str());
    printf("A == B? %s\n", text_a == text_b ? "True" : "False");

    INFO("%s", "A task worked correctly");
    WARN("A task failed and returned with code %d", 1);
    ERROR("%s", "A task failed and will probably affect the app");
    FATAL("%s", "A crucial task failed and forced the app to close");

    return 0;
}
