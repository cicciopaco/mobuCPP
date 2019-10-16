/***************************************************************************************

***************************************************************************************/

/**	\file	ORFilter_MergeDisrutpion_filter.cxx
*	Filter template
*/

//--- Class declarations
#include "orfilter_mergeDisruption_filter.h"

#define FBTIME_POS_INFINITE FBTime( kLongLong( K_LONGLONG( 0x7fffffffffffffff )))
#define FBTIME_NEG_INFINITE FBTime( kLongLong( K_LONGLONG( 0x7fffffffffffffff )))

//--- Filter Error table
static const char* ORFilterErrorTable [ORFilter_MergeDisrutpion::eErrorCount] =
{
	"Left Unchanged",
};

//--- Registration defines
#define	ORFILTERTEMPLATE__CLASS			ORFilter_MergeDisrutpion
#define ORFILTERTEMPLATE__NAME			"OR - Merge Disruption"
#define ORFILTERTEMPLATE__LABEL			"OR - Merge Disruption"
#define ORFILTERTEMPLATE__DESC			"Detects the diruption on the curve/selected frame and corrects the interpolation"
#define ORFILTERTEMPLATE__TYPE			kFBFilterNumber | kFBFilterVector
#define ORFILTERTEMPLATE__ERRTABLE		ORFilterErrorTable
#define ORFILTERTEMPLATE__ERRCOUNT		ORFilter_MergeDisrutpion::eErrorCount
#define ERROR_POS 0xFFFF

//--- FiLMBOX implementation and registration
FBFilterImplementation	(	ORFILTERTEMPLATE__CLASS			);
FBRegisterFilter		(	ORFILTERTEMPLATE__CLASS,
							ORFILTERTEMPLATE__NAME,
							ORFILTERTEMPLATE__DESC,
							ORFILTERTEMPLATE__TYPE,
							ORFILTERTEMPLATE__ERRTABLE,
							ORFILTERTEMPLATE__ERRCOUNT,
							FB_DEFAULT_SDK_ICON				);	// Icon filename (default=Open Reality icon)


//FBRegisterFilter(ORFILTERTEMPLATE__CLASS, ORFILTERTEMPLATE__LABEL, ORFILTERTEMPLATE__DESC, ORFILTERTEMPLATE__TYPE, ORFILTERTEMPLATE__ERRTABLE, ORFILTERTEMPLATE__ERRCOUNT, FB_DEFAULT_SDK_ICON)



/************************************************
 *	FiLMBOX Creation function.
 ************************************************/
bool ORFilter_MergeDisrutpion::FBCreate()
{
	/*
	*	1. Add the properties to the filter
	*	2, Reset the filter
	*/
	FBPropertyPublish( this, mDeviationThreshold,	"Deviation Threshold", NULL, NULL	);
	FBPropertyPublish( this, mWindowWidth, "Correction Window", NULL, NULL);

	Reset		();
	return true;
}


/************************************************
 *	FiLMBOX Destruction function.
 ************************************************/
void ORFilter_MergeDisrutpion::FBDestroy()
{
	/*
	*	Free any user memory.
	*/
}


/************************************************
 *	Filtering functionality (animation node).
 ************************************************/
bool ORFilter_MergeDisrutpion::Apply( FBAnimationNode* pNode, bool pRecursive )
{
	/*
	*	Perform AnimationNode level filter operations.
	*/
	return FBFilter::Apply( pNode, pRecursive );
}

/************************************************
 *	Filtering functionality (FCurve).
 ************************************************/


bool ORFilter_MergeDisrutpion::Apply( FBFCurve* pCurve )
{
	/*
	*	Perform FCurve level operations.
	*/

	// NOTICE: it seems that every ORSDK object must be iniitalized at the beginning of the function... WHY?!
	int framesCount(pCurve->Keys.GetCount());
	int i=1;

	while (i < framesCount-1) // WATCH OUT: be aware that we could be off 1
	{
		// skip to next key if the current one is not selected
		if (!pCurve->Keys[i].Selected) 
		{
			i++;
			continue;
		}
		
		// skip to next frame if the current one has a slope lower than the threshold
		if (abs(getDeviationDifference(pCurve, i)) < mDeviationThreshold)
		{
			i++;
			continue;
		}

		int startID = i + 1;
		int stopID = startID;

		for (stopID++; stopID < startID + mWindowWidth; stopID++)
		{
			if (stopID >= framesCount) break;
			if (!pCurve->Keys[stopID].Selected) break;
			if (abs(getDeviationDifference(pCurve, stopID)) >= mDeviationThreshold) break;
		}
		//FBTrace("\nperforming correction betwen %i - %i\n", startID, stopID);

		// in case the tail of the curve is selected 
		if (stopID == framesCount)
		{
			naiveOffset(pCurve, startID, stopID);
			continue;
		}

		naiveInterpolation(pCurve, startID, stopID);
		i = stopID + 1;
	}
	return FBFilter::Apply( pCurve );
}


double ORFilter_MergeDisrutpion::getKeyFrame(const FBFCurveKey &key)
 {
	 FBTime keyFBTime(key.Time);
	 return keyFBTime.GetFrame(mTimeMode);
 }



void ORFilter_MergeDisrutpion::naiveOffset(FBFCurve* curve, const unsigned short sStartCorrection, const unsigned short sEndCorrection)
 {
	 /*
	 It works on keyframe-id-base
	 */

	double fGapStartValue ;
	double fPreGapValue ;

	double currentFrameTime;

	double lGapStartTime = getKeyFrame(curve->Keys[sStartCorrection]);
	double lPreGapTime = getKeyFrame(curve->Keys[sStartCorrection-1]);
	fGapStartValue = getKeyValue(curve->Keys[sStartCorrection]);
	fPreGapValue = getKeyValue(curve->Keys[sStartCorrection-1]);
			
	double dPreSlope = calculateSlope(curve->Keys[sStartCorrection-2], curve->Keys[sStartCorrection-1]);
	double dPreIntercept = fPreGapValue - (dPreSlope * lPreGapTime);

	double dGapStartExpectedValue = (lGapStartTime * dPreSlope) + dPreIntercept;
	double dStartDeltaValue = fGapStartValue - dGapStartExpectedValue;

	unsigned short id(sStartCorrection);

	while (id <= sEndCorrection)
	{
		currentFrameTime = getKeyFrame(curve->Keys[id]);
		curve->Keys[id].Value = curve->Keys[id].Value - dStartDeltaValue;
		id++;
	}
 }


 void ORFilter_MergeDisrutpion::naiveInterpolation(FBFCurve* curve, const unsigned short sStartCorrection, const unsigned short sEndCorrection)
 {

	double fGapStartValue ;
	double fGapEndValue ;
	double fPreGapValue ;
	double fPostGapValue ;
	
	double currentFrameTime;

	double lGapStartTime = getKeyFrame(curve->Keys[sStartCorrection]);
	double lGapEndTime = getKeyFrame(curve->Keys[sEndCorrection]);
	double lPreGapTime = getKeyFrame(curve->Keys[sStartCorrection-1]);
	double lPostGapTime = getKeyFrame(curve->Keys[sEndCorrection+1]);

	fGapStartValue = getKeyValue(curve->Keys[sStartCorrection]);
	fGapEndValue = getKeyValue(curve->Keys[sEndCorrection]);
	fPreGapValue = getKeyValue(curve->Keys[sStartCorrection-1]);
	fPostGapValue = getKeyValue(curve->Keys[sEndCorrection+1]);
	
	//FBTrace("\tfGapStartValue= %f\n", fGapStartValue);

	double dPreSlope = calculateSlope(curve->Keys[sStartCorrection-2], curve->Keys[sStartCorrection-1]);
	double dPostSlope = calculateSlope(curve->Keys[sEndCorrection+1], curve->Keys[sEndCorrection+2]);
	double lGapDeltaTime = lGapEndTime - lGapStartTime;
	double dPreIntercept = fPreGapValue - (dPreSlope * lPreGapTime);
	double dPostIntercept = fPostGapValue - (dPostSlope * lPostGapTime);
	double dGapStartExpectedValue = (lGapStartTime * dPreSlope) + dPreIntercept;
	double dGapEndExpectedValue = (lGapEndTime * dPostSlope) + dPostIntercept;
	double dStartDeltaValue = fGapStartValue - dGapStartExpectedValue;
	double dEndDeltaValue = fGapEndValue - dGapEndExpectedValue;
	unsigned short id(sStartCorrection);

	while (id<= sEndCorrection)
	{
		currentFrameTime = getKeyFrame(curve->Keys[id]);

		double dPreFactor = ((currentFrameTime - lGapStartTime) - lGapDeltaTime) / -lGapDeltaTime;
		double dPostFactor = (currentFrameTime - lGapStartTime) / lGapDeltaTime;
		curve->Keys[id].Value = curve->Keys[id].Value - ( (dStartDeltaValue * dPreFactor) + (dEndDeltaValue * dPostFactor) );
		id++;
	}

 }


void ORFilter_MergeDisrutpion::naiveInterpolationIT(std::vector<FBFCurveKey> &keySelected, std::vector<FBFCurveKey>::iterator& itStart, std::vector<FBFCurveKey>::iterator& itStop)
{
	
	// add a different correction strategy for offsetting the queue of the animation


	//WHATCH OUT: There is the risk to go out of bound

	long long lGapStartTime = getKeyFrame(*itStart);
	long long lGapEndTime = getKeyFrame(*itStop);

	long long lPreGapTime = getKeyFrame(*(itStart-1));
	long long lPostGapTime = getKeyFrame(*(itStop+1));

	FBPropertyFloat fGapStartValue = (*itStart).Value;
	FBPropertyFloat fGapEndValue = (*itStop).Value;

	FBPropertyFloat fPreGapValue = (*(itStart-1)).Value;
	FBPropertyFloat fPostGapValue = (*(itStop+1)).Value;

	double dPreSlope = calculateSlope(*itStart, *itStop);
	double dPostSlope = calculateSlope(*itStart, *itStop);

	long long lGapDeltaTime = lGapStartTime - lGapEndTime;

	double dPreIntercept = fPreGapValue - (dPreSlope * lPreGapTime);
	double dPostIntercept = fPostGapValue - (dPostSlope * lPostGapTime);

	double dGapStartExpectedValue = (lGapStartTime * dPreSlope) + dPreIntercept;
	double dGapEndExpectedValue = (lGapEndTime * dPostSlope) + dPostIntercept;

	double dStartDeltaValue = fGapStartValue - dGapStartExpectedValue;
	double dEndDeltaValue = fGapEndValue - dGapEndExpectedValue;


	for (std::vector<FBFCurveKey>::iterator it = itStart; it !=itStop; it++)
	{
		double dPreFactor = ((getKeyFrame(*it) - lGapStartTime) - lGapDeltaTime) / (-lGapDeltaTime);
		double dPostFactor = (getKeyFrame(*it) - lGapStartTime) / lGapDeltaTime;

		(*it).Value = (*it).Value - ( (dStartDeltaValue * dPreFactor) + (dEndDeltaValue * dPostFactor) );
	}
}


/************************************************
 *	Reset the filter properties.
 ************************************************/
void ORFilter_MergeDisrutpion::Reset()
{
	/*
	*	Reset property values.
	*/
	mDeviationThreshold		= 4.0;
	mWindowWidth			= 8;

	FBPlayerControl playerControl;
	mTimeMode = playerControl.GetTransportFps();


}
