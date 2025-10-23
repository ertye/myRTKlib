#include <string.h>
#include <stdlib.h>
#include "nav_parser.h"

#define NAV_LINE_LENGTH 256

// �������������ַ����е� 'D' �滻Ϊ 'e' �Ա�C���Խ���
static void replace_exponent_d(char* str) {
    char* p = str;
    while ((p = strchr(p, 'D')) != NULL) {
        *p = 'e';
    }
}

/**
 * @brief ����RINEX�����ļ�(.nav)
 * @param filepath �ļ�·��
 * @param nav_data ָ�򵼺�����������ָ��
 * @return 0 ��ʾ�ɹ�, -1 ��ʾʧ��
 */
int parse_nav_file(const char *filepath, nav_data_t *nav_data) {
    FILE *fp = fopen(filepath, "r");
    if (fp == NULL) {
        perror("�����޷��򿪵����ļ�");
        return -1;
    }

    char line[NAV_LINE_LENGTH];
    int prn = 0;

    // ��ʼ��nav_data
    memset(nav_data, 0, sizeof(nav_data_t));

    // --- 1. �����ļ�ͷ ---
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
            break; // �ļ�ͷ����
        }
    }

    // --- 2. �������������� ---
    while (fgets(line, NAV_LINE_LENGTH, fp) != NULL) {
        replace_exponent_d(line);

        // ��ȡ������¼�ĵ�һ��
        int year;
        sscanf(line, "%d %d %d %d %d %d %lf %lf %lf %lf",
               &prn, &year, &nav_data->eph[prn].month, &nav_data->eph[prn].day,
               &nav_data->eph[prn].hour, &nav_data->eph[prn].minute, &nav_data->eph[prn].second,
               &nav_data->eph[prn].af0, &nav_data->eph[prn].af1, &nav_data->eph[prn].af2);

        nav_data->eph[prn].year = (year < 80) ? (2000 + year) : (1900 + year);
        nav_data->eph[prn].prn = prn;

        // ��ȡ����7������
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
    printf("�ɹ����������ļ���������%d�����ǵ�������\n", prn > 0 ? MAX_GPS_SATELLITES : 0); // ��ʾ��
    return 0;
}
