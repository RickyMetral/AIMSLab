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
 * \page   MarkerPositionCollection.h
 * \file   MarkerPositionCollection.h
 *  
 *  Class for storing ordered marker positions which are
 *  then accessed by index. Labeled marker data can also be stored and
 *  accessed by index.
 * \brief  Class for storing ordered marker positions.
 *********************************************************************/

#ifndef _MARKER_POSITION_COLLECTION_H_
#define _MARKER_POSITION_COLLECTION_H_

#include <tuple>

#include "NatNetTypes.h"

/**
 * Class for storing ordered marker positions which are then accessed 
 * by index. Labeled marker data can also be stored and accessed by 
 * index.
 * \brief  Class for storing ordered marker positions.
 */
class MarkerPositionCollection
{
public:
  // Limit ourselves to reasonable number of markers for example purposes.
  static const size_t MAX_MARKER_COUNT = 1000;

  //*************************************************************************
  // Constructors
  //

  //////////////////////////////////////////////////////////////////////////
  /// Default constructor. Resulting object contains no marker data.
  //////////////////////////////////////////////////////////////////////////
  MarkerPositionCollection();


  //*************************************************************************
  // Member Functions
  //

 /**
  * \brief Gets the x, y, z coordinates of the ith marker. Const version.
  * 
  * \param i Marker index. Valid value are 
  *  0 to MarkerPositionCollection::MarkerPositionCount() - 1
  * \return A const ref to the (x, y, z) tuple of coordinates
  */
  const std::tuple<float,float,float>& GetMarkerPosition (size_t i) const { return mMarkerPositions[i]; }

  //////////////////////////////////////////////////////////////////////////
  /// <summary>
  /// Gets the x, y, z coordinates of the ith marker. Non-const version.
  /// </summary>
  /// <param name='i'>Marker index. Valid value are 
  /// 0 to MarkerPositionCollection::MarkerPositionCount() - 1</param>
  /// <returns>A ref to the (x, y, z) tuple of coordinates.</returns>
  //////////////////////////////////////////////////////////////////////////
  std::tuple<float,float,float>& GetMarkerPosition (size_t i) { return mMarkerPositions[i]; }

  //////////////////////////////////////////////////////////////////////////
  /// <summary>
  /// Gets the labeled marker data for the ith labeled marker. Const version.
  /// </summary>
  /// <param name='i'>Marker index. Valid value are 
  /// 0 to MarkerPositionCollection::LabeledMarkerPositionCount() - 1</param>
  /// <returns>Const ref to marker data structure.</returns>
  //////////////////////////////////////////////////////////////////////////
  const sMarker& GetLabeledMarker (size_t i) const { return mLabledMarkers[i]; }

  //////////////////////////////////////////////////////////////////////////
  /// <summary>
  /// Gets the labeled marker data for the ith labeled marker. Non-const version.
  /// </summary>
  /// <param name='i'>Marker index. Valid value are 
  /// 0 to MarkerPositionCollection::LabeledMarkerPositionCount() - 1</param>
  /// <returns>Ref to marker data structure.</returns>
  //////////////////////////////////////////////////////////////////////////
  sMarker& GetLabeledMarker (size_t i) { return mLabledMarkers[i]; }

  //////////////////////////////////////////////////////////////////////////
  /// <summary>
  /// Gets the number of marker positions.
  /// </summary>
  /// <returns>Number of marker positions.</returns>
  //////////////////////////////////////////////////////////////////////////
  size_t MarkerPositionCount() const { return mMarkerPositionCount; }

  //////////////////////////////////////////////////////////////////////////
  /// <summary>
  /// Gets the number of labeled markers.
  /// </summary>
  /// <returns>Number of labeled marker.</returns>
  //////////////////////////////////////////////////////////////////////////
  size_t LabeledMarkerPositionCount() const { return mLabledMarkerCount; }

  void AppendMarkerPositions(float markerData[][3], size_t numMarkers);
  
  //////////////////////////////////////////////////////////////////////////
  /// <summary>
  /// Sets marker position data. Any existing marker position data will be
  /// lost.
  /// </summar>
  /// <param name='markerData'>Array of (x, y, z) coordinates for the markers.
  /// </param>
  /// <param name='numMarkers'>Number of marker positions.<param>
  //////////////////////////////////////////////////////////////////////////
  void SetMarkerPositions(float markerData[][3], size_t numMarkers)
  {
    mMarkerPositionCount = 0;
    AppendMarkerPositions(markerData, numMarkers);
  }

  void AppendLabledMarkers(sMarker markers[], size_t numMarkers);

  //////////////////////////////////////////////////////////////////////////
  /// <summary>
  /// Sets labeled marker data. Any existing labeled marker data will be
  /// lost.
  /// </summar>
  /// <param name='markers'>Array of labeled marker data structures.</param>
  /// <param name='numMarkers'>Number of labeled markers.</param>
  //////////////////////////////////////////////////////////////////////////
  void SetLabledMarkers(sMarker markers[], size_t numMarkers) 
  { 
    mLabledMarkerCount = 0; 
    AppendLabledMarkers(markers, numMarkers);
  }

private:
  //*************************************************************************
  // Instance Variables
  //

  // Array of (x, y, z) marker positions.
  std::tuple<float,float,float> mMarkerPositions[MAX_MARKER_COUNT];
  // Number of marker positions.
  size_t mMarkerPositionCount;

  // Array of labeled marker structures.
  sMarker mLabledMarkers[MAX_MARKER_COUNT];
  // Number of labeled marker structures.
  size_t mLabledMarkerCount;
};

#endif // _MARKER_POSITION_COLLECTION_H_