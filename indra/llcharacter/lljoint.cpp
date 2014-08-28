/** 
 * @file lljoint.cpp
 * @brief Implementation of LLJoint class.
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

//-----------------------------------------------------------------------------
// Header Files
//-----------------------------------------------------------------------------
#include "linden_common.h"

#include "lljoint.h"

#include "llmath.h"

S32 LLJoint::sNumUpdates = 0;
S32 LLJoint::sNumTouches = 0;


//-----------------------------------------------------------------------------
// LLJoint::AttachmentOverrideRecord::AttachmentOverrideRecord()
//-----------------------------------------------------------------------------
LLJoint::AttachmentOverrideRecord::AttachmentOverrideRecord()
{
}

template <class T> 
bool attachment_map_iter_compare_name(const T& a, const T& b)
{
	return a.second.name < b.second.name;
}

//-----------------------------------------------------------------------------
// LLJoint()
// Class Constructor
//-----------------------------------------------------------------------------


void LLJoint::init()
{
	mName = "unnamed";
	mParent = NULL;
	mXform.setScaleChildOffset(TRUE);
	mXform.setScale(LLVector3(1.0f, 1.0f, 1.0f));
	mDirtyFlags = MATRIX_DIRTY | ROTATION_DIRTY | POSITION_DIRTY;
	mUpdateXform = TRUE;
}

LLJoint::LLJoint() :
	mJointNum(-1)
{
	init();
	touch();
}

LLJoint::LLJoint(S32 joint_num) :
	mJointNum(joint_num)
{
	init();
	touch();
}


//-----------------------------------------------------------------------------
// LLJoint()
// Class Constructor
//-----------------------------------------------------------------------------
LLJoint::LLJoint(const std::string &name, LLJoint *parent) :
	mJointNum(0)
{
	init();
	mUpdateXform = FALSE;

	setName(name);
	if (parent)
	{
		parent->addChild( this );
	}
	touch();
}

//-----------------------------------------------------------------------------
// ~LLJoint()
// Class Destructor
//-----------------------------------------------------------------------------
LLJoint::~LLJoint()
{
	if (mParent)
	{
		mParent->removeChild( this );
	}
	removeAllChildren();
}


//-----------------------------------------------------------------------------
// setup()
//-----------------------------------------------------------------------------
void LLJoint::setup(const std::string &name, LLJoint *parent)
{
	setName(name);
	if (parent)
	{
		parent->addChild( this );
	}
}

//-----------------------------------------------------------------------------
// touch()
// Sets all dirty flags for all children, recursively.
//-----------------------------------------------------------------------------
void LLJoint::touch(U32 flags)
{
	if ((flags | mDirtyFlags) != mDirtyFlags)
	{
		sNumTouches++;
		mDirtyFlags |= flags;
		U32 child_flags = flags;
		if (flags & ROTATION_DIRTY)
		{
			child_flags |= POSITION_DIRTY;
		}

		for (child_list_t::iterator iter = mChildren.begin();
			 iter != mChildren.end(); ++iter)
		{
			LLJoint* joint = *iter;
			joint->touch(child_flags);
		}
	}
}

//-----------------------------------------------------------------------------
// getRoot()
//-----------------------------------------------------------------------------
LLJoint *LLJoint::getRoot()
{
	if ( getParent() == NULL )
	{
		return this;
	}
	return getParent()->getRoot();
}


//-----------------------------------------------------------------------------
// findJoint()
//-----------------------------------------------------------------------------
LLJoint *LLJoint::findJoint( const std::string &name )
{
	if (name == getName())
		return this;

	for (child_list_t::iterator iter = mChildren.begin();
		 iter != mChildren.end(); ++iter)
	{
		LLJoint* joint = *iter;
		LLJoint *found = joint->findJoint(name);
		if (found)
		{
			return found;
		}
	}

	return NULL;	
}


//--------------------------------------------------------------------
// addChild()
//--------------------------------------------------------------------
void LLJoint::addChild(LLJoint* joint)
{
	if (joint->mParent)
		joint->mParent->removeChild(joint);

	mChildren.push_back(joint);
	joint->mXform.setParent(&mXform);
	joint->mParent = this;	
	joint->touch();
}


//--------------------------------------------------------------------
// removeChild()
//--------------------------------------------------------------------
void LLJoint::removeChild(LLJoint* joint)
{
	child_list_t::iterator iter = std::find(mChildren.begin(), mChildren.end(), joint);
	if (iter != mChildren.end())
	{
		mChildren.erase(iter);
	
		joint->mXform.setParent(NULL);
		joint->mParent = NULL;
		joint->touch();
	}
}


//--------------------------------------------------------------------
// removeAllChildren()
//--------------------------------------------------------------------
void LLJoint::removeAllChildren()
{
	for (child_list_t::iterator iter = mChildren.begin();
		 iter != mChildren.end();)
	{
		child_list_t::iterator curiter = iter++;
		LLJoint* joint = *curiter;
		mChildren.erase(curiter);
		joint->mXform.setParent(NULL);
		joint->mParent = NULL;
		joint->touch();
	}
}


//--------------------------------------------------------------------
// getPosition()
//--------------------------------------------------------------------
const LLVector3& LLJoint::getPosition()
{
	return mXform.getPosition();
}


//--------------------------------------------------------------------
// setPosition()
//--------------------------------------------------------------------
void LLJoint::setPosition( const LLVector3& pos )
{
	mXform.setPosition(pos);
	touch(MATRIX_DIRTY | POSITION_DIRTY);
}

//--------------------------------------------------------------------
// addAttachmentPosOverride()
//--------------------------------------------------------------------
void LLJoint::addAttachmentPosOverride( const LLVector3& pos, const std::string& attachment_name )
{
	if (attachment_name.empty())
	{
		return;
	}
	if (m_attachmentOverrides.empty())
	{
		LL_DEBUGS("Avatar") << getName() << " saving m_posBeforeOverrides " << getPosition() << LL_ENDL;
		m_posBeforeOverrides = getPosition();
	}
	AttachmentOverrideRecord rec;
	rec.name = attachment_name;
	rec.pos = pos;
	m_attachmentOverrides[attachment_name] = rec;
	LL_DEBUGS("Avatar") << getName() << " addAttachmentPosOverride for " << attachment_name << " pos " << pos << LL_ENDL;
	updatePos();
}

//--------------------------------------------------------------------
// removeAttachmentPosOverride()
//--------------------------------------------------------------------
void LLJoint::removeAttachmentPosOverride( const std::string& attachment_name )
{
	if (attachment_name.empty())
	{
		return;
	}
	attachment_map_t::iterator it = m_attachmentOverrides.find(attachment_name);
	if (it != m_attachmentOverrides.end())
	{
		LL_DEBUGS("Avatar") << getName() << " removeAttachmentPosOverride for " << attachment_name << LL_ENDL;
		m_attachmentOverrides.erase(it);
	}
	updatePos();
}

//--------------------------------------------------------------------
// updatePos()
//--------------------------------------------------------------------
void LLJoint::updatePos()
{
	LLVector3 pos;
	attachment_map_t::iterator it = std::max_element(m_attachmentOverrides.begin(),
													 m_attachmentOverrides.end(),
													 attachment_map_iter_compare_name<LLJoint::attachment_map_t::value_type>);
	if (it != m_attachmentOverrides.end())
	{
		AttachmentOverrideRecord& rec = it->second;
		LL_DEBUGS("Avatar") << getName() << " updatePos, winner of " << m_attachmentOverrides.size() << " is attachment " << rec.name << " pos " << rec.pos << LL_ENDL;
		pos = rec.pos;
	}
	else
	{
		LL_DEBUGS("Avatar") << getName() << " updatePos, winner is posBeforeOverrides " << m_posBeforeOverrides << LL_ENDL;
		pos = m_posBeforeOverrides;
	}
	setPosition(pos);
}

//--------------------------------------------------------------------
// getWorldPosition()
//--------------------------------------------------------------------
LLVector3 LLJoint::getWorldPosition()
{
	updateWorldPRSParent();
	return mXform.getWorldPosition();
}

//-----------------------------------------------------------------------------
// getLastWorldPosition()
//-----------------------------------------------------------------------------
LLVector3 LLJoint::getLastWorldPosition()
{
	return mXform.getWorldPosition();
}
//--------------------------------------------------------------------
// setWorldPosition()
//--------------------------------------------------------------------
void LLJoint::setWorldPosition( const LLVector3& pos )
{
	if (mParent == NULL)
	{
		this->setPosition( pos );
		return;
	}

	LLMatrix4 temp_matrix = getWorldMatrix();
	temp_matrix.mMatrix[VW][VX] = pos.mV[VX];
	temp_matrix.mMatrix[VW][VY] = pos.mV[VY];
	temp_matrix.mMatrix[VW][VZ] = pos.mV[VZ];

	LLMatrix4 parentWorldMatrix = mParent->getWorldMatrix();
	LLMatrix4 invParentWorldMatrix = parentWorldMatrix.invert();

	temp_matrix *= invParentWorldMatrix;

	LLVector3 localPos(	temp_matrix.mMatrix[VW][VX],
						temp_matrix.mMatrix[VW][VY],
						temp_matrix.mMatrix[VW][VZ] );

	setPosition( localPos );
}


//--------------------------------------------------------------------
// getRotation()
//--------------------------------------------------------------------
const LLQuaternion& LLJoint::getRotation()
{
	return mXform.getRotation();
}


//--------------------------------------------------------------------
// setRotation()
//--------------------------------------------------------------------
void LLJoint::setRotation( const LLQuaternion& rot )
{
	if (rot.isFinite())
	{
	//	if (mXform.getRotation() != rot)
		{
			mXform.setRotation(rot);
			touch(MATRIX_DIRTY | ROTATION_DIRTY);
		}
	}
}


//--------------------------------------------------------------------
// getWorldRotation()
//--------------------------------------------------------------------
LLQuaternion LLJoint::getWorldRotation()
{
	updateWorldPRSParent();

	return mXform.getWorldRotation();
}

//-----------------------------------------------------------------------------
// getLastWorldRotation()
//-----------------------------------------------------------------------------
LLQuaternion LLJoint::getLastWorldRotation()
{
	return mXform.getWorldRotation();
}

//--------------------------------------------------------------------
// setWorldRotation()
//--------------------------------------------------------------------
void LLJoint::setWorldRotation( const LLQuaternion& rot )
{
	if (mParent == NULL)
	{
		this->setRotation( rot );
		return;
	}

	LLMatrix4 temp_mat(rot);

	LLMatrix4 parentWorldMatrix = mParent->getWorldMatrix();
	parentWorldMatrix.mMatrix[VW][VX] = 0;
	parentWorldMatrix.mMatrix[VW][VY] = 0;
	parentWorldMatrix.mMatrix[VW][VZ] = 0;

	LLMatrix4 invParentWorldMatrix = parentWorldMatrix.invert();

	temp_mat *= invParentWorldMatrix;

	setRotation(LLQuaternion(temp_mat));
}


//--------------------------------------------------------------------
// getScale()
//--------------------------------------------------------------------
const LLVector3& LLJoint::getScale()
{
	return mXform.getScale();	
}

//--------------------------------------------------------------------
// setScale()
//--------------------------------------------------------------------
void LLJoint::setScale( const LLVector3& scale )
{
//	if (mXform.getScale() != scale)
	{	
		mXform.setScale(scale);
		touch();
	}

}



//--------------------------------------------------------------------
// getWorldMatrix()
//--------------------------------------------------------------------
const LLMatrix4 &LLJoint::getWorldMatrix()
{
	updateWorldMatrixParent();

	return mXform.getWorldMatrix();
}


//--------------------------------------------------------------------
// setWorldMatrix()
//--------------------------------------------------------------------
void LLJoint::setWorldMatrix( const LLMatrix4& mat )
{
	LL_INFOS() << "WARNING: LLJoint::setWorldMatrix() not correctly implemented yet" << LL_ENDL;
	// extract global translation
	LLVector3 trans(	mat.mMatrix[VW][VX],
						mat.mMatrix[VW][VY],
						mat.mMatrix[VW][VZ] );

	// extract global rotation
	LLQuaternion rot( mat );

	setWorldPosition( trans );
	setWorldRotation( rot );
}

//-----------------------------------------------------------------------------
// updateWorldMatrixParent()
//-----------------------------------------------------------------------------
void LLJoint::updateWorldMatrixParent()
{
	if (mDirtyFlags & MATRIX_DIRTY)
	{
		LLJoint *parent = getParent();
		if (parent)
		{
			parent->updateWorldMatrixParent();
		}
		updateWorldMatrix();
	}
}

//-----------------------------------------------------------------------------
// updateWorldPRSParent()
//-----------------------------------------------------------------------------
void LLJoint::updateWorldPRSParent()
{
	if (mDirtyFlags & (ROTATION_DIRTY | POSITION_DIRTY))
	{
		LLJoint *parent = getParent();
		if (parent)
		{
			parent->updateWorldPRSParent();
		}

		mXform.update();
		mDirtyFlags &= ~(ROTATION_DIRTY | POSITION_DIRTY);
	}
}

//-----------------------------------------------------------------------------
// updateWorldMatrixChildren()
//-----------------------------------------------------------------------------
void LLJoint::updateWorldMatrixChildren()
{	
	if (!this->mUpdateXform) return;

	if (mDirtyFlags & MATRIX_DIRTY)
	{
		updateWorldMatrix();
	}
	for (child_list_t::iterator iter = mChildren.begin();
		 iter != mChildren.end(); ++iter)
	{
		LLJoint* joint = *iter;
		joint->updateWorldMatrixChildren();
	}
}

//-----------------------------------------------------------------------------
// updateWorldMatrix()
//-----------------------------------------------------------------------------
void LLJoint::updateWorldMatrix()
{
	if (mDirtyFlags & MATRIX_DIRTY)
	{
		sNumUpdates++;
		mXform.updateMatrix(FALSE);
		mDirtyFlags = 0x0;
	}
}

//--------------------------------------------------------------------
// getSkinOffset()
//--------------------------------------------------------------------
const LLVector3 &LLJoint::getSkinOffset()
{
	return mSkinOffset;
}


//--------------------------------------------------------------------
// setSkinOffset()
//--------------------------------------------------------------------
void LLJoint::setSkinOffset( const LLVector3& offset )
{
	mSkinOffset = offset;
}


//-----------------------------------------------------------------------------
// clampRotation()
//-----------------------------------------------------------------------------
void LLJoint::clampRotation(LLQuaternion old_rot, LLQuaternion new_rot)
{
	LLVector3 main_axis(1.f, 0.f, 0.f);

	for (child_list_t::iterator iter = mChildren.begin();
		 iter != mChildren.end(); ++iter)
	{
		LLJoint* joint = *iter;
		if (joint->isAnimatable())
		{
			main_axis = joint->getPosition();
			main_axis.normVec();
			// only care about first animatable child
			break;
		}
	}
}

// End

