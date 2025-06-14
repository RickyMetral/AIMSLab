//=============================================================================
// Copyright © 2025 NaturalPoint, Inc. All Rights Reserved.
// 
// THIS SOFTWARE IS GOVERNED BY THE OPTITRACK PLUGINS EULA AVAILABLE AT https://www.optitrack.com/about/legal/eula.html 
// AND/OR FOR DOWNLOAD WITH THE APPLICABLE SOFTWARE FILE(S) (“PLUGINS EULA”). BY DOWNLOADING, INSTALLING, ACTIVATING 
// AND/OR OTHERWISE USING THE SOFTWARE, YOU ARE AGREEING THAT YOU HAVE READ, AND THAT YOU AGREE TO COMPLY WITH AND ARE
// BOUND BY, THE PLUGINS EULA AND ALL APPLICABLE LAWS AND REGULATIONS. IF YOU DO NOT AGREE TO BE BOUND BY THE PLUGINS
// EULA, THEN YOU MAY NOT DOWNLOAD, INSTALL, ACTIVATE OR OTHERWISE USE THE SOFTWARE AND YOU MUST PROMPTLY DELETE OR
// RETURN IT. IF YOU ARE DOWNLOADING, INSTALLING, ACTIVATING AND/OR OTHERWISE USING THE SOFTWARE ON BEHALF OF AN ENTITY,
// THEN BY DOING SO YOU REPRESENT AND WARRANT THAT YOU HAVE THE APPROPRIATE AUTHORITY TO ACCEPT THE PLUGINS EULA ON
// BEHALF OF SUCH ENTITY. See license file in root directory for additional governing terms and information.
//=============================================================================


/*********************************************************************
 * \page   RigidBodyCollection.h
 * \file   RigidBodyCollection.h
 * \brief Class for storing ordered rigid body data which is accessed via an
 * index.
 * The rigid body data is extracted from an array of rigid body data
 * structures produced by NatNet and stored in the same order as this
 * input array.
*********************************************************************/


#ifndef _RIGIDBODYCOLLECTION_H_
#define _RIGIDBODYCOLLECTION_H_

#include <tuple>

#include "NatNetTypes.h"

class RigidBodyCollection
{
public:
  // Limit ourselves to a reasonable number of rigid bodies for example purposes.
  static size_t const MAX_RIGIDBODY_COUNT = 200;

  //*************************************************************************
  // Constructors
  //

  //////////////////////////////////////////////////////////////////////////
  /// Default constructor. Resulting object contains no rigid bodies.
  //////////////////////////////////////////////////////////////////////////
  RigidBodyCollection();


  //*************************************************************************
  // Member Functions.
  //

  //////////////////////////////////////////////////////////////////////////
  /// <summary>
  /// Extract and store the NatNet rigid body data. Any existing data will be
  /// lost
  /// </summary>
  /// <param name='rigidBodyData'>Pointer to an array of NatNet rigid body
  /// data structures. This structure is part of the <c>sFrameOfMocapData</c>
  /// structure sent by NatNet.<param>
  /// <param name='numRigidBodies'>Length of the <c>rigidBodyData</c> array
  /// </param>
  /// <remarks>The order of the rigid bodies is preserved.</remarks>
  //////////////////////////////////////////////////////////////////////////
  void SetRigidBodyData(sRigidBodyData const * const rigidBodyData, size_t numRigidBodies)
  {
    mNumRigidBodies = 0;
    AppendRigidBodyData(rigidBodyData, numRigidBodies);
  }

  //////////////////////////////////////////////////////////////////////////
  /// <summary>
  /// Returns the number of rigid bodies.
  /// </summary>
  /// <returns>Number of rigid bodies.</returns>
  size_t Count() const { return mNumRigidBodies; }

  //////////////////////////////////////////////////////////////////////////
  /// <summary>Gets the ID of the rigid body corresponding to the given 
  /// index.
  /// </summary>
  /// <param name='index'>Get the ID of the rigid body at this index.
  /// Valid value are 0 to RigidBodyCollection::Count() - 1</param>
  /// <returns>ID of the rigid body.</returns>
  //////////////////////////////////////////////////////////////////////////
  int GetId(size_t index) const { return mIds[index]; }

  //////////////////////////////////////////////////////////////////////////
  /// <summary>
  /// Gets the coordinates of the ith rigid body.
  /// </summary>
  /// <param name='i'>Index of the rigid body. Valid value are 
  /// 0 to RigidBodyCollection::Count() - 1</param>
  /// <returns><c>tuple</c> of x, y, z coordinates in that order.</returns>
  //////////////////////////////////////////////////////////////////////////
  const std::tuple<float,float,float>& GetCoordinates(size_t i) const { return mXYZCoord[i]; }

  //////////////////////////////////////////////////////////////////////////
  /// <summary>
  /// Gets the Quaternion of the ith rigid body.
  /// </summary>
  /// <param name='i'>Index of the rigid body. Valid value are 
  /// 0 to RigidBodyCollection::Count() - 1</param>
  /// <returns><c>tuple</c> of qx, qy, qz, qw quaternion vlaues in that 
  /// order.</returns>
  //////////////////////////////////////////////////////////////////////////
  const std::tuple<float,float,float,float>& GetQuaternion(size_t i) const { return mXYZWQuats[i]; }

  //////////////////////////////////////////////////////////////////////////
  /// <summary>Gets the ID of the ith rigid body.</summary>
  /// <param name='i'>Rigid body index. Valid value are 
  /// 0 to RigidBodyCollection::Count() - 1</param>
  /// <returns>ID of the rigid body.</returns>
  //////////////////////////////////////////////////////////////////////////
  int ID(size_t i) const { return mIds[i]; }

  //////////////////////////////////////////////////////////////////////////
  /// <summary>Appends the given rigid body to any existing rigid body data
  /// contained in self.
  /// </summary>
  /// <param name='rigidBodyData'>Pointer to an array of NatNet rigid body
  /// data structures. This structure is part of the <c>sFrameOfMocapData</c>
  /// structure sent by NatNet.<param>
  /// <param name='numRigidBodies'>Length of the <c>rigidBodyData</c> array
  /// </param>
  /// <remarks>The order of the rigid bodies is preserved.</remarks>
  //////////////////////////////////////////////////////////////////////////
  void AppendRigidBodyData(sRigidBodyData const * const rigidBodyData, size_t numRigidBodies);


  //*************************************************************************
  // Instance Variables.
  //
private:
  // Rigid body x,y,z coordinates.
  std::tuple<float,float,float> mXYZCoord[MAX_RIGIDBODY_COUNT];

  // Rigid body quaternions.
  std::tuple<float,float,float,float> mXYZWQuats[MAX_RIGIDBODY_COUNT];

  // Rigid body ID's.
  int mIds[MAX_RIGIDBODY_COUNT];

  // Number of rigid bodies.
  size_t mNumRigidBodies;
};

#endif // _RIGIDBODYCOLLECTION_H_
