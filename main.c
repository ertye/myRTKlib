#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nav_parser.h"

// --- �������� ---
#define MAX_SATELLITES 32   // һ����Ԫ�п��ܵ����������
#define MAX_OBS_TYPES  10   // ֧�ֵ����۲������� (C1, L1, P2, etc.)
#define LINE_LENGTH    256  // �ļ���һ�е���󳤶�

// --- �ṹ�嶨�� ---

// �洢RINEX�ļ�ͷ��Ϣ�Ľṹ��
typedef struct
{
    double version;
    char file_type; // 'O' for Observation
    char sat_system; // 'G' for GPS, 'M' for Mixed
    char approx_pos[3][20]; // ���ջ�����λ�� X, Y, Z
    int num_obs_types;
    char obs_types[MAX_OBS_TYPES][4]; // �洢�۲����ͣ����� "C1", "L1", "P2"
} rinex_header_t;

// �洢�������ǹ۲����ݵĽṹ��
typedef struct
{
    char prn[4];       // ����PRN��, e.g., "G07"
    double obs[MAX_OBS_TYPES]; // ��Ӧͷ�ļ��еĹ۲�ֵ
} satellite_obs_t;

// �洢һ����Ԫ���й۲����ݵĽṹ��
typedef struct
{
    int year, month, day, hour, minute;
    double second;
    int epoch_flag;
    int num_sats;
    satellite_obs_t sat_obs[MAX_SATELLITES];
} epoch_data_t;


// --- �������� ---
int parse_rinex_header(FILE *fp, rinex_header_t *header);
int parse_epoch(FILE *fp, const rinex_header_t *header, epoch_data_t *epoch);
void print_header_info(const rinex_header_t *header);
void print_epoch_data(const epoch_data_t *epoch);


// --- ������ ---
int main(int argc, char *argv[])
{
    // ��������в���
    if (argc < 2)
    {
        fprintf(stderr, "�÷�: %s <rinex_obs_file.obs>\n", argv[0]);
        return 1;
    }

    const char *filename = argv[1];
    FILE *fp = fopen(filename, "r");
    if (fp == NULL)
    {
        perror("�����޷����ļ�");
        return 1;
    }

    printf("���ڽ����ļ�: %s\n\n", filename);

    // 1. �����ļ�ͷ
    rinex_header_t header = {0};
    if (parse_rinex_header(fp, &header) != 0)
    {
        fprintf(stderr, "���󣺽���RINEX�ļ�ͷʧ�ܡ�\n");
        fclose(fp);
        return 1;
    }
    print_header_info(&header);

    // 2. ѭ������ÿһ����Ԫ
    epoch_data_t current_epoch = {0};
    printf("\n--- ��ʼ��ȡ�۲����� ---\n");
    while (parse_epoch(fp, &header, &current_epoch) == 0)
    {
        print_epoch_data(&current_epoch);
    }
    printf("\n--- �ļ�ĩβ ---\n");

    fclose(fp);
    return 0;
}


/**
 * @brief ����RINEX�۲��ļ����ļ�ͷ
 * @param fp �ļ�ָ��
 * @param header ָ��ͷ��Ϣ�ṹ���ָ��
 * @return 0 ��ʾ�ɹ�, -1 ��ʾʧ��
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
                header->obs_types[i][2] = '\0'; // ȷ���ַ�������
                p += 6;
            }
        }
        else if (strstr(line, "END OF HEADER"))
        {
            return 0; // �ļ�ͷ�����ɹ�
        }
    }
    return -1; // δ�ҵ� "END OF HEADER"
}

/**
 * @brief ����һ����Ԫ�Ĺ۲�����
 * @param fp �ļ�ָ��
 * @param header �ļ�ͷ��Ϣ
 * @param epoch ָ����Ԫ���ݽṹ���ָ��
 * @return 0 ��ʾ�ɹ�, -1 ��ʾ�ļ�������ʧ��
 */
int parse_epoch(FILE *fp, const rinex_header_t *header, epoch_data_t *epoch)
{
    char line[LINE_LENGTH];

    // ��ȡ��Ԫͷ
    if (fgets(line, LINE_LENGTH, fp) == NULL)
    {
        return -1; // �ļ�����
    }

    // ����ʱ�䡢��Ԫ��־����������
    // ��ʽ: yy mm dd hh mm ss.sssss  f  n
    int year, month, day, hour, minute, num_sats;
    double second;
    sscanf(line, "%d %d %d %d %d %lf %d %d",
           &year, &month, &day, &hour, &minute, &second,
           &epoch->epoch_flag, &num_sats);

    // RINEX 2.xx �������λ��
    epoch->year = (year > 80) ? (1900 + year) : (2000 + year);
    epoch->month = month;
    epoch->day = day;
    epoch->hour = hour;
    epoch->minute = minute;
    epoch->second = second;
    epoch->num_sats = num_sats;

    // ��ȡ����PRN�б�
    for (int i = 0; i < num_sats; ++i)
    {
        // ÿ12�����ǻ�һ��
        if (i > 0 && i % 12 == 0)
        {
            fgets(line, LINE_LENGTH, fp);
        }
        // ��������ȡ3���ַ���PRN
        strncpy(epoch->sat_obs[i].prn, line + 32 + (i % 12) * 3, 3);
        epoch->sat_obs[i].prn[3] = '\0';
    }

    // ��ȡÿ�����ǵĹ۲�ֵ
    for (int i = 0; i < num_sats; ++i)
    {
        if (fgets(line, LINE_LENGTH, fp) == NULL)
        {
            return -1; // �ļ���ǰ����
        }
        char *p = line;
        for (int j = 0; j < header->num_obs_types; ++j)
        {
            char obs_str[15];
            strncpy(obs_str, p, 14);
            obs_str[14] = '\0';
            epoch->sat_obs[i].obs[j] = atof(obs_str);
            p += 16;
            // RINEX 2.xx һ�����5���۲�ֵ
            if (j == 4 && header->num_obs_types > 5)
            {
                if (fgets(line, LINE_LENGTH, fp) == NULL) return -1;
                p = line;
            }
        }
    }

    return 0;
}

// --- ������ӡ���� ---

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
        // ��������ֻ��ӡ��һ���۲�ֵ��Ϊʾ��
        printf("Obs1 = %14.3f\n", epoch->sat_obs[i].obs[0]);
    }
}
