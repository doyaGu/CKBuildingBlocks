#ifndef UNLIMITED_PARAMS_H
#define UNLIMITED_PARAMS_H

#include "CKAll.h"

#define PARAM_LOOP				0x0001
#define PARAM_STOPABLE			0x0002	
#define PARAM_BREAK				0x0004
#define PARAM_WARP				0x0008
#define PARAM_PLAY				0x0010
#define PARAM_USERIGHTLEFT		0x0020
#define PARAM_ORIENTATION		0x0040
#define PARAM_TIMEBASE			0x0080
#define PARAM_PRECALCULATED		0x0100
#define PARAM_LOOPN				0x0200
#define PARAM_STAY				0x0400
#define PARAM_WARPSTART			0x0800
#define PARAM_WARPSAME			0x1000
#define PARAM_WARPMASK			0x1808

// Index of parameter for animation
#define PARAM_ANIMATION_INDEX	0
#define PARAM_WARPFLAGS_INDEX	1
#define PARAM_WARPLENGTH_INDEX	2
#define PARAM_STOP_INDEX		3
#define PARAM_TIME_INDEX		4
#define PARAM_FPS_INDEX			5
#define PARAM_TURN_INDEX		6
#define PARAM_ORIENT_INDEX		7
#define PARAM_SPLAYMODE_INDEX	8
#define PARAM_STAYONLAST_INDEX	9
#define PARAM_STARTFRAME_INDEX	10
#define PARAM_LOOPCOUNT_INDEX	11

#define PARAM_ANIMATION_MASK	0x00000001L
#define PARAM_WARPFLAGS_MASK	0x00000002L
#define PARAM_WARPLENGTH_MASK	0x00000004L
#define PARAM_STOP_MASK			0x00000008L
#define PARAM_TIME_MASK			0x00000010L
#define PARAM_FPS_MASK			0x00000020L
#define PARAM_TURN_MASK			0x00000040L
#define PARAM_ORIENT_MASK		0x00000080L
#define PARAM_SPLAYMODE_MASK	0x00000100L
#define PARAM_STAYONLAST_MASK	0x00000200L
#define PARAM_STARTFRAME_MASK	0x00000400L
#define PARAM_LOOPCOUNT_MASK	0x00000800L

#define DESC_MAX_LENGTH 40

typedef struct UnlimitedParam {
				int IndexAnimation;
				union 
				{
					float StartFrame;
					int Priority;
				};
				char  Desc[DESC_MAX_LENGTH];
				CKAnimation* AnimationPtr;
				CKWORD  Align;
				CKWORD  ParameterMask;	 
				char  ParameterIndexes[16];	// Index of linked input parameter
				char Message[64];
				int  Msg;
				float fps;
				float warplength;
				unsigned short flags;
				unsigned short LoopCount;
				} UnlimitedParam;

#endif