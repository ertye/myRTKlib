#include ".\src\rtklib.h"

#define TRACEFILE   "rtknavi_%Y%m%d%h%M.trace" // debug trace file
#define MAXPORTOFF  9                   // max port number offset

rtksvr_t rtksvr;                        // rtk server struct
stream_t monistr;                       // monitor stream

int main(void)
{
    int SvrCycle=10, SvrBuffSize=32768, NavSelect=0;
    int NmeaReq=0, NmeaCycle=1000;
    int Stream[MAXSTRRTK]= {4,4,0,5,0,0,0,0},StreamC[MAXSTRRTK]= {1,1,0,1,0,0,0,0},Format[MAXSTRRTK]= {1,1,0,3,0,0,0,0};
    int DebugTraceF=9;
    double NmeaPos[3]= {0};
    int port = 52001, TimeoutTime=10000, ReconTime=10000;
    char path[64];

    solopt_t SolOpt=
    {
        SOLF_LLH,TIMES_GPST,1,3,    /* posf,times,timef,timeu */
        0,0,0,0,0,0,0,              /* degf,outhead,outopt,outvel,datum,height,geoid */
        0,0,0,                      /* solstatic,sstat,trace */
        {0.0,0.0},                  /* nmeaintv */
        "","",                      /* separator/program name */
        0                           /* maxsolstd */
    }, solopt[2];
    double pos[3],nmeapos[3];
    int itype[]=
    {
        STR_SERIAL,STR_TCPCLI,STR_TCPSVR,STR_NTRIPCLI,STR_FILE,STR_FTP,STR_HTTP
    };
    int otype[]=
    {
        STR_SERIAL,STR_TCPCLI,STR_TCPSVR,STR_NTRIPSVR,STR_NTRIPCAS,STR_FILE
    };
    int i,strs[MAXSTRRTK]= {0};
    char* paths[8] = { "data\\Rover_20240520_082407.rtcm3::T::x60",
                       "data\\Base_Station_20240520_082407.rtcm3::T::x60",
                       "","data\\Solution_%Y%m%d-%h%M%S.log","","","",""
                     },
                     * cmds[3] = { NULL,NULL,NULL }, * cmds_periodic[3] = { NULL,NULL,NULL }, * rcvopts[3] = { "","","" };
    char errmsg[20148];
    pcv_t pcv0= {0};
    prcopt_t PrcOpt=
    {
        PMODE_KINEMA,0,1,51,   /* mode,soltype,nf,navsys */
        15.0*D2R,{{0,0}},           /* elmin,snrmask */
        0,1,0,1,                    /* sateph,modear,glomodear,bdsmodear */
        5,0,10,1,                   /* maxout,minlock,minfix,armaxiter */
        1,1,0,0,                    /* ionoopt,tropopt,dynamics,tidecorr */
        1,0,0,0,0,                  /* niter,codesmooth,intpref,sbascorr,sbassatsel */
        0,0,                        /* rovpos,refpos */
        {100.0,100.0},              /* eratio[] */
        {100.0,0.003,0.003,0.0,1.0}, /* err[] */
        {30.0,0.03,0.3},            /* std[] */
        {1E-4,1E-3,1E-4,10.0,10.0,0.0}, /* prn[] */
        5E-12,                      /* sclkstab */
        {3.0,0.9999,0.25,0.1,0.05}, /* thresar */
        0.0,0.0,0.05,               /* elmaskar,elmaskhold,thresslip */
        30.0,30.0,30.0,             /* maxtdiff,maxinno,maxgdop */
        {0},{0},{0},                /* baseline,ru,rb */
        {"",""},                    /* anttype */
        {{0}},{{0}},{0},             /* antdel,pcv,exsats */
        3600,1,0,{"",""},{0,0,0,0,0,0},0 /* maxaveep,initrst,outsingle,rnxopt,posopt,syncsol */
    };

    rtksvrinit(&rtksvr);
    strinit(&monistr);

    for (i=0; i<=MAXPORTOFF; i++)
    {

        sprintf(path,":%d",port+i);

        if (stropen(&monistr,STR_TCPSVR,STR_MODE_RW,path))
        {
            strsettimeout(&monistr,TimeoutTime,ReconTime);
            break;
        }
    }

    trace(3,"SvrStart\n");

    if (DebugTraceF>0)
    {
        traceopen(TRACEFILE);
        tracelevel(DebugTraceF);
    }

    // Rover Position Type, LLH - Latitude Longtitude Height
    PrcOpt.rovpos=POSOPT_POS;
    for (i = 0; i < 3; i++) PrcOpt.ru[i] = 0.0;

    // Reference Position Type, LLH - Latitude Longtitude Height
    PrcOpt.refpos=POSOPT_RTCM;
    for (i=0; i<3; i++) PrcOpt.rb[i]=0.0;

    for (i=0; i<MAXSAT; i++)
    {
        PrcOpt.exsats[i]=0;
    }

    // PCV, antenna phase centre variation 天线相位中心改正
    PrcOpt.pcvr[0]=PrcOpt.pcvr[1]=pcv0; // initialize antenna PCV

    PrcOpt.baseline[0]=0.0;
    PrcOpt.baseline[1]=0.0;

    // input type and output type
    for (i=0; i<3; i++) strs[i]=StreamC[i]?itype[Stream[i]]:STR_NONE;
    for (i=3; i<5; i++) strs[i]=StreamC[i]?otype[Stream[i]]:STR_NONE;
    for (i=5; i<8; i++) strs[i]=StreamC[i]?otype[Stream[i]]:STR_NONE;

    NmeaCycle=NmeaCycle<1000?1000:NmeaCycle;

    // degrees to radian 角度转弧度
    pos[0]=NmeaPos[0]*D2R;
    pos[1]=NmeaPos[1]*D2R;
    pos[2]=NmeaPos[2];
    pos2ecef(pos,nmeapos);

    for (i=0; i<2; i++)
    {
        solopt[i]=SolOpt;
        solopt[i].posf=Format[i+3];
    }

    // start rtk server
    printf("######In main function now\n");
    if (!rtksvrstart(&rtksvr,SvrCycle,SvrBuffSize,strs,paths,Format,NavSelect,
                     cmds,cmds_periodic,rcvopts,NmeaCycle,NmeaReq,nmeapos,
                     &PrcOpt,solopt,&monistr,errmsg))
    {
        trace(2,"rtksvrstart error %s\n",errmsg);
        traceclose();
        return 0;
    }
    printf("######In main function now\n");
    return 1;
}
