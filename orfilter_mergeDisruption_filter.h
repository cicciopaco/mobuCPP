#ifndef __ORFILTER_MERGEDISRUPTION_FILTER_H__
#define __ORFILTER_MERGEDISRUPTION_FILTER_H__


/**	\file	ORFilter_MergeDisrutpion_filter.h
*	Declaration of a simple filter class.
*	Simple filter class declaration (FBSimpleFilter).
*/

#include <vector>
#include <iostream>
#include <algorithm>
#include <cmath>

//--- SDK include
#include <fbsdk/fbsdk.h>

//! A simple filter class.
class ORFilter_MergeDisrutpion : public FBFilter
{
	//--- FiLMBOX declaration
	FBFilterDeclare( ORFilter_MergeDisrutpion, FBFilter );

public:
	//--- FiLMBOX Creation/Destruction
	virtual bool FBCreate();
	virtual void FBDestroy();

	//--- Filter Application
	virtual bool Apply( FBAnimationNode* pNode, bool pRecursive = true) override;
	virtual bool Apply( FBFCurve* pCurve ) override;

	//--- Filter Reset
	virtual void Reset();

public:
	//--- Filter error messages
	enum ORFilterError
	{
		eLeftUnchanged,
		eErrorCount		// Last element in enumeration to give count.
	};

private:
	FBPropertyDouble	mDeviationThreshold;
	FBPropertyInt		mWindowWidth;
	FBTimeMode			mTimeMode;

	std::vector<unsigned short>::iterator itStart;

	double calculateSlope(const FBFCurveKey& keyA, const FBFCurveKey& keyB) { return (keyB.Value - keyA.Value) / (getKeyFrame(keyB) - getKeyFrame(keyA)); }
	double getDeviationDifference(FBFCurve* curve, const unsigned short keyId) { return (calculateSlope(curve->Keys[keyId-1], curve->Keys[keyId])) - (calculateSlope(curve->Keys[keyId], curve->Keys[keyId+1])); }

	double getKeyValue(const FBFCurveKey& key) { return key.Value; }
	double getKeyFrame(const FBFCurveKey& key);

	void naiveOffset(FBFCurve* curve, const unsigned short sStartCorrection, const unsigned short sEndCorrection);
	void naiveInterpolation(FBFCurve* curve, const unsigned short sStartCorrection, const unsigned short sEndCorrection);
	
	void naiveInterpolationIT(std::vector<FBFCurveKey> &keySelected, std::vector<FBFCurveKey>::iterator& itStart, std::vector<FBFCurveKey>::iterator& itStop);

};

#endif	/* __ORFILTER_MERGEDIRUPTION_FILTER_H__ */
