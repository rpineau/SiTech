#include "SiTech.h"

// Constructor for SiTech
SiTech::SiTech()
{

	m_bIsConnected = false;

    m_bDebugLog = true;
    m_bJNOW = false;

    m_b24h = false;
    m_bDdMmYy = false;
    m_bTimeSetOnce = false;
    m_bLimitCached = false;

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

    nErr = getFirmwareVersion(szFirmware, SERIAL_BUFFER_SIZE);
    if(nErr) {
        m_bIsConnected = false;
    }
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

    if(ulTotalBytesRead)
        *(pszBufPtr-1) = 0; //remove the \n

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

int SiTech::getRaAndDec(double &dRa, double &dDec)
{
    int nErr = PLUGIN_OK;
    char szResp[SERIAL_BUFFER_SIZE];
    int nTmp;
    
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
        nTmp = atoi(&szResp[1]);
        dRa = stepToDeg(nTmp, m_nRaAzTickPerRev);
    }
    
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
        nTmp = atoi(&szResp[1]);
        dRa = stepToDeg(nTmp, m_nDecAltTickPerRev);
    }

    #if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [SiTech::getRaAndDec] Ra : %f\n", timestamp, dRa);
        fprintf(Logfile, "[%s] [SiTech::getRaAndDec] Dec : %f\n", timestamp, dDec);
        fflush(Logfile);
    #endif

    return nErr;
}


#pragma mark - Sync and Cal
int SiTech::syncTo(double dRa, double dDec)
{
    int nErr = PLUGIN_OK;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [SiTech::syncTo] Ra : %f\n", timestamp, dRa);
    fprintf(Logfile, "[%s] [SiTech::syncTo] Dec : %f\n", timestamp, dDec);
    fflush(Logfile);
#endif

    return nErr;
}

#pragma mark - tracking rates
int SiTech::setTrackingRates(bool bTrackingOn, bool bIgnoreRates, double dTrackRaArcSecPerHr, double dTrackDecArcSecPerHr)
{
    int nErr = PLUGIN_OK;
    char szResp[SERIAL_BUFFER_SIZE];

    if(!bTrackingOn) { // stop tracking
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [SiTech::setTrackingRates] setting to Drift\n", timestamp);
        fflush(Logfile);
#endif
        // nErr = sendCommand("!RStrDrift;", szResp, SERIAL_BUFFER_SIZE);
    }
    else if(bTrackingOn && bIgnoreRates) { // sidereal
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [SiTech::setTrackingRates] setting to Sidereal\n", timestamp);
        fflush(Logfile);
#endif
        // nErr = sendCommand("!RStrSidereal;", szResp, SERIAL_BUFFER_SIZE);
    }
    else { // custom rate
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [SiTech::setTrackingRates] setting to Custom\n", timestamp);
        fprintf(Logfile, "[%s] [SiTech::setTrackingRates] dTrackRaArcSecPerHr = %f\n", timestamp, dTrackRaArcSecPerHr);
        fprintf(Logfile, "[%s] [SiTech::setTrackingRates] dTrackDecArcSecPerHr = %f\n", timestamp, dTrackDecArcSecPerHr);
        fflush(Logfile);
#endif
        snprintf(m_szLogBuffer,PLUGIN_LOG_BUFFER_SIZE,"[SiTech::setTrackingRates] Setting customr tracking rate. dTrackRaArcSecPerHr = %f , dTrackDecArcSecPerHr = %f", dTrackRaArcSecPerHr, dTrackDecArcSecPerHr);
        m_pLogger->out(m_szLogBuffer);
        nErr = setCustomTRateOffsetRA(dTrackRaArcSecPerHr);
        nErr |= setCustomTRateOffsetDec(dTrackDecArcSecPerHr);
        if(nErr) {
            return nErr; // if we cant set the rate no need to switch to custom.
        }
        // nErr = sendCommand("!RStrCustom;", szResp, SERIAL_BUFFER_SIZE);
    }
    return nErr;
}

int SiTech::getTrackRates(bool &bTrackingOn, double &dTrackRaArcSecPerHr, double &dTrackDecArcSecPerHr)
{
    int nErr = PLUGIN_OK;
    char szResp[SERIAL_BUFFER_SIZE];

    // nErr = sendCommand("!RGtr;", szResp, SERIAL_BUFFER_SIZE);
    bTrackingOn = true;
    if(strncmp(szResp, "Drift", SERIAL_BUFFER_SIZE)==0) {
        bTrackingOn = false;
    }
    nErr = getCustomTRateOffsetRA(dTrackRaArcSecPerHr);
    nErr |= getCustomTRateOffsetDec(dTrackDecArcSecPerHr);

    return nErr;
}

int SiTech::setCustomTRateOffsetRA(double dRa)
{
    int nErr = PLUGIN_OK;
    char szCmd[SERIAL_BUFFER_SIZE];
    char szResp[SERIAL_BUFFER_SIZE];

    snprintf(szCmd, SERIAL_BUFFER_SIZE, "!RSor%.2f;", dRa);
    // nErr = sendCommand(szCmd, szResp, SERIAL_BUFFER_SIZE);
    if(nErr) {
#if defined PLUGIN_DEBUG
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [SiTech::setCustomTRateOffsetRA] Error setting Ra tracking rate to %f\n", timestamp, dRa);
        fflush(Logfile);
#endif
    }
    return nErr;
}

int SiTech::setCustomTRateOffsetDec(double dDec)
{
    int nErr = PLUGIN_OK;
    char szCmd[SERIAL_BUFFER_SIZE];
    char szResp[SERIAL_BUFFER_SIZE];

    snprintf(szCmd, SERIAL_BUFFER_SIZE, "!RSod%.2f;", dDec);
    // nErr = sendCommand(szCmd, szResp, SERIAL_BUFFER_SIZE);
    if(nErr) {
#if defined PLUGIN_DEBUG
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [SiTech::setCustomTRateOffsetDec] Error setting Dec tracking rate to %f\n", timestamp, dDec);
        fflush(Logfile);
#endif
    }
    return nErr;
}

int SiTech::getCustomTRateOffsetRA(double &dTrackRaArcSecPerHr)
{
    int nErr = PLUGIN_OK;
    char szResp[SERIAL_BUFFER_SIZE];

    // nErr = sendCommand("!RGor;", szResp, SERIAL_BUFFER_SIZE);
    if(nErr)
        return nErr;
    dTrackRaArcSecPerHr = atof(szResp);

    return nErr;
}

int SiTech::getCustomTRateOffsetDec(double &dTrackDecArcSecPerHr)
{
    int nErr = PLUGIN_OK;
    char szResp[SERIAL_BUFFER_SIZE];

    // nErr = sendCommand("!RGod;", szResp, SERIAL_BUFFER_SIZE);
    if(nErr)
        return nErr;
    dTrackDecArcSecPerHr = atof(szResp);

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

    dHoursEast = m_pTsx->hourAngle(dEast);
    dHoursWest = m_pTsx->hourAngle(dWest);

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

    if(timer.GetElapsedSeconds()<2) {
        // we're checking for comletion to quickly, assume it's moving for now
        return nErr;
    }

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
    char szResp[SERIAL_BUFFER_SIZE];

    // nErr = sendCommand("!XXxx;", szResp, SERIAL_BUFFER_SIZE);
    return nErr;
}

#pragma mark - time and site methods


#pragma mark - Special commands & functions



int SiTech::countsPerSecToSpeedValue(int cps)
{
    return int(round(cps * 33.55657962109575));
}

int SiTech::speedValueToCountsPerSec(int speed)
{
    return int(round(speed * 0.0298004150390625));
}


int SiTech::degsPerSec2MotorSpeed(double dDegsPerSec, int nTicksPerRev)
{
    return int(round(nTicksPerRev * dDegsPerSec * 0.09321272116971));
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

    // nErr = sendCommand(std::string("XXV\r"), szResp, SERIAL_BUFFER_SIZE);
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

    // nErr = sendCommand(std::string("XXU\r"), szResp, SERIAL_BUFFER_SIZE);
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

int SiTech::degToSteps(double dDegs,  int nTicksPerRev)
{
    return int((double(nTicksPerRev) * dDegs)/ 360);
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

