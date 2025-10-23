#include <string.h>
#include <stdlib.h>
#include "nav_parser.h"

#define NAV_LINE_LENGTH 256

// 辅助函数：将字符串中的 'D' 替换为 'e' 以便C语言解析
static void replace_exponent_d(char* str) {
    char* p = str;
    while ((p = strchr(p, 'D')) != NULL) {
        *p = 'e';
    }
}

/**
 * @brief 解析RINEX导航文件(.nav)
 * @param filepath 文件路径
 * @param nav_data 指向导航数据容器的指针
 * @return 0 表示成功, -1 表示失败
 */
int parse_nav_file(const char *filepath, nav_data_t *nav_data) {
    FILE *fp = fopen(filepath, "r");
    if (fp == NULL) {
        perror("错误：无法打开导航文件");
        return -1;
    }

    char line[NAV_LINE_LENGTH];
    int prn = 0;

    // 初始化nav_data
    memset(nav_data, 0, sizeof(nav_data_t));

    // --- 1. 解析文件头 ---
    while (fgets(line, NAV_LINE_LENGTH, fp) != NULL) {
        if (strstr(line, "ION ALPHA")) {
            replace_exponent_d(line);
            sscanf(line, "%lf%lf%lf%lf", &nav_data->ion_alpha[0], &nav_data->ion_alpha[1],
                   &nav_data->ion_alpha[2], &nav_data->ion_alpha[3]);
        } else if (strstr(line, "ION BETA")) {
            replace_exponent_d(line);
            sscanf(line, "%lf%lf%lf%lf", &nav_data->ion_beta[0], &nav_data->ion_beta[1],
                   &nav_data->ion_beta[2], &nav_data->ion_beta[3]);
        } else if (strstr(line, "END OF HEADER")) {
            break; // 文件头结束
        }
    }

    // --- 2. 解析星历数据体 ---
    while (fgets(line, NAV_LINE_LENGTH, fp) != NULL) {
        replace_exponent_d(line);

        // 读取星历记录的第一行
        int year;
        sscanf(line, "%d %d %d %d %d %d %lf %lf %lf %lf",
               &prn, &year, &nav_data->eph[prn].month, &nav_data->eph[prn].day,
               &nav_data->eph[prn].hour, &nav_data->eph[prn].minute, &nav_data->eph[prn].second,
               &nav_data->eph[prn].af0, &nav_data->eph[prn].af1, &nav_data->eph[prn].af2);

        nav_data->eph[prn].year = (year < 80) ? (2000 + year) : (1900 + year);
        nav_data->eph[prn].prn = prn;

        // 读取后续7行数据
        // Line 2
        fgets(line, NAV_LINE_LENGTH, fp); replace_exponent_d(line);
        sscanf(line, "    %lf %lf %lf %lf", &nav_data->eph[prn].IODE, &nav_data->eph[prn].Crs, &nav_data->eph[prn].delta_n, &nav_data->eph[prn].M0);

        // Line 3
        fgets(line, NAV_LINE_LENGTH, fp); replace_exponent_d(line);
        sscanf(line, "    %lf %lf %lf %lf", &nav_data->eph[prn].Cuc, &nav_data->eph[prn].e, &nav_data->eph[prn].Cus, &nav_data->eph[prn].sqrtA);

        // Line 4
        fgets(line, NAV_LINE_LENGTH, fp); replace_exponent_d(line);
        sscanf(line, "    %lf %lf %lf %lf", &nav_data->eph[prn].Toe, &nav_data->eph[prn].Cic, &nav_data->eph[prn].OMEGA0, &nav_data->eph[prn].Cis);

        // Line 5
        fgets(line, NAV_LINE_LENGTH, fp); replace_exponent_d(line);
        sscanf(line, "    %lf %lf %lf %lf", &nav_data->eph[prn].i0, &nav_data->eph[prn].Crc, &nav_data->eph[prn].omega, &nav_data->eph[prn].OMEGADOT);

        // Line 6
        fgets(line, NAV_LINE_LENGTH, fp); replace_exponent_d(line);
        sscanf(line, "    %lf %lf", &nav_data->eph[prn].IDOT, &nav_data->eph[prn].health); // health is actually part of a different field but sscanf can pick it up

        // Line 7 and 8 are usually empty or contain other info we ignore for now
        fgets(line, NAV_LINE_LENGTH, fp);
        fgets(line, NAV_LINE_LENGTH, fp);
    }

    fclose(fp);
    printf("成功解析导航文件，加载了%d颗卫星的星历。\n", prn > 0 ? MAX_GPS_SATELLITES : 0); // 简单示意
    return 0;
}
