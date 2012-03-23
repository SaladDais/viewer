/** 
 * @file llpathfindingnavmeshstatus.h
 * @author William Todd Stinson
 * @brief A class for representing the navmesh status of a pathfinding region.
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc. 
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * 
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */

#ifndef LL_LLPATHFINDINGNAVMESHSTATUS_H
#define LL_LLPATHFINDINGNAVMESHSTATUS_H

#include "lluuid.h"

#include <string>

// XXX stinson 03/12/2012 : This definition is in place to support an older version of the pathfinding simulator that does not have versioned information
#define DEPRECATED_UNVERSIONED_NAVMESH

class LLSD;

class LLPathfindingNavMeshStatus
{
public:
	typedef enum
	{
		kPending,
		kBuilding,
		kComplete,
		kRepending
	} ENavMeshStatus;

	LLPathfindingNavMeshStatus();
	LLPathfindingNavMeshStatus(const LLUUID &pRegionUUID);
	LLPathfindingNavMeshStatus(const LLUUID &pRegionUUID, const LLSD &pContent);
	LLPathfindingNavMeshStatus(const LLSD &pContent);
	LLPathfindingNavMeshStatus(const LLPathfindingNavMeshStatus &pOther);
	virtual ~LLPathfindingNavMeshStatus();

	LLPathfindingNavMeshStatus &operator =(const LLPathfindingNavMeshStatus &pOther);

#ifdef DEPRECATED_UNVERSIONED_NAVMESH
	void incrementNavMeshVersionXXX() {++mVersion;};
#endif // DEPRECATED_UNVERSIONED_NAVMESH

	bool           isValid() const        {return mIsValid;};
	const LLUUID   &getRegionUUID() const {return mRegionUUID;};
	U32            getVersion() const     {return mVersion;};
	ENavMeshStatus getStatus() const      {return mStatus;};

protected:

private:
	void           parseStatus(const LLSD &pContent);

	bool           mIsValid;
	LLUUID         mRegionUUID;
	U32            mVersion;
	ENavMeshStatus mStatus;

	static const std::string sStatusPending;
	static const std::string sStatusBuilding;
	static const std::string sStatusComplete;
	static const std::string sStatusRepending;
};

#endif // LL_LLPATHFINDINGNAVMESHSTATUS_H