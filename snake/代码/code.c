#include <reg52.h>

#define lattice_len 32 //16x16点阵，用32字节存储

typedef unsigned char u8;  //一个字节的数

sbit st = P3 ^ 0;
sbit sh = P3 ^ 1;
sbit ds = P3 ^ 2; //单片机连接595的IO口

sbit up = P1 ^ 0;
sbit down = P1 ^ 1;
sbit left = P1 ^ 2;
sbit right = P1 ^ 3;
sbit start = P1 ^ 4;    //游戏进行的时候不能重新开始
sbit pause = P1 ^ 5; //游戏暂停
sbit speedup = P1 ^ 6;
sbit speeddown = P1 ^ 7;


u8 xdata lattice_matrix[lattice_len];    //16x16点阵存储数组
u8 xdata* data lattice_buf = lattice_matrix;
u8 is_start = 0; //防止一开始往反向走
u8 speed = 5;  //蛇的移动速度
//  0   16
//  1   17
//……


u8 xdata snake_matrix[lattice_len * lattice_len];   //data里面存不下了	前4位x，后四位y坐标
u8 xdata* data snake = snake_matrix;		  //通过data里的psnake来访问snake。
u8 snake_len = 0; // 蛇的长度
u8 direction = 5;  //蛇头的运动方向，5表示初始为静止

u8 code match[8] = { 1,2,4,8,16,32,64,128 };
u8 code segcode[10] = { 0x3f,0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x07, 0x7f, 0x6f };
u8 tcount = 0;
u8 is_food = 1;   //为1表示需要食物
u8 food = 0; //前四位是x，后四位是y
u8 is_live = 1;  //判断是否游戏结束

void delay(unsigned int i) {
    while (i--) { }
}

void init595() {
    st = sh = ds = 0;
}

void write_to_595(u8 dat) {    //给595写入数据
    u8 i = 0;
    for (i = 0; i < 8; i++) {
        sh = 0;
        ds = dat & 0x80;    //每次取最高位
        sh = 1;    //上升沿读ds
        dat <<= 1;  //左移一位s
    }
}

void send_from_595() {         //595输出
    sh = 0;
    st = 0;     //产生一个上升沿
    st = 1;
    ds = 0;
    st = 1;
}


void lattice_ini(u8 value) {       //初始化点阵阵列.一开始全灭
    short int i = 0;
    for (i; i < lattice_len; i++) {
        lattice_buf[i] = value;
    }
}

void lattice_refresh() {      //刷新点阵，从上往下刷新
    static u8 row = 0;    //存储当前的列编号
    write_to_595(lattice_buf[row + 16]);
    write_to_595(lattice_buf[row]);
    if (row < 8) {        //上半屏
        write_to_595(0xff);    //先写的会被移到级联得后面
        write_to_595(~match[row]);
    }
    else {
        write_to_595(~match[row - 8]);
        write_to_595(0xff);
    }
    send_from_595();
    row += 1;
    row %= 16;
}

void ini_snake() {
    snake[0] = 7 * 16 + 8;
    snake[1] = 7 * 16 + 7;
    snake[2] = 7 * 16 + 6;
    snake[3] = 7 * 16 + 5;
    snake_len = 4;
}

void illuminate_spot(u8 x, u8 y, bit onf) {   //点亮或熄灭给定位置的灯 左上角为原点，行为x，列为y
    u8 blocky = y / 8;         //判断左半屏还是右半屏
    u8 posy = y % 8;
    if (onf)
        lattice_buf[x + 16 * blocky] |= match[posy];	   //点亮
    else
        lattice_buf[x + 16 * blocky] &= ~match[posy];	//熄灭
}

void illuminate_snake() {          //显示整个蛇
    u8 i = 0;
    for (i; i < snake_len; i++) {
        illuminate_spot(snake[i] / 16, snake[i] % 16, 1);
    }
}

void judge() {
    //判断按键，不能反向移动
    if (up == 0 && direction != 1) {
        delay(20);
        if (up == 0 && direction != 1) {
            is_start = 1;
            direction = 0;
        }
    }
    if (down == 0 && direction != 0) {
        delay(20);
        if (down == 0 && direction != 0) {
            is_start = 1;
            direction = 1;
        }
    }
    if (left == 0 && direction != 3) {
        delay(20);
        if (left == 0 && direction != 3 && is_start == 1)
            direction = 2;
    }
    if (right == 0 && right != 2) {
        delay(20);
        if (right == 0 && direction != 2) {
            direction = 3;
            is_start = 1;
        }
    }
}


void move() {
    if (direction < 5) {	  //5为表示静止
        u8 tail = snake[snake_len - 1];
        u8 i = 1, j = 1;
        for (i = snake_len - 1; i >= 1; i--) {
            snake[i] = snake[i - 1];
        }
        if (direction == 0) {   //向上运动
            if (snake[0] / 16 == 0)
                is_live = 0;      //游戏结束了
            else snake[0] = snake[0] - 16;
        }
        else if (direction == 1) {   //向下运动
            if (snake[0] / 16 == 15)
                is_live = 0;
            else snake[0] = snake[0] + 16;
        }
        else if (direction == 2) {  //向左运动
            if (snake[0] % 16 == 0)
                is_live = 0;
            else snake[0] = snake[0] - 1;
        }
        else if (direction == 3) {   //向右运动
            if (snake[0] % 16 == 15)
                is_live = 0;
            snake[0] = snake[0] + 1;
        }

        if (snake[0] == food) {     //吃到食物尾巴不移动
            snake[snake_len] = tail;
            snake_len += 1;
            is_food = 1;      //需要生成新的食物
        }
        else
            illuminate_spot(tail / 16, tail % 16, 0);
        //先显示头，头正好走到之前的尾巴的话会被更新为不显示
        illuminate_spot(snake[0] / 16, snake[0] % 16, 1);
        for (j; j < snake_len; j++) {
            if (snake[j] == snake[0]) {    //撞到了自己
                is_live = 0;
                break;
            }
        }
        if (is_live == 0)
            lattice_ini(255);
    }
}


void produce_food() {
    u8 i;
    static u8 cnt = 0;
    cnt++;           //cnt实现伪随机
    if (is_food == 1) {
        for (i = 0; i < snake_len; i++) {
            if (cnt == snake[i])    //食物不能生成在蛇的身上
                break;
        }
        if (i == snake_len) {
            is_food = 0;    //已经生成了食物
            food = cnt;
            illuminate_spot(food / 16, food % 16, 1);
        }
        else cnt += 17;    //质数保证能遍历所有的格子
    }
}

void speed_judge() {
    if (pause == 1 || is_start == 0 || is_live == 0) {
        if (speedup == 0) {
            delay(20);
            if (speedup == 0) {
                if (speed >= 2) {
                    speed -= 1;
                    delay(30000);
                }
            }
        }
        if (speeddown == 0) {
            delay(20);
            if (speeddown == 0) {
                if (speed < 9) {
                    speed += 1;
                    delay(30000);
                }
            }
        }
        P2 = segcode[10 - speed];
    }
}

void recovery() {
    //速度不重置
    is_live = 1;
    direction = 5;
    is_food = 1;
    tcount = 0;
    is_start = 0;
    TH1 = 0x3c;
    TL1 = 0xb0;
    init595();
    lattice_ini(0);
    ini_snake();
    illuminate_snake();
}

void main() {
    TMOD = 0x01;
    TH1 = 0x3c;               //定时50ms
    TL1 = 0xb0;
    EA = 1;
    ET1 = 1;
    TR1 = 1;
    P2 = segcode[10 - speed];
    init595();
    lattice_ini(0);
    ini_snake();            //初始化蛇的位置
    illuminate_snake();     //点亮初始的位置

    while (1) {
        lattice_refresh();
        delay(20);
        speed_judge();
        if (is_live == 1 && pause == 0) {   //死了就一直显示当前画面
            judge();
            if (tcount == 10 * speed) {
                tcount = 0;
                move();
            }
            produce_food();
        }
        else {
            if (start == 0) {     //重新开始
                delay(20);
                if (start == 0) {
                    recovery();
                }
            }
        }
    }
}

void timeout() interrupt 3{    //用定时器0不知道为什么会出现神奇错误
    TH1 = 0x3c;
    TL1 = 0xb0;
    tcount += 1;	     //标记定时结束
}