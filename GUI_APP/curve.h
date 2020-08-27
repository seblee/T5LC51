#ifndef __CURVE_H_
#define __CURVE_H_
/**
 ****************************************************************************
 * @Warning Without permission from the author,Not for commercial use
 * @File
 * @Author  xiaowine@cee0.com
 * @date
 * @version V1.0
 *************************************************
 * @brief   -->>
 ****************************************************************************
 * @attention
 * Powered By Xiaowine
 * <h2><center>&copy;  Copyright(C) cee0.com 2015-2019</center></h2>
 * All rights reserved
 *
 **/
#include "sys.h"

#define TimerPeriod 10  // T*100ms

#define CURVE_FLASH_ADDR 0
#define CURVE_VP_ADDR 0x1000
#define CURVE_LEN 0x1000

// curve0---
#define Curve0Point 0x0300
#define Curve0Len 0x0301
#define Curve0Start 0x1000
#define Curve0End 0x17FE
// #define Curve0Addr 0x5004  // 1个
#define Curve0Data 0x5005  // 1个
// curve1---
#define Curve1Point 0x0302
#define Curve1Len 0x0303
#define Curve1Start 0x1800
#define Curve1End 0x1FFE
// #define Curve1Addr 0x5006  // 1个
#define Curve1Data 0x5007  // 1个

#define CurveMAX 0x07FE
#define Curveffset 0x1000
#define Curvetemp 0x5000  // 4个
#define Curveflag 0x5002  // 1个

// Curvedrag---
#define CurveManual 0x5008
#define CurveManualBak 0x5009
#define CurveDrager 0x500A  // 10个
#define DragerX 0x5011
#define DragerLeft 143
#define DragerRight 555

void curveInit(void);
void curveProcess(void);
void dragCuave(void);
#endif