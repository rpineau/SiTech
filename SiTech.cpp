#include "SiTech.h"

// Constructor for SiTech
SiTech::SiTech()
{

	m_bIsConnected = false;

    m_bJNOW = false;

    m_b24h = false;
    m_bDdMmYy = false;
    m_bTimeSetOnce = false;
    m_bLimitCached = false;

    m_nRaAzTickPerRev = 1;
    m_nDecAltTickPerRev = 1;
    
    m_dRaAzRate = SIDEREAL_TRACKING_SPEED;
    m_dDecAltRate = 0;
    m_bTracking = false;
    
    m_dServoFreq = SERVO_FREQ;
    
#ifdef PLUGIN_DEBUG
#if defined(SB_WIN_BUILD)
    m_sLogfilePath = getenv("HOMEDRIVE");
    m_sLogfilePath += getenv("HOMEPATH");
    m_sLogfilePath += "\\SiTechLog.txt";
#elif defined(SB_LINUX_BUILD)
    m_sLogfilePath = getenv("HOME");
    m_sLogfilePath += "/SiTechLog.txt";
#elif defined(SB_MAC_BUILD)
    m_sLogfilePath = getenv("HOME");
    m_sLogfilePath += "/SiTechLog.txt";
#endif
	Logfile = fopen(m_sLogfilePath.c_str(), "w");
#endif

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    ltime = time(NULL);
	timestamp = asctime(localtime(&ltime));
	timestamp[strlen(timestamp) - 1] = 0;
	fprintf(Logfile, "[%s] SiTech New Constructor Called\n", timestamp);
    fprintf(Logfile, "[%s] [SiTech::SiTech] version %3.2f build 2020_05_21_12_45.\n", timestamp, DRIVER_VERSION);
    fflush(Logfile);
#endif

}


SiTech::~SiTech(void)
{
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
	ltime = time(NULL);
	timestamp = asctime(localtime(&ltime));
	timestamp[strlen(timestamp) - 1] = 0;
	fprintf(Logfile, "[%s] SiTech Destructor Called\n", timestamp );
    fflush(Logfile);
#endif
#ifdef PLUGIN_DEBUG
    // Close LogFile
    if (Logfile) fclose(Logfile);
#endif
}

int SiTech::Connect(char *pszPort)
{
    int nErr = PLUGIN_OK;
    char szFirmware[SERIAL_BUFFER_SIZE];
    
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
	ltime = time(NULL);
	timestamp = asctime(localtime(&ltime));
	timestamp[strlen(timestamp) - 1] = 0;
	fprintf(Logfile, "[%s] SiTech::Connect Called %s\n", timestamp, pszPort);
    fflush(Logfile);
#endif

    // 19200 8N1
    if(m_pSerx->open(pszPort, 19200, SerXInterface::B_NOPARITY, "-DTR_CONTROL 1") == 0)
        m_bIsConnected = true;
    else
        m_bIsConnected = false;

    if(!m_bIsConnected)
        return ERR_COMMNOLINK;
    timer.Reset();
    m_bLimitCached = false;
    m_pSerx->purgeTxRx();
    
    nErr = getFirmwareVersion(szFirmware, SERIAL_BUFFER_SIZE);
    if(nErr) {
        m_bIsConnected = false;
    }
    nErr = getRaAzTickPerRev(m_nRaAzTickPerRev);
    nErr = getDecAltTickPerRev(m_nDecAltTickPerRev);
    m_bNorthHemisphere = (m_pTSX->latitude() > 0);
    
    return nErr;
}


int SiTech::Disconnect(void)
{
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
	ltime = time(NULL);
	timestamp = asctime(localtime(&ltime));
	timestamp[strlen(timestamp) - 1] = 0;
	fprintf(Logfile, "[%s] SiTech::Disconnect Called\n", timestamp);
    fflush(Logfile);
#endif
	if (m_bIsConnected) {
        if(m_pSerx){
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
            ltime = time(NULL);
            timestamp = asctime(localtime(&ltime));
            timestamp[strlen(timestamp) - 1] = 0;
            fprintf(Logfile, "[%s] SiTech::Disconnect closing serial port\n", timestamp);
            fflush(Logfile);
#endif
            m_pSerx->flushTx();
            m_pSerx->purgeTxRx();
            m_pSerx->close();
        }
    }
	m_bIsConnected = false;
    m_bLimitCached = false;

	return SB_OK;
}


int SiTech::getNbSlewRates()
{
    return PLUGIN_NB_SLEW_SPEEDS;
}

// returns "Slew", "ViewVel4", "ViewVel3", "ViewVel2", "ViewVel1"
int SiTech::getRateName(int nZeroBasedIndex, char *pszOut, unsigned int nOutMaxSize)
{
    if (nZeroBasedIndex > PLUGIN_NB_SLEW_SPEEDS)
        return PLUGIN_ERROR;

    strncpy(pszOut, m_aszSlewRateNames[nZeroBasedIndex], nOutMaxSize);

    return PLUGIN_OK;
}

#pragma mark - SiTech communication

int SiTech::sendCommand(const std::string sCmd, char *pszResult, int nResultMaxLen)
{
    int nErr = PLUGIN_OK;
    char szResp[SERIAL_BUFFER_SIZE];
    unsigned long  ulBytesWrite;
    
    if(!m_bIsConnected)
        return ERR_COMMNOLINK;

    m_pSerx->purgeTxRx();
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [SiTech::sendCommand] Sending : %s\n", timestamp, sCmd.c_str());
    fflush(Logfile);
#endif

    nErr = m_pSerx->writeFile((void *) (sCmd.c_str()) , sCmd.size(), ulBytesWrite);
    m_pSerx->flushTx();

    if(nErr){
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [SiTech::sendCommand] writeFile Error : %d\n", timestamp, nErr);
        fflush(Logfile);
#endif
        return nErr;
    }

    if(pszResult) {
        // read response
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [SiTech::sendCommand] Getting response\n", timestamp);
        fflush(Logfile);
#endif

        nErr = readResponse(szResp, SERIAL_BUFFER_SIZE);
        if(nErr){
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
            ltime = time(NULL);
            timestamp = asctime(localtime(&ltime));
            timestamp[strlen(timestamp) - 1] = 0;
            fprintf(Logfile, "[%s] [SiTech::sendCommand] readResponse Error : %d\n", timestamp, nErr);
            fflush(Logfile);
#endif
            return nErr;
        }
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [SiTech::sendCommand] response : '%s'\n", timestamp, szResp);
        fflush(Logfile);
#endif
        strncpy(pszResult, szResp, nResultMaxLen);
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [SiTech::sendCommand] response copied to pszResult : \"%s\"\n", timestamp, pszResult);
        fflush(Logfile);
#endif
    }
    return nErr;
}

int SiTech::readResponse(char *szRespBuffer, int nBufferLen, int nTimeout )
{
    int nErr = PLUGIN_OK;
    unsigned long ulBytesRead = 0;
    unsigned long ulTotalBytesRead = 0;
    char *pszBufPtr;
    std::string sTmp;
    std::string sResp;
    
    memset(szRespBuffer, 0, (size_t) nBufferLen);
    pszBufPtr = szRespBuffer;

    do {
        nErr = m_pSerx->readFile(pszBufPtr, 1, ulBytesRead, nTimeout);
        if(nErr) {
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
            ltime = time(NULL);
            timestamp = asctime(localtime(&ltime));
            timestamp[strlen(timestamp) - 1] = 0;
            fprintf(Logfile, "[%s] [SiTech::readResponse] readFile error\n", timestamp);
            fflush(Logfile);
#endif
            return nErr;
        }

        if (ulBytesRead !=1) {// timeout
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
            ltime = time(NULL);
            timestamp = asctime(localtime(&ltime));
            timestamp[strlen(timestamp) - 1] = 0;
            fprintf(Logfile, "[%s] SiTech::readResponse Timeout while waiting for response from controller\n", timestamp);
            fflush(Logfile);
#endif
            nErr = ERR_DATAOUT;
            break;
        }
        ulTotalBytesRead += ulBytesRead;
    } while (*pszBufPtr++ != 0x0a && ulTotalBytesRead < nBufferLen );

    if(ulTotalBytesRead) {
        //cleanup the string
        sTmp.assign(szRespBuffer);
        sResp = trim(sTmp," \n\r");
        strncpy(szRespBuffer, sResp.c_str(), SERIAL_BUFFER_SIZE);
    }
        
    #if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
                ltime = time(NULL);
                timestamp = asctime(localtime(&ltime));
                timestamp[strlen(timestamp) - 1] = 0;
                fprintf(Logfile, "[%s] [SiTech::readResponse] response = %s\n", timestamp, szRespBuffer);
                fflush(Logfile);
    #endif

    return nErr;
}


#pragma mark - dome controller informations

int SiTech::getFirmwareVersion(char *pszVersion, unsigned int nStrMaxLen)
{
    int nErr = PLUGIN_OK;
    char szResp[SERIAL_BUFFER_SIZE];

    if(!m_bIsConnected)
        return NOT_CONNECTED;

    nErr = sendCommand(std::string("XV\r"), szResp, SERIAL_BUFFER_SIZE);
    if(nErr)
        return nErr;

    strncpy(pszVersion, szResp, nStrMaxLen);
    strncpy(m_szFirmwareVersion, szResp, nStrMaxLen);
    return nErr;
}


#pragma mark - Mount Coordinates

int SiTech::getHaAndDec(double &dHa, double &dDec)
{
    int nErr = PLUGIN_OK;
    char szResp[SERIAL_BUFFER_SIZE];
    int nTmp = 0;
    
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [SiTech::getRaAndDec] ===========================\n", timestamp);
    fflush(Logfile);
#endif

    // get Ra/Az
    nErr = sendCommand(std::string("Y\r"), szResp, SERIAL_BUFFER_SIZE);
    if(nErr) {
        return nErr;
    }
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [SiTech::getRaAndDec] Ra/Az szResp = %s\n", timestamp, szResp);
    fflush(Logfile);
#endif
    if(szResp[0] == 'Y') {
        m_nRaAzPos = atoi(&szResp[1]);
        dHa = stepToHa(m_nRaAzPos, m_nRaAzTickPerRev);

        #if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
            ltime = time(NULL);
            timestamp = asctime(localtime(&ltime));
            timestamp[strlen(timestamp) - 1] = 0;
            fprintf(Logfile, "[%s] [SiTech::getRaAndDec] Ra/Az m_nRaAzPos = %d\n", timestamp, m_nRaAzPos);
            fprintf(Logfile, "[%s] [SiTech::getRaAndDec] Ra/Az dHa = %f\n", timestamp, dHa);
            fflush(Logfile);
        #endif
    }
    else
        return COMMAND_FAILED;
    

    // get Dec/Alt
    nErr = sendCommand(std::string("X\r"), szResp, SERIAL_BUFFER_SIZE);
    if(nErr) {
        return nErr;
    }
    #if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [SiTech::getRaAndDec] Dec/Alt szResp = %s\n", timestamp, szResp);
        fflush(Logfile);
    #endif
    if(szResp[0] == 'X') {
        m_nDecAltPos = atoi(&szResp[1]);
        dDec = stepToDeg(m_nDecAltPos, m_nDecAltTickPerRev);
        #if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
            ltime = time(NULL);
            timestamp = asctime(localtime(&ltime));
            timestamp[strlen(timestamp) - 1] = 0;
            fprintf(Logfile, "[%s] [SiTech::getRaAndDec] Ra/Az m_nDecAltPos = %d\n", timestamp, m_nDecAltPos);
            fprintf(Logfile, "[%s] [SiTech::getRaAndDec] Ra/Az dDEc = %f\n", timestamp, dDec);
            fflush(Logfile);
        #endif
    }
    else
        return COMMAND_FAILED;

    #if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [SiTech::getRaAndDec] Ha  : %f\n", timestamp, dHa);
        fprintf(Logfile, "[%s] [SiTech::getRaAndDec] Dec : %f\n", timestamp, dDec);
        fprintf(Logfile, "[%s] [SiTech::getRaAndDec] ---------------------------\n", timestamp);
        fflush(Logfile);
    #endif

    return nErr;
}


#pragma mark - Sync and Cal
int SiTech::syncTo(double dRa, double dDec)
{
    int nErr = PLUGIN_OK;
    int nStepPos;
    double dHa;
    std::string sCmd;

    dHa = m_pTSX->hourAngle(dRa);

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [SiTech::syncTo] Ra  : %f\n", timestamp, dRa);
    fprintf(Logfile, "[%s] [SiTech::syncTo] dHa : %f\n", timestamp, dHa);
    fprintf(Logfile, "[%s] [SiTech::syncTo] Dec : %f\n", timestamp, dDec);
    fflush(Logfile);
#endif
    
    nStepPos = haToSteps(dHa, m_nRaAzTickPerRev);
    #if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [SiTech::syncTo] dHa nStepPos : %d\n", timestamp, nStepPos);
        fflush(Logfile);
    #endif


    sCmd = "YF";
    sCmd += std::to_string(nStepPos);
    sCmd += "\r";
    nErr = sendCommand(sCmd, nullptr, SERIAL_BUFFER_SIZE);
    if(nErr)
        return nErr;

    nStepPos = degToSteps(dDec, m_nDecAltTickPerRev);
    #if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [SiTech::syncTo] Dec nStepPos : %d\n", timestamp, nStepPos);
        fflush(Logfile);
    #endif
    sCmd = "XF";
    sCmd += std::to_string(nStepPos);
    sCmd += "\r";
    nErr = sendCommand(sCmd, nullptr, SERIAL_BUFFER_SIZE);
    if(nErr)
        return nErr;

    //now enable tracking at sidereal rate
    setTrackingRates(true, true, 0, 0);
    m_bIsSynced = true;
    
    return nErr;

}

void SiTech::isSynced(bool &bSynced)
{
    bSynced = m_bIsSynced;
    
    #if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
            ltime = time(NULL);
            timestamp = asctime(localtime(&ltime));
            timestamp[strlen(timestamp) - 1] = 0;
            fprintf(Logfile, "[%s] [SiTech::isSynced] m_bIsSynced = %s\n", timestamp, m_bIsSynced?"True":"False");
            fflush(Logfile);
    #endif
}

#pragma mark - tracking rates
int SiTech::setTrackingRates(bool bTrackingOn, bool bIgnoreRates, double dRaRateArcSecPerSec, double dDecRateArcSecPerSec)
{
    int nErr = PLUGIN_OK;
    std::string sCmd;
    int nTarget;
    double dRaSpeed;
    int nMotorSpeed;
    
    if(!bTrackingOn) { // stop tracking
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [SiTech::setTrackingRates] setting to Drift\n", timestamp);
        fflush(Logfile);
#endif
        nErr = sendCommand(std::string("YN\r"), nullptr, SERIAL_BUFFER_SIZE);
        nErr |= sendCommand(std::string("XN\r"), nullptr, SERIAL_BUFFER_SIZE);
        
        m_dRaAzRate = 15.0410681;
        m_dDecAltRate = 0;
        m_bTracking = false;
    }
    else if(bTrackingOn ) {
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [SiTech::setTrackingRates] setting to Sidereal\n", timestamp);
        fflush(Logfile);
#endif
        // Sidereal = 15.0410681 arcsecPerSec
        // dRaSpeed = Sidereal - dRaRateArcSecPerSec
        dRaSpeed = SIDEREAL_TRACKING_SPEED - dRaRateArcSecPerSec;
        nMotorSpeed = arcsecPerSec2MotorSpeed(dRaSpeed, m_nRaAzTickPerRev);
        #if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
                ltime = time(NULL);
                timestamp = asctime(localtime(&ltime));
                timestamp[strlen(timestamp) - 1] = 0;
                fprintf(Logfile, "[%s] [SiTech::setTrackingRates] nMotorSpeed Ra : %d\n", timestamp, nMotorSpeed);
                fflush(Logfile);
        #endif

        // Target should be set depending on tracking sign
        if(m_bNorthHemisphere) {
            nTarget = FarFarAway;
        }
        else {
            nTarget = -FarFarAway;
            nMotorSpeed = abs(nMotorSpeed);
        }
        
        sCmd = "Y";
        sCmd += std::to_string(nTarget);
        sCmd += "S";
        sCmd += std::to_string(nMotorSpeed);
        sCmd += "\r";
        nErr = sendCommand(sCmd, nullptr, SERIAL_BUFFER_SIZE);
        if(nErr)
            return nErr;

        m_dRaAzRate = dRaRateArcSecPerSec;
        
        // Dec
        nMotorSpeed = arcsecPerSec2MotorSpeed(dDecRateArcSecPerSec, m_nRaAzTickPerRev);
        nTarget = FarFarAway;
        sCmd = "X";
        sCmd += std::to_string(nTarget);
        sCmd += "S";
        sCmd += std::to_string(nMotorSpeed);
        sCmd += "\r";
        nErr = sendCommand(sCmd, nullptr, SERIAL_BUFFER_SIZE);
        if(nErr)
            return nErr;

        m_dDecAltRate = dDecRateArcSecPerSec;

        m_bTracking = true;
    }

    #if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [SiTech::setTrackingRates] m_bTracking = %s\n", timestamp, m_bTracking?"True":"False");
        fprintf(Logfile, "[%s] [SiTech::setTrackingRates] m_dRaAzRate = %f\n", timestamp, m_dRaAzRate);
        fprintf(Logfile, "[%s] [SiTech::setTrackingRates] m_dDecAltRate = %f\n", timestamp, m_dDecAltRate);
        fflush(Logfile);
    #endif

    return nErr;
}

int SiTech::getTrackRates(bool &bTrackingOn, double &dRaRateArcSecPerSec, double &dDecRateArcSecPerSec)
{
    int nErr = PLUGIN_OK;

    dRaRateArcSecPerSec = m_dRaAzRate;
    dDecRateArcSecPerSec = m_dDecAltRate;
    bTrackingOn = m_bTracking;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [SiTech::getTrackRates] bTrackingOn = %s\n", timestamp, bTrackingOn?"True":"False");
    fprintf(Logfile, "[%s] [SiTech::getTrackRates] dRaRateArcSecPerSec = %f\n", timestamp, dRaRateArcSecPerSec);
    fprintf(Logfile, "[%s] [SiTech::getTrackRates] dDecRateArcSecPerSec = %f\n", timestamp, dDecRateArcSecPerSec);
    fflush(Logfile);
#endif

    return nErr;
}

#pragma mark - Limis
int SiTech::getLimits(double &dHoursEast, double &dHoursWest)
{
    int nErr = PLUGIN_OK;
    double dEast, dWest;

    if(m_bLimitCached) {
        dHoursEast = m_dHoursEast;
        dHoursWest = m_dHoursWest;
        return nErr;
    }

    nErr = getSoftLimitEastAngle(dEast);
    if(nErr)
        return nErr;

    nErr = getSoftLimitWestAngle(dWest);
    if(nErr)
        return nErr;

    dHoursEast = m_pTSX->hourAngle(dEast);
    dHoursWest = m_pTSX->hourAngle(dWest);

    m_bLimitCached = true;
    m_dHoursEast = dHoursEast;
    m_dHoursWest = dHoursWest;
    return nErr;
}

int SiTech::getSoftLimitEastAngle(double &dAngle)
{
    int nErr = PLUGIN_OK;
    char szResp[SERIAL_BUFFER_SIZE];

    // nErr = sendCommand("!NGle;", szResp, SERIAL_BUFFER_SIZE);
    if(nErr)
        return nErr;
    dAngle = atof(szResp);

    return nErr;
}

int SiTech::getSoftLimitWestAngle(double &dAngle)
{
    int nErr = PLUGIN_OK;
    char szResp[SERIAL_BUFFER_SIZE];

    // nErr = sendCommand("!NGlw;", szResp, SERIAL_BUFFER_SIZE);
    if(nErr)
        return nErr;
    dAngle = atof(szResp);

    return nErr;
}

#pragma mark - Slew

int SiTech::startSlewTo(double dRa, double dDec)
{
    int nErr = PLUGIN_OK;
    bool bAligned;

    // set sync target coordinate
    // nErr = setTarget(dRa, dDec);
    if(nErr)
        return nErr;

    slewTargetRADec();
    return nErr;
}

int SiTech::slewTargetRADec()
{
    int nErr = PLUGIN_OK;
    char szResp[SERIAL_BUFFER_SIZE];

    // nErr = sendCommand("!GTrn;", szResp, SERIAL_BUFFER_SIZE);

    timer.Reset();
    return nErr;

}

int SiTech::startOpenSlew(const MountDriverInterface::MoveDir Dir, unsigned int nRate)
{
    int nErr = PLUGIN_OK;
    char szCmd[SERIAL_BUFFER_SIZE];
    char szResp[SERIAL_BUFFER_SIZE];

    m_nOpenLoopDir = Dir;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [SiTech::startOpenSlew] setting to Dir %d\n", timestamp, Dir);
    fprintf(Logfile, "[%s] [SiTech::startOpenSlew] Setting rate to %d\n", timestamp, nRate);
    fflush(Logfile);
#endif

    // select rate
    if(nRate == 4) { // "Slew"
        // nErr = sendCommand("!KSsl;", szResp, SERIAL_BUFFER_SIZE);
    }
    else {
        // clear slew
        // nErr = sendCommand("!KCsl;", szResp, SERIAL_BUFFER_SIZE);
        // select rate
        // KScv + 1,2 3 or 4 for ViewVel 1,2,3,4, 'ViewVel 1' is index 0 so nRate+1
        snprintf(szCmd, SERIAL_BUFFER_SIZE, "!KScv%d;", nRate+1);
    }
    // figure out direction
    switch(Dir){
        case MountDriverInterface::MD_NORTH:
            // nErr = sendCommand("!KSpu100;", szResp, SERIAL_BUFFER_SIZE);
            break;
        case MountDriverInterface::MD_SOUTH:
            // nErr = sendCommand("!KSpd100;", szResp, SERIAL_BUFFER_SIZE);
            break;
        case MountDriverInterface::MD_EAST:
            // nErr = sendCommand("!KSpl100;", szResp, SERIAL_BUFFER_SIZE);
            break;
        case MountDriverInterface::MD_WEST:
            // nErr = sendCommand("!KSsr100;", szResp, SERIAL_BUFFER_SIZE);
            break;
    }

    return nErr;
}

int SiTech::stopOpenLoopMove()
{
    int nErr = PLUGIN_OK;
    char szResp[SERIAL_BUFFER_SIZE];

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [SiTech::stopOpenLoopMove] Dir was %d\n", timestamp, m_nOpenLoopDir);
    fflush(Logfile);
#endif

    switch(m_nOpenLoopDir){
        case MountDriverInterface::MD_NORTH:
        case MountDriverInterface::MD_SOUTH:
            // nErr = sendCommand("!XXud;", szResp, SERIAL_BUFFER_SIZE);
            break;
        case MountDriverInterface::MD_EAST:
        case MountDriverInterface::MD_WEST:
            // nErr = sendCommand("!XXlr;", szResp, SERIAL_BUFFER_SIZE);
            break;
    }

    return nErr;
}

int SiTech::isSlewToComplete(bool &bComplete)
{
    int nErr = PLUGIN_OK;
    char szResp[SERIAL_BUFFER_SIZE];
    int nPrecentRemaining;

    bComplete = false;

    // debug
    bComplete = true;
    return nErr;
    
    // nErr = sendCommand("!GGgr;", szResp, SERIAL_BUFFER_SIZE);
    if(nErr)
        return nErr;
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [SiTech::isSlewToComplete] szResp : %s\n", timestamp, szResp);
    fflush(Logfile);
#endif

    // remove the %
    szResp[strlen(szResp) -1 ] = 0;
    
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [SiTech::isSlewToComplete] szResp : %s\n", timestamp, szResp);
    fflush(Logfile);
#endif

    nPrecentRemaining = atoi(szResp);
    if(nPrecentRemaining == 0)
        bComplete = true;

    return nErr;
}

int SiTech::gotoPark(double dRa, double dDec)
{
    int nErr = PLUGIN_OK;
    char szResp[SERIAL_BUFFER_SIZE];

    // set park position ?
    // or goto ?
    // goto park
    // nErr = sendCommand("!GTop;", szResp, SERIAL_BUFFER_SIZE);

    return nErr;
}

int SiTech::markParkPosition()
{
    int nErr = PLUGIN_OK;
    char szResp[SERIAL_BUFFER_SIZE];

    // nErr = sendCommand("!AMpp;", szResp, SERIAL_BUFFER_SIZE);

    return nErr;

}

int SiTech::getAtPark(bool &bParked)
{
    int nErr = PLUGIN_OK;
    char szResp[SERIAL_BUFFER_SIZE];

    bParked = false;
    // nErr = sendCommand("!AGak;", szResp, SERIAL_BUFFER_SIZE);
    if(strncmp(szResp,"Yes",SERIAL_BUFFER_SIZE) == 0) {
        bParked = true;
    }
    return nErr;
}

int SiTech::unPark()
{
    int nErr = PLUGIN_OK;
    return nErr;
}


int SiTech::getRefractionCorrEnabled(bool &bEnabled)
{
    int nErr = PLUGIN_OK;
    char szResp[SERIAL_BUFFER_SIZE];

    bEnabled = false;
    // nErr = sendCommand("!PGre;", szResp, SERIAL_BUFFER_SIZE);
    if(strncmp(szResp,"Yes",SERIAL_BUFFER_SIZE) == 0) {
        bEnabled = true;
    }
    return nErr;
}

int SiTech::setRefractionCorrEnabled(bool bEnable)
{
    int nErr = PLUGIN_OK;
    char szResp[SERIAL_BUFFER_SIZE];
    char szCmd[SERIAL_BUFFER_SIZE];

    if(!m_bIsConnected)
        return NOT_CONNECTED;
    if(bEnable) {
        snprintf(szCmd, SERIAL_BUFFER_SIZE, "!PSreYes;");
    }
    else  {
        snprintf(szCmd, SERIAL_BUFFER_SIZE, "!PSreNo;");
    }
    // nErr = sendCommand(szCmd, szResp, SERIAL_BUFFER_SIZE);

    return nErr;
}


int SiTech::Abort()
{
    int nErr = PLUGIN_OK;

    nErr = sendCommand(std::string("XN\r"), nullptr, SERIAL_BUFFER_SIZE);
    nErr |= sendCommand(std::string("YN\r"), nullptr, SERIAL_BUFFER_SIZE);

    return nErr;
}


#pragma mark - Special commands & functions

int SiTech::countsPerSecToSpeedValue(int cps)
{
    return int(round(cps * 33.55657962109575));
}

int SiTech::speedValueToCountsPerSec(int speed)
{
    return int(speed * 0.0298004150390625);
}


int SiTech::degsPerSec2MotorSpeed(double dDegsPerSec, int nTicksPerRev)
{
    double d1 = 1.0 / (m_dServoFreq * 0.0054931641);
    double r1 = double(nTicksPerRev) * dDegsPerSec * d1;
    return int(r1);
}

int SiTech::arcsecPerSec2MotorSpeed(double arcsecPerSec, int nTicksPerRev)
{
    double d1 = 1.0 / (m_dServoFreq * 0.0054931641);
    double r1 = double(nTicksPerRev) * (arcsecPerSec/3600) * d1;
    return int(r1);
}

double SiTech::motorSpeed2ArcsecPerSec(int nSpeed,  int nTicksPerRev)
{
    return ((double(nSpeed) / double(nTicksPerRev)) * 10.7281494140625)*3600;
}

double SiTech::motorSpeed2DegsPerSec(int nSpeed,  int nTicksPerRev)
{
    return (double(nSpeed) / double(nTicksPerRev)) * 10.7281494140625;
}


int SiTech::getRaAzTickPerRev(int &nRaAzTickPerRev)
{
    int nErr = PLUGIN_OK;
    char szResp[SERIAL_BUFFER_SIZE];

    if(!m_bIsConnected)
        return NOT_CONNECTED;

    nErr = sendCommand(std::string("XXV\r"), szResp, SERIAL_BUFFER_SIZE);
    if(nErr)
        return nErr;
    if(szResp[0] == 'V') {
        nRaAzTickPerRev = atoi(&szResp[1]);
        m_nRaAzTickPerRev = nRaAzTickPerRev;
    }

    return nErr;
}

int SiTech::getDecAltTickPerRev(int &nDecAltTickPerRev)
{
    int nErr = PLUGIN_OK;
    char szResp[SERIAL_BUFFER_SIZE];

    if(!m_bIsConnected)
        return NOT_CONNECTED;

    nErr = sendCommand(std::string("XXU\r"), szResp, SERIAL_BUFFER_SIZE);
    if(nErr)
        return nErr;
    if(szResp[0] == 'U') {
        nDecAltTickPerRev = atoi(&szResp[1]);
        m_nDecAltTickPerRev = nDecAltTickPerRev;
    }
    return nErr;
}

double  SiTech::stepToDeg(int nSteps, int nTicksPerRev)
{
    return (double(nSteps)/double(nTicksPerRev)) * 360.0;
}

double  SiTech::stepToHa(int nSteps, int nTicksPerRev)
{
    return (double(nSteps)/double(nTicksPerRev)) * 24.0;
}

int SiTech::degToSteps(double dDegs,  int nTicksPerRev)
{
    return int((double(nTicksPerRev) * dDegs)/ 360.0);
}

int SiTech::haToSteps(double dHa, int nTicksPerRev)
{
    return int((double(nTicksPerRev) * dHa)/ 24.0);
}


int SiTech::parseFields(const char *pszIn, std::vector<std::string> &svFields, char cSeparator)
{
    int nErr = PLUGIN_OK;
    std::string sSegment;
    std::stringstream ssTmp(pszIn);

    svFields.clear();
    // split the string into vector elements
    while(std::getline(ssTmp, sSegment, cSeparator))
    {
        svFields.push_back(sSegment);
    }

    if(svFields.size()==0) {
        nErr = ERR_PARSE;
    }
    return nErr;
}

std::string& SiTech::trim(std::string &str, const std::string& filter )
{
    return ltrim(rtrim(str, filter), filter);
}

std::string& SiTech::ltrim(std::string& str, const std::string& filter)
{
    str.erase(0, str.find_first_not_of(filter));
    return str;
}

std::string& SiTech::rtrim(std::string& str, const std::string& filter)
{
    str.erase(str.find_last_not_of(filter) + 1);
    return str;
}

std::string SiTech::findField(std::vector<std::string> &svFields, const std::string& token)
{
    for(int i=0; i<svFields.size(); i++){
        if(svFields[i].find(token)!= -1) {
            return svFields[i];
        }
    }
    return std::string();
}

