#pragma once
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <ctype.h>
#include <memory.h>
#include <string.h>
#include <time.h>
#ifdef SB_MAC_BUILD
#include <unistd.h>
#endif

// C++ includes
#include <string>
#include <vector>
#include <sstream>
#include <iostream>

#include "../../licensedinterfaces/sberrorx.h"
#include "../../licensedinterfaces/theskyxfacadefordriversinterface.h"
#include "../../licensedinterfaces/sleeperinterface.h"
#include "../../licensedinterfaces/serxinterface.h"
#include "../../licensedinterfaces/loggerinterface.h"
#include "../../licensedinterfaces/mountdriverinterface.h"

#include "StopWatch.h"


#define PLUGIN_DEBUG 2   // define this to have log files, 1 = bad stuff only, 2 and up.. full debug

enum SiTechErrors {PLUGIN_OK=0, NOT_CONNECTED, PLUGIN_CANT_CONNECT, PLUGIN_BAD_CMD_RESPONSE, COMMAND_FAILED, PLUGIN_ERROR};

#define SERIAL_BUFFER_SIZE 256
#define MAX_TIMEOUT 1000
#define PLUGIN_LOG_BUFFER_SIZE 256
#define ERR_PARSE   1

/// ATCL response code
#define ATCL_ENTER  0xB1
#define ATCL_ACK    0x8F
#define ATCL_NACK   0xA5

#define ATCL_STATUS         0x9A
#define ATCL_WARNING        0x9B
#define ATCL_ALERT          0x9C
#define ATCL_INTERNAL_ERROR 0x9D
#define ATCL_SYNTAX_ERROR   0x9E
#define ATCL_IDC_ASYNCH     0x9F

#define PLUGIN_NB_SLEW_SPEEDS 5
#define PLUGIN_SLEW_NAME_LENGHT 12
#define PLUGIN_NB_ALIGNEMENT_TYPE 4
#define PLUGIN_ALIGNEMENT_NAME_LENGHT 12


// Define Class for SiTech controller.
class SiTech
{
public:
	SiTech();
	~SiTech();

	int Connect(char *pszPort);
	int Disconnect();
	bool isConnected() const { return m_bIsConnected; }

    void setSerxPointer(SerXInterface *p) { m_pSerx = p; }
    void setLogger(LoggerInterface *pLogger) { m_pLogger = pLogger; };
    void setTSX(TheSkyXFacadeForDriversInterface *pTSX) { m_pTsx = pTSX;};
    void setSleeper(SleeperInterface *pSleeper) { m_pSleeper = pSleeper;};

    int getNbSlewRates();
    int getRateName(int nZeroBasedIndex, char *pszOut, unsigned int nOutMaxSize);


    int getFirmwareVersion(char *version, unsigned int strMaxLen);

    int getRaAndDec(double &dRa, double &dDec);
    int syncTo(double dRa, double dDec);

    int setTrackingRates(bool bTrackingOn, bool bIgnoreRates, double dTrackRaArcSecPerHr, double dTrackDecArcSecPerHr);
    int getTrackRates(bool &bTrackingOn, double &dTrackRaArcSecPerHr, double &dTrackDecArcSecPerHr);

    int startSlewTo(double dRa, double dDec);
    int isSlewToComplete(bool &bComplete);

    int startOpenSlew(const MountDriverInterface::MoveDir Dir, unsigned int nRate);
    int stopOpenLoopMove();

    int gotoPark(double dRa, double dDEc);
    int markParkPosition();
    int getAtPark(bool &bParked);
    int unPark();

    int getRefractionCorrEnabled(bool &bEnabled);
    int setRefractionCorrEnabled(bool bEnable);

    int getLimits(double &dHoursEast, double &dHoursWest);

    int Abort();

private:

    SerXInterface                       *m_pSerx;
    LoggerInterface                     *m_pLogger;
    TheSkyXFacadeForDriversInterface    *m_pTsx;
    SleeperInterface                    *m_pSleeper;

    bool    m_bDebugLog;
    char    m_szLogBuffer[PLUGIN_LOG_BUFFER_SIZE];

	bool    m_bIsConnected;                               // Connected to the mount?
    char    m_szFirmwareVersion[SERIAL_BUFFER_SIZE];

    char    m_szHardwareModel[SERIAL_BUFFER_SIZE];
    char    m_szTime[SERIAL_BUFFER_SIZE];
    char    m_szDate[SERIAL_BUFFER_SIZE];
    int     m_nSiteNumber;

	double  m_dGotoRATarget;						  // Current Target RA;
	double  m_dGotoDECTarget;                      // Current Goto Target Dec;

    bool    m_bJNOW;
    bool    m_b24h;
    bool    m_bDdMmYy;
    bool    m_bTimeSetOnce;
    MountDriverInterface::MoveDir      m_nOpenLoopDir;

    // limits don't change mid-course so we cache them
    bool    m_bLimitCached;
    double  m_dHoursEast;
    double  m_dHoursWest;


    int     slewTargetRADec();

    int     setCustomTRateOffsetRA(double dRa);
    int     setCustomTRateOffsetDec(double dDec);
    int     getCustomTRateOffsetRA(double &dTrackRaArcSecPerHr);
    int     getCustomTRateOffsetDec(double &dTrackDecArcSecPerHr);

    int     getSoftLimitEastAngle(double &dAngle);
    int     getSoftLimitWestAngle(double &dAngle);

    int     parseFields(const char *pszIn, std::vector<std::string> &svFields, char cSeparator);

    const char m_aszSlewRateNames[PLUGIN_NB_SLEW_SPEEDS][PLUGIN_SLEW_NAME_LENGHT] = { "ViewVel 1", "ViewVel 2", "ViewVel 3", "ViewVel 4",  "Slew"};
    const char m_szAlignmentType[PLUGIN_NB_ALIGNEMENT_TYPE][PLUGIN_ALIGNEMENT_NAME_LENGHT] = { "Polar", "AltAz", "NearlyPolar", "NearlyAltAz"};

    // SiTech specific
    int     sendCommand(const std::string sCmd, char *pszResult, int nResultMaxLen);
    int     readResponse(char *respBuffer, int nBufferLen, int nTimeout = MAX_TIMEOUT);

    int     getRaAzTickPerRev(int &nRaAzTickPerRev);
    int     getDecAltTickPerRev(int &nDecAltTickPerRev);

    int     countsPerSecToSpeedValue(int cps);
    int     speedValueToCountsPerSec(int speed);
    int     degsPerSec2MotorSpeed(double dDegsPerSec, int nTicksPerRev);
    double  motorSpeed2DegsPerSec(int nSpeed,  int nTicksPerRev);

    double  stepToDeg(int nSteps, int nTicksPerRev);
    int     degToSteps(double dDegs,  int nTicksPerRev);

    int     m_nRaAzTickPerRev;
    int     m_nDecAltTickPerRev;

    CStopWatch      timer;


#ifdef PLUGIN_DEBUG
    std::string m_sLogfilePath;
	// timestamp for logs
    char *timestamp;
	time_t ltime;
	FILE *Logfile;	  // LogFile
#endif

};


