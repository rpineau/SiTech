#include "x2mount.h"

X2Mount::X2Mount(const char* pszDriverSelection,
				 const int& nInstanceIndex,
				 SerXInterface					* pSerX,
				 TheSkyXFacadeForDriversInterface	* pTheSkyX,
				 SleeperInterface					* pSleeper,
				 BasicIniUtilInterface			* pIniUtil,
				 LoggerInterface					* pLogger,
				 MutexInterface					* pIOMutex,
				 TickCountInterface				* pTickCount)
{

	m_nPrivateMulitInstanceIndex	= nInstanceIndex;
	m_pSerX							= pSerX;
	m_pTheSkyXForMounts				= pTheSkyX;
	m_pSleeper						= pSleeper;
	m_pIniUtil						= pIniUtil;
	m_pLogger						= pLogger;
	m_pIOMutex						= pIOMutex;
	m_pTickCount					= pTickCount;
	
	
	m_bSynced = false;
	m_bParked = false;
    m_bLinked = false;

    mSiTech.setSerxPointer(m_pSerX);
    mSiTech.setTSX(m_pTheSkyXForMounts);
    mSiTech.setSleeper(m_pSleeper);
    mSiTech.setLogger(m_pLogger);

    m_CurrentRateIndex = 0;

	// Read the current stored values for the settings
	if (m_pIniUtil)
	{
	}

}

X2Mount::~X2Mount()
{
	// Write the stored values

    if(m_bLinked)
        mSiTech.Disconnect();
    
    if (m_pSerX)
		delete m_pSerX;
	if (m_pTheSkyXForMounts)
		delete m_pTheSkyXForMounts;
	if (m_pSleeper)
		delete m_pSleeper;
	if (m_pIniUtil)
		delete m_pIniUtil;
	if (m_pLogger)
		delete m_pLogger;
	if (m_pIOMutex)
		delete m_pIOMutex;
	if (m_pTickCount)
		delete m_pTickCount;
	
}

int X2Mount::queryAbstraction(const char* pszName, void** ppVal)
{
	*ppVal = NULL;
	
	if (!strcmp(pszName, SyncMountInterface_Name))
	    *ppVal = dynamic_cast<SyncMountInterface*>(this);
	if (!strcmp(pszName, SlewToInterface_Name))
		*ppVal = dynamic_cast<SlewToInterface*>(this);
	else if (!strcmp(pszName, AsymmetricalEquatorialInterface_Name))
		*ppVal = dynamic_cast<AsymmetricalEquatorialInterface*>(this);
	else if (!strcmp(pszName, OpenLoopMoveInterface_Name))
		*ppVal = dynamic_cast<OpenLoopMoveInterface*>(this);
	else if (!strcmp(pszName, NeedsRefractionInterface_Name))
		*ppVal = dynamic_cast<NeedsRefractionInterface*>(this);
	else if (!strcmp(pszName, ModalSettingsDialogInterface_Name))
		*ppVal = dynamic_cast<ModalSettingsDialogInterface*>(this);
	else if (!strcmp(pszName, X2GUIEventInterface_Name))
		*ppVal = dynamic_cast<X2GUIEventInterface*>(this);
	else if (!strcmp(pszName, TrackingRatesInterface_Name))
		*ppVal = dynamic_cast<TrackingRatesInterface*>(this);
	else if (!strcmp(pszName, ParkInterface_Name))
		*ppVal = dynamic_cast<ParkInterface*>(this);
	else if (!strcmp(pszName, UnparkInterface_Name))
		*ppVal = dynamic_cast<UnparkInterface*>(this);
	else if (!strcmp(pszName, LoggerInterface_Name))
        *ppVal = GetLogger();
    else if (!strcmp(pszName, SerialPortParams2Interface_Name))
        *ppVal = dynamic_cast<SerialPortParams2Interface*>(this);
    else if (!strcmp(pszName, DriverSlewsToParkPositionInterface_Name))
        *ppVal = dynamic_cast<DriverSlewsToParkPositionInterface*>(this);
	return SB_OK;
}

#pragma mark - OpenLoopMoveInterface

int X2Mount::startOpenLoopMove(const MountDriverInterface::MoveDir& Dir, const int& nRateIndex)
{
    int nErr = SB_OK;
    if(!m_bLinked)
        return ERR_NOLINK;

    X2MutexLocker ml(GetMutex());

	m_CurrentRateIndex = nRateIndex;

    nErr = mSiTech.startOpenSlew(Dir, nRateIndex);
    if(nErr) {
        return ERR_CMDFAILED;
    }
    return SB_OK;
}

int X2Mount::endOpenLoopMove(void)
{
	int nErr = SB_OK;
    if(!m_bLinked)
        return ERR_NOLINK;

    X2MutexLocker ml(GetMutex());

    nErr = mSiTech.stopOpenLoopMove();
    if(nErr) {
        return ERR_CMDFAILED;
    }
    return nErr;
}

int X2Mount::rateCountOpenLoopMove(void) const
{
    X2Mount* pMe = (X2Mount*)this;

    X2MutexLocker ml(pMe->GetMutex());
	return pMe->mSiTech.getNbSlewRates();
}

int X2Mount::rateNameFromIndexOpenLoopMove(const int& nZeroBasedIndex, char* pszOut, const int& nOutMaxSize)
{
    int nErr = SB_OK;
    nErr = mSiTech.getRateName(nZeroBasedIndex, pszOut, nOutMaxSize);
    if(nErr) {
        return ERR_CMDFAILED;
    }
    return nErr;
}

int X2Mount::rateIndexOpenLoopMove(void)
{
	return m_CurrentRateIndex;
}

#pragma mark - UI binding

int X2Mount::execModalSettingsDialog(void)
{
	int nErr = SB_OK;
	X2ModalUIUtil uiutil(this, m_pTheSkyXForMounts);
	X2GUIInterface*					ui = uiutil.X2UI();
	X2GUIExchangeInterface*			dx = NULL;//Comes after ui is loaded
	bool bPressedOK = false;
    char szTmpBuf[SERIAL_BUFFER_SIZE];
    char szTime[SERIAL_BUFFER_SIZE];
    char szDate[SERIAL_BUFFER_SIZE];

    if (NULL == ui) return ERR_POINTER;
	
	if ((nErr = ui->loadUserInterface("SiTech.ui", deviceType(), m_nPrivateMulitInstanceIndex)))
		return nErr;
	
	if (NULL == (dx = uiutil.X2DX())) {
		return ERR_POINTER;
	}

    memset(szTmpBuf,0,SERIAL_BUFFER_SIZE);
    memset(szTime,0,SERIAL_BUFFER_SIZE);
    memset(szDate,0,SERIAL_BUFFER_SIZE);

    X2MutexLocker ml(GetMutex());

 	//Display the user interface
	if ((nErr = ui->exec(bPressedOK)))
		return nErr;
	
	//Retreive values from the user interface
	if (bPressedOK) {
	}
	return nErr;
}

void X2Mount::uiEvent(X2GUIExchangeInterface* uiex, const char* pszEvent)
{
    int nErr = PLUGIN_OK;
    char szTmpBuf[SERIAL_BUFFER_SIZE];
    char szTime[SERIAL_BUFFER_SIZE];
    char szDate[SERIAL_BUFFER_SIZE];

    if(!m_bLinked)
        return ;

    memset(szTmpBuf,0,SERIAL_BUFFER_SIZE);
    memset(szTime,0,SERIAL_BUFFER_SIZE);
    memset(szDate,0,SERIAL_BUFFER_SIZE);

	if (!strcmp(pszEvent, "on_timer")) {
	}
	return;
}

#pragma mark - LinkInterface
int X2Mount::establishLink(void)
{
    int nErr;
    char szPort[DRIVER_MAX_STRING];

	X2MutexLocker ml(GetMutex());
	// get serial port device name
    portNameOnToCharPtr(szPort,DRIVER_MAX_STRING);

	nErr =  mSiTech.Connect(szPort);
    if(nErr) {
        m_bLinked = false;
    }
    else {
        m_bLinked = true;
    }
    return nErr;
}

int X2Mount::terminateLink(void)
{
    int nErr = SB_OK;

	X2MutexLocker ml(GetMutex());

    nErr = mSiTech.Disconnect();
    m_bLinked = false;

    return nErr;
}

bool X2Mount::isLinked(void) const
{

	return mSiTech.isConnected();;
}

bool X2Mount::isEstablishLinkAbortable(void) const
{
    return false;
}

#pragma mark - AbstractDriverInfo

void	X2Mount::driverInfoDetailedInfo(BasicStringInterface& str) const
{
	str = "SiTech X2 plugin by Rodolphe Pineau";
}

double	X2Mount::driverInfoVersion(void) const
{
	return DRIVER_VERSION;
}

void X2Mount::deviceInfoNameShort(BasicStringInterface& str) const
{
    str = "SiTech Servo controller";

}

void X2Mount::deviceInfoNameLong(BasicStringInterface& str) const
{
	str = "SiTech Servo controller";
	
}
void X2Mount::deviceInfoDetailedDescription(BasicStringInterface& str) const
{
	str = "SiTech Servo controller";
	
}
void X2Mount::deviceInfoFirmwareVersion(BasicStringInterface& str)
{
    if(m_bLinked) {
        char cFirmware[SERIAL_BUFFER_SIZE];
        X2MutexLocker ml(GetMutex());
        mSiTech.getFirmwareVersion(cFirmware, SERIAL_BUFFER_SIZE);
        str = cFirmware;
    }
    else
        str = "Not connected";
}
void X2Mount::deviceInfoModel(BasicStringInterface& str)
{
    str = "SiTech Servo controller";
}

#pragma mark - Common Mount specifics
int X2Mount::raDec(double& ra, double& dec, const bool& bCached)
{
	int nErr = 0;

    if(!m_bLinked)
        return ERR_NOLINK;

    X2MutexLocker ml(GetMutex());

	// Get the RA and DEC from the mount
	nErr = mSiTech.getRaAndDec(ra, dec);
    if(nErr)
        nErr = ERR_CMDFAILED;

	return nErr;
}

int X2Mount::abort()
{
    int nErr = SB_OK;
    if(!m_bLinked)
        return ERR_NOLINK;

    X2MutexLocker ml(GetMutex());

    nErr = mSiTech.Abort();
    if(nErr)
        nErr = ERR_CMDFAILED;

    return nErr;
}

int X2Mount::startSlewTo(const double& dRa, const double& dDec)
{
	int nErr = SB_OK;

    if(!m_bLinked)
        return ERR_NOLINK;

    X2MutexLocker ml(GetMutex());

    nErr = mSiTech.startSlewTo(dRa, dDec);
    if(nErr) {
        return ERR_CMDFAILED;
    }

    return nErr;
}

int X2Mount::isCompleteSlewTo(bool& bComplete) const
{
    int nErr = SB_OK;
    if(!m_bLinked)
        return ERR_NOLINK;

    X2Mount* pMe = (X2Mount*)this;
    X2MutexLocker ml(pMe->GetMutex());

    nErr = pMe->mSiTech.isSlewToComplete(bComplete);
	return nErr;
}

int X2Mount::endSlewTo(void)
{
    return SB_OK;
}


int X2Mount::syncMount(const double& ra, const double& dec)
{
	int nErr = SB_OK;

    if(!m_bLinked)
        return ERR_NOLINK;

    X2MutexLocker ml(GetMutex());

    nErr = mSiTech.syncTo(ra, dec);
    return nErr;
}

bool X2Mount::isSynced(void)
{
    if(!m_bLinked)
        return false;

    X2MutexLocker ml(GetMutex());
    m_bSynced = false;

    return m_bSynced;
}

#pragma mark - TrackingRatesInterface
int X2Mount::setTrackingRates(const bool& bTrackingOn, const bool& bIgnoreRates, const double& dRaRateArcSecPerSec, const double& dDecRateArcSecPerSec)
{
    int nErr = SB_OK;
    double dTrackRaArcSecPerHr;
    double dTrackDecArcSecPerHr;
    if(!m_bLinked)
        return ERR_NOLINK;

    X2MutexLocker ml(GetMutex());

    dTrackRaArcSecPerHr = dRaRateArcSecPerSec * 3600;
    dTrackDecArcSecPerHr = dDecRateArcSecPerSec * 3600;

    nErr = mSiTech.setTrackingRates(bTrackingOn, bIgnoreRates, dTrackRaArcSecPerHr, dTrackDecArcSecPerHr);
    return nErr;
	
}

int X2Mount::trackingRates(bool& bTrackingOn, double& dRaRateArcSecPerSec, double& dDecRateArcSecPerSec)
{
    int nErr = SB_OK;
    double dTrackRaArcSecPerHr;
    double dTrackDecArcSecPerHr;

    if(!m_bLinked)
        return ERR_NOLINK;

    X2MutexLocker ml(GetMutex());

    nErr = mSiTech.getTrackRates(bTrackingOn, dTrackRaArcSecPerHr, dTrackDecArcSecPerHr);
    if(nErr) {
        return ERR_CMDFAILED;
    }
    dRaRateArcSecPerSec = dTrackRaArcSecPerHr / 3600;
    dDecRateArcSecPerSec = dTrackDecArcSecPerHr / 3600;

	return nErr;
}

int X2Mount::siderealTrackingOn()
{
    int nErr = SB_OK;
    if(!m_bLinked)
        return ERR_NOLINK;

    X2MutexLocker ml(GetMutex());

    nErr = setTrackingRates( true, true, 0.0, 0.0);
    return nErr;
}

int X2Mount::trackingOff()
{
    int nErr = SB_OK;
    if(!m_bLinked)
        return ERR_NOLINK;

    X2MutexLocker ml(GetMutex());

    nErr = setTrackingRates( false, true, 0.0, 0.0);
    return nErr;
}

#pragma mark - NeedsRefractionInterface
bool X2Mount::needsRefactionAdjustments(void)
{
    bool bEnabled;
    int nErr;

    if(!m_bLinked)
        return false;

    X2MutexLocker ml(GetMutex());

    // check if SiTech refraction adjustment is on.
    nErr = mSiTech.getRefractionCorrEnabled(bEnabled);
    return !bEnabled; // if enabled in SiTech, don't ask TSX to do it.
}

#pragma mark - Parking Interface
bool X2Mount::isParked(void)
{
    int nErr;
    bool bTrackingOn;
    bool bIsPArked;
    double dTrackRaArcSecPerHr, dTrackDecArcSecPerHr;

    if(!m_bLinked)
        return false;

    X2MutexLocker ml(GetMutex());

    nErr = mSiTech.getAtPark(bIsPArked);
    if(nErr) {
        return false;
    }
    if(!bIsPArked) // not parked
        return false;

    // get tracking state.
    nErr = mSiTech.getTrackRates(bTrackingOn, dTrackRaArcSecPerHr, dTrackDecArcSecPerHr);
    if(nErr) {
        return false;
    }
    // if AtPark and tracking is off, then we're parked, if not then we're unparked.
    if(bIsPArked && !bTrackingOn)
        m_bParked = true;
    else
        m_bParked = false;
    return m_bParked;
}

int X2Mount::startPark(const double& dAz, const double& dAlt)
{
	double dRa, dDec;
	int nErr = SB_OK;

    if(!m_bLinked)
        return ERR_NOLINK;
	
	X2MutexLocker ml(GetMutex());

	nErr = m_pTheSkyXForMounts->HzToEq(dAz, dAlt, dRa, dDec);
    if (nErr) {
        return nErr;
    }

    // goto park
    nErr = mSiTech.gotoPark(dRa, dDec);
	return nErr;
}


int X2Mount::isCompletePark(bool& bComplete) const
{
    int nErr = SB_OK;

    if(!m_bLinked)
        return ERR_NOLINK;

    X2Mount* pMe = (X2Mount*)this;

    X2MutexLocker ml(pMe ->GetMutex());

    nErr = pMe->mSiTech.getAtPark(bComplete);
	return nErr;
}

int X2Mount::endPark(void)
{
    return SB_OK;
}

int X2Mount::startUnpark(void)
{
    int nErr = SB_OK;

    if(!m_bLinked)
        return ERR_NOLINK;

    X2MutexLocker ml(GetMutex());

    nErr = mSiTech.unPark();
    if(nErr) {
        nErr = ERR_CMDFAILED;
    }
    m_bParked = false;
    return nErr;
}

/*!Called to monitor the unpark process.
 \param bComplete Set to true if the unpark is complete, otherwise set to false.
*/
int X2Mount::isCompleteUnpark(bool& bComplete) const
{
    int nErr;
    bool bIsParked;
    bool bTrackingOn;
    double dTrackRaArcSecPerHr, dTrackDecArcSecPerHr;

    if(!m_bLinked)
        return ERR_NOLINK;

    X2Mount* pMe = (X2Mount*)this;

    X2MutexLocker ml(pMe ->GetMutex());

    bComplete = false;

    nErr = pMe->mSiTech.getAtPark(bIsParked);
    if(nErr) {
        nErr = ERR_CMDFAILED;
    }
    if(!bIsParked) { // no longer parked.
        bComplete = true;
        pMe->m_bParked = false;
        return nErr;
    }

    // if we're still at the park position
    // get tracking state. If tracking is off, then we're parked, if not then we're unparked.
    nErr = pMe->mSiTech.getTrackRates(bTrackingOn, dTrackRaArcSecPerHr, dTrackDecArcSecPerHr);
    if(nErr)
        nErr = ERR_CMDFAILED;

    if(bTrackingOn) {
        bComplete = true;
        pMe->m_bParked = false;
    }
    else {
        bComplete = false;
        pMe->m_bParked = true;
    }
	return SB_OK;
}

/*!Called once the unpark is complete.
 This is called once for every corresponding startUnpark() allowing software implementations of unpark.
 */
int		X2Mount::endUnpark(void)
{
	return SB_OK;
}

#pragma mark - AsymmetricalEquatorialInterface

bool X2Mount::knowsBeyondThePole()
{
    return true;
}

int X2Mount::beyondThePole(bool& bYes) {
	// bYes = mSiTech.GetIsBeyondThePole();
	return SB_OK;
}


double X2Mount::flipHourAngle() {
	return 0.0;
}


int X2Mount::gemLimits(double& dHoursEast, double& dHoursWest)
{
    int nErr = SB_OK;
    if(!m_bLinked)
        return ERR_NOLINK;

    X2MutexLocker ml(GetMutex());

    nErr = mSiTech.getLimits(dHoursEast, dHoursWest);

    // temp debugging.
	dHoursEast = 0.0;
	dHoursWest = 0.0;
	return SB_OK;
}


#pragma mark - SerialPortParams2Interface

void X2Mount::portName(BasicStringInterface& str) const
{
    char szPortName[DRIVER_MAX_STRING];

    portNameOnToCharPtr(szPortName, DRIVER_MAX_STRING);

    str = szPortName;

}

void X2Mount::setPortName(const char* pszPort)
{
    if (m_pIniUtil)
        m_pIniUtil->writeString(PARENT_KEY, CHILD_KEY_PORT_NAME, pszPort);

}


void X2Mount::portNameOnToCharPtr(char* pszPort, const unsigned int& nMaxSize) const
{
    if (NULL == pszPort)
        return;

    snprintf(pszPort, nMaxSize,DEF_PORT_NAME);

    if (m_pIniUtil)
        m_pIniUtil->readString(PARENT_KEY, CHILD_KEY_PORT_NAME, pszPort, pszPort, nMaxSize);

}




