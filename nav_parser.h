#ifndef NAV_PARSER_H
#define NAV_PARSER_H

#include <stdio.h>

// --- �������� ---
#define MAX_GPS_SATELLITES 32 // GPS��������

// --- �ṹ�嶨�� ---

/**
 * @brief �洢GPS�㲥���������Ľṹ��
 * RINEX 2.xx NAV file format
 */
typedef struct {
    int prn;         // ����PRN��
    int year, month, day, hour, minute; // toc: time of clock
    double second;
    double af0, af1, af2; // �����Ӳ����

    // �������
    double IODE;     // Issue of Data, Ephemeris
    double Crs, delta_n, M0;
    double Cuc, e, Cus;
    double sqrtA;    // Semi-major axis��ƽ����
    double Toe;      // Time of Ephemeris
    double Cic, OMEGA0, Cis;
    double i0, Crc, omega;
    double OMEGADOT, IDOT;

    int health;      // ���ǽ���״̬
} ephemeris_t;


/**
 * @brief �洢�������ǵ������ݵ�����
 */
typedef struct {
    ephemeris_t eph[MAX_GPS_SATELLITES + 1]; // ����������ӦPRN�� (1-32)
    double ion_alpha[4]; // �������� alpha
    double ion_beta[4];  // �������� beta
} nav_data_t;


// --- �������� ---

/**
 * @brief ����RINEX�����ļ�(.nav)
 * @param filepath �ļ�·��
 * @param nav_data ָ�򵼺�����������ָ��
 * @return 0 ��ʾ�ɹ�, -1 ��ʾʧ��
 */
int parse_nav_file(const char *filepath, nav_data_t *nav_data);

#endif // NAV_PARSER_H
