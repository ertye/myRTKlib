#ifndef NAV_PARSER_H
#define NAV_PARSER_H

#include <stdio.h>

// --- 常量定义 ---
#define MAX_GPS_SATELLITES 32 // GPS卫星总数

// --- 结构体定义 ---

/**
 * @brief 存储GPS广播星历参数的结构体
 * RINEX 2.xx NAV file format
 */
typedef struct {
    int prn;         // 卫星PRN号
    int year, month, day, hour, minute; // toc: time of clock
    double second;
    double af0, af1, af2; // 卫星钟差参数

    // 轨道参数
    double IODE;     // Issue of Data, Ephemeris
    double Crs, delta_n, M0;
    double Cuc, e, Cus;
    double sqrtA;    // Semi-major axis的平方根
    double Toe;      // Time of Ephemeris
    double Cic, OMEGA0, Cis;
    double i0, Crc, omega;
    double OMEGADOT, IDOT;

    int health;      // 卫星健康状态
} ephemeris_t;


/**
 * @brief 存储所有卫星导航数据的容器
 */
typedef struct {
    ephemeris_t eph[MAX_GPS_SATELLITES + 1]; // 数组索引对应PRN号 (1-32)
    double ion_alpha[4]; // 电离层参数 alpha
    double ion_beta[4];  // 电离层参数 beta
} nav_data_t;


// --- 函数声明 ---

/**
 * @brief 解析RINEX导航文件(.nav)
 * @param filepath 文件路径
 * @param nav_data 指向导航数据容器的指针
 * @return 0 表示成功, -1 表示失败
 */
int parse_nav_file(const char *filepath, nav_data_t *nav_data);

#endif // NAV_PARSER_H
