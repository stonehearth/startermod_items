#ifndef _RADIANT_VOXEL_QUBICLE_FILE_FORMAT_H
#define _RADIANT_VOXEL_QUBICLE_FILE_FORMAT_H

BEGIN_RADIANT_VOXEL_NAMESPACE

// As documented at: http://www.minddesk.com/od_dataexchange_qb.php

enum ColorFormat {
   COLOR_FORAMT_RGBA = 0,
   COLOR_FORAMT_BRGA = 1,
};

enum AxisOrientation {
   LEFT_HANDED       = 0,
   RIGHT_HANDED      = 1,
};

enum Compression {
   COMPRESSION_NONE  = 0,
   COMPRESSION_RLE   = 1,
};

enum CompressionRLEFlag {
   RLE_CODEFLAG      = 2,
   RLE_NEXTSLICEFLAG = 4,
};

enum VisibilityMethod {
   VISIBILITY_BINARY = 0, // 0 == invisible, 255 == visible
   VISIBILITY_MASK   = 1, // See VisibilityMask...
};

// Assumes left hand coordinate systsm
enum VisibilityMask {
   LEFT_MASK = 2,     // left side visible   (+X)
   RIGHT_MASK = 4,    // right side visible  (-X)
   TOP_MASK = 8,      // top side visible    (+Y)
   BOTTOM_MASK = 16,  // bottom side visible (-Y)
   FRONT_MASK = 32,   // front side visible  (+Z)
   BACK_MASK = 64,    // back side visible   (-Z)
};

struct QbHeader
{
   uint32      version;
   uint32      colorFormat;
   uint32      axisOrientation;
   uint32      compression;
   uint32      visibilityMethod;
   uint32      numMatricies;
};

END_RADIANT_VOXEL_NAMESPACE

#endif //  _RADIANT_VOXEL_QUBICLE_FILE_FORMAT_H
