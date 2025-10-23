#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nav_parser.h"

// --- 常量定义 ---
#define MAX_SATELLITES 32   // 一个历元中可能的最大卫星数
#define MAX_OBS_TYPES  10   // 支持的最大观测类型数 (C1, L1, P2, etc.)
#define LINE_LENGTH    256  // 文件中一行的最大长度

// --- 结构体定义 ---

// 存储RINEX文件头信息的结构体
typedef struct
{
    double version;
    char file_type; // 'O' for Observation
    char sat_system; // 'G' for GPS, 'M' for Mixed
    char approx_pos[3][20]; // 接收机近似位置 X, Y, Z
    int num_obs_types;
    char obs_types[MAX_OBS_TYPES][4]; // 存储观测类型，例如 "C1", "L1", "P2"
} rinex_header_t;

// 存储单个卫星观测数据的结构体
typedef struct
{
    char prn[4];       // 卫星PRN号, e.g., "G07"
    double obs[MAX_OBS_TYPES]; // 对应头文件中的观测值
} satellite_obs_t;

// 存储一个历元所有观测数据的结构体
typedef struct
{
    int year, month, day, hour, minute;
    double second;
    int epoch_flag;
    int num_sats;
    satellite_obs_t sat_obs[MAX_SATELLITES];
} epoch_data_t;


// --- 函数声明 ---
int parse_rinex_header(FILE *fp, rinex_header_t *header);
int parse_epoch(FILE *fp, const rinex_header_t *header, epoch_data_t *epoch);
void print_header_info(const rinex_header_t *header);
void print_epoch_data(const epoch_data_t *epoch);


// --- 主函数 ---
int main(int argc, char *argv[])
{
    // 检查命令行参数
    if (argc < 2)
    {
        fprintf(stderr, "用法: %s <rinex_obs_file.obs>\n", argv[0]);
        return 1;
    }

    const char *filename = argv[1];
    FILE *fp = fopen(filename, "r");
    if (fp == NULL)
    {
        perror("错误：无法打开文件");
        return 1;
    }

    printf("正在解析文件: %s\n\n", filename);

    // 1. 解析文件头
    rinex_header_t header = {0};
    if (parse_rinex_header(fp, &header) != 0)
    {
        fprintf(stderr, "错误：解析RINEX文件头失败。\n");
        fclose(fp);
        return 1;
    }
    print_header_info(&header);

    // 2. 循环解析每一个历元
    epoch_data_t current_epoch = {0};
    printf("\n--- 开始读取观测数据 ---\n");
    while (parse_epoch(fp, &header, &current_epoch) == 0)
    {
        print_epoch_data(&current_epoch);
    }
    printf("\n--- 文件末尾 ---\n");

    fclose(fp);
    return 0;
}


/**
 * @brief 解析RINEX观测文件的文件头
 * @param fp 文件指针
 * @param header 指向头信息结构体的指针
 * @return 0 表示成功, -1 表示失败
 */
int parse_rinex_header(FILE *fp, rinex_header_t *header)
{
    char line[LINE_LENGTH];

    while (fgets(line, LINE_LENGTH, fp) != NULL)
    {
        if (strstr(line, "RINEX VERSION / TYPE"))
        {
            sscanf(line, "%lf", &header->version);
            header->file_type = line[20];
            header->sat_system = line[40];
        }
        else if (strstr(line, "APPROX POSITION XYZ"))
        {
            sscanf(line, "%s %s %s", header->approx_pos[0], header->approx_pos[1], header->approx_pos[2]);
        }
        else if (strstr(line, "# / TYPES OF OBSERV"))
        {
            sscanf(line, "%d", &header->num_obs_types);
            int count = header->num_obs_types;
            char *p = line + 10;
            for (int i = 0; i < count && i < MAX_OBS_TYPES; ++i)
            {
                strncpy(header->obs_types[i], p, 2);
                header->obs_types[i][2] = '\0'; // 确保字符串结束
                p += 6;
            }
        }
        else if (strstr(line, "END OF HEADER"))
        {
            return 0; // 文件头解析成功
        }
    }
    return -1; // 未找到 "END OF HEADER"
}

/**
 * @brief 解析一个历元的观测数据
 * @param fp 文件指针
 * @param header 文件头信息
 * @param epoch 指向历元数据结构体的指针
 * @return 0 表示成功, -1 表示文件结束或失败
 */
int parse_epoch(FILE *fp, const rinex_header_t *header, epoch_data_t *epoch)
{
    char line[LINE_LENGTH];

    // 读取历元头
    if (fgets(line, LINE_LENGTH, fp) == NULL)
    {
        return -1; // 文件结束
    }

    // 解析时间、历元标志和卫星数量
    // 格式: yy mm dd hh mm ss.sssss  f  n
    int year, month, day, hour, minute, num_sats;
    double second;
    sscanf(line, "%d %d %d %d %d %lf %d %d",
           &year, &month, &day, &hour, &minute, &second,
           &epoch->epoch_flag, &num_sats);

    // RINEX 2.xx 年份是两位数
    epoch->year = (year > 80) ? (1900 + year) : (2000 + year);
    epoch->month = month;
    epoch->day = day;
    epoch->hour = hour;
    epoch->minute = minute;
    epoch->second = second;
    epoch->num_sats = num_sats;

    // 读取卫星PRN列表
    for (int i = 0; i < num_sats; ++i)
    {
        // 每12颗卫星换一行
        if (i > 0 && i % 12 == 0)
        {
            fgets(line, LINE_LENGTH, fp);
        }
        // 从行中提取3个字符的PRN
        strncpy(epoch->sat_obs[i].prn, line + 32 + (i % 12) * 3, 3);
        epoch->sat_obs[i].prn[3] = '\0';
    }

    // 读取每个卫星的观测值
    for (int i = 0; i < num_sats; ++i)
    {
        if (fgets(line, LINE_LENGTH, fp) == NULL)
        {
            return -1; // 文件提前结束
        }
        char *p = line;
        for (int j = 0; j < header->num_obs_types; ++j)
        {
            char obs_str[15];
            strncpy(obs_str, p, 14);
            obs_str[14] = '\0';
            epoch->sat_obs[i].obs[j] = atof(obs_str);
            p += 16;
            // RINEX 2.xx 一行最多5个观测值
            if (j == 4 && header->num_obs_types > 5)
            {
                if (fgets(line, LINE_LENGTH, fp) == NULL) return -1;
                p = line;
            }
        }
    }

    return 0;
}

// --- 辅助打印函数 ---

void print_header_info(const rinex_header_t *header)
{
    printf("--- RINEX Header Info ---\n");
    printf("Version       : %.2f\n", header->version);
    printf("File Type     : %c (%s)\n", header->file_type, "Observation Data");
    printf("Sat System    : %c (%s)\n", header->sat_system, "GPS/Mixed");
    printf("Approx Pos XYZ: %s, %s, %s\n", header->approx_pos[0], header->approx_pos[1], header->approx_pos[2]);
    printf("Obs Types (%d) : ", header->num_obs_types);
    for (int i = 0; i < header->num_obs_types; ++i)
    {
        printf("%s ", header->obs_types[i]);
    }
    printf("\n-------------------------\n");
}

void print_epoch_data(const epoch_data_t *epoch)
{
    printf("\n> Epoch: %04d-%02d-%02d %02d:%02d:%06.3f | Satellites: %d\n",
           epoch->year, epoch->month, epoch->day, epoch->hour, epoch->minute, epoch->second,
           epoch->num_sats);

    for (int i = 0; i < epoch->num_sats; ++i)
    {
        printf("  %s: ", epoch->sat_obs[i].prn);
        // 这里我们只打印第一个观测值作为示例
        printf("Obs1 = %14.3f\n", epoch->sat_obs[i].obs[0]);
    }
}
