#include <stdio.h>
#include <windows.h>
#include <stdlib.h>

/*
    作品名称：Heartbeat Love Program
    作品主题：把喜欢写成程序，把心动变成可执行结果
*/

void delay(int ms)
{
    Sleep(ms);
}

void clearScreen()
{
    system("cls");
}

void printSlow(const char *text, int speed)
{
    while (*text)
    {
        printf("%c", *text);
        fflush(stdout);
        delay(speed);
        text++;
    }
}

void setColor(int color)
{
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
}

void printHeartSmall()
{
    printf("\n");
    printf("        **     **\n");
    printf("      ****** ******\n");
    printf("     ***************\n");
    printf("      *************\n");
    printf("        *********\n");
    printf("          *****\n");
    printf("            *\n");
}

void printHeartBig()
{
    printf("\n");
    printf("          ****       ****\n");
    printf("       ********** **********\n");
    printf("     ***********************\n");
    printf("    *************************\n");
    printf("    *************************\n");
    printf("     ***********************\n");
    printf("       *******************\n");
    printf("         ***************\n");
    printf("           ***********\n");
    printf("             *******\n");
    printf("               ***\n");
    printf("                *\n");
}

void heartbeatAnimation()
{
    int i;

    for (i = 0; i < 6; i++)
    {
        clearScreen();
        setColor(12);
        printf("正在检测心动信号...\n");
        printf("LOVE_SIGNAL = HIGH\n");
        printHeartSmall();
        delay(250);

        clearScreen();
        setColor(13);
        printf("正在检测心动信号...\n");
        printf("LOVE_SIGNAL = HIGH HIGH HIGH\n");
        printHeartBig();
        delay(250);
    }
}

void printECG()
{
    setColor(10);
    printf("\n心跳波形采集中：\n\n");

    printSlow("____/\\/\\____/\\/\\/\\____/\\/\\____/\\/\\/\\/\\____\n", 20);
    delay(300);
    printSlow("分析结果：检测到异常心动\n", 40);
    printSlow("异常原因：你出现在了我的世界里\n", 40);
}

void printLoveCode()
{
    setColor(11);

    printf("\n\n========== 可执行代码情书 ==========\n\n");

    printSlow("if (you.appear == true) {\n", 35);
    printSlow("    my_world.light += 100;\n", 35);
    printSlow("    my_heart.speed++;\n", 35);
    printSlow("}\n\n", 35);

    printSlow("while (life.isRunning()) {\n", 35);
    printSlow("    miss_you++;\n", 35);
    printSlow("    love_you = love_you * 1.01;\n", 35);
    printSlow("}\n\n", 35);

    printSlow("if (distance > 0) {\n", 35);
    printSlow("    memory.play(your_smile);\n", 35);
    printSlow("    waiting.keep();\n", 35);
    printSlow("}\n\n", 35);

    printSlow("return \"你是我程序里，唯一不想结束的循环。\";\n", 45);
}

int main()
{
    SetConsoleOutputCP(65001);

    heartbeatAnimation();

    clearScreen();
    setColor(14);
    printf("========================================\n");
    printf("        Heartbeat Love Program\n");
    printf("========================================\n");

    printECG();
    printLoveCode();

    setColor(12);
    printf("\n\n最终输出结果：\n");
    printf("喜欢你，不是临时变量，而是全局常量。\n");
    printf("陪伴你，不是一次函数调用，而是一生的主程序。\n");

    setColor(7);
    printf("\n\n程序运行结束：LOVE COMPILE SUCCESSFUL.\n");

    system("pause");
    return 0;
}