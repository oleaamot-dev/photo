
/***********************************************************************
   EXIF File Tags
   Matt Martin 4/99
   Taken from "exifdump.py"
    Written by Thierry Bousch <bousch@topo.math.u-psud.fr>

 **********************************************************************/
#ifndef EXIF_TAGS_H
#define EXIF_TAGS_H

#define EXIF_STRIP_OFFSETS 0x111
#define EXIF_STRIP_BYTES 0x117
#define EXIF_NewSubFileType	0xFE
#define EXIF_ImageWidth	0x100
#define EXIF_ImageLength	0x101
#define EXIF_BitsPerSample	0x102
#define EXIF_Compression	0x103
#define EXIF_PhotometricInterpretation	0x106
#define EXIF_FillOrder	0x10A
#define EXIF_DocumentName	0x10D
#define EXIF_ImageDescription	0x10E
#define EXIF_Make	0x10F
#define EXIF_Model	0x110
#define EXIF_StripOffsets	0x111
#define EXIF_Orientation	0x112
#define EXIF_SamplesPerPixel	0x115
#define EXIF_RowsPerStrip	0x116
#define EXIF_StripByteCounts	0x117
#define EXIF_XResolution	0x11A
#define EXIF_YResolution	0x11B
#define EXIF_PlanarConfiguration	0x11C
#define EXIF_ResolutionUnit	0x128
#define EXIF_TransferFunction	0x12D
#define EXIF_Software	0x131
#define EXIF_DateTime	0x132
#define EXIF_Artist	0x13B
#define EXIF_WhitePoint	0x13E
#define EXIF_PrimaryChromaticities	0x13F
#define EXIF_TransferRange	0x156
#define EXIF_JPEGProc	0x200
#define EXIF_JPEGInterchangeFormat	0x201
#define EXIF_JPEGInterchangeFormatLength	0x202
#define EXIF_YCbCrCoefficients	0x211
#define EXIF_YCbCrSubSampling	0x212
#define EXIF_YCbCrPositioning	0x213
#define EXIF_ReferenceBlackWhite	0x214
#define EXIF_CFARepeatPatternDim	0x828
#define EXIF_CFAPattern	0x828
#define EXIF_BatteryLevel	0x828
#define EXIF_Copyright	0x829
#define EXIF_ExposureTime	0x829
#define EXIF_FNumber	0x829
#define EXIF_IPTC_NAA	0x83B
#define EXIF_ExifOffset	0x876
#define EXIF_InterColorProfile	0x877
#define EXIF_ExposureProgram	0x882
#define EXIF_SpectralSensitivity	0x882
#define EXIF_GPSInfo	0x882
#define EXIF_ISOSpeedRatings	0x882
#define EXIF_OECF	0x882
#define EXIF_ExifVersion	0x900
#define EXIF_DateTimeOriginal	0x900
#define EXIF_DateTimeDigitized	0x900
#define EXIF_ComponentsConfiguration	0x910
#define EXIF_CompressedBitsPerPixel	0x910
#define EXIF_ShutterSpeedValue	0x920
#define EXIF_ApertureValue	0x920
#define EXIF_BrightnessValue	0x920
#define EXIF_ExposureBiasValue	0x920
#define EXIF_MaxApertureValue	0x920
#define EXIF_SubjectDistance	0x920
#define EXIF_MeteringMode	0x920
#define EXIF_LightSource	0x920
#define EXIF_Flash	0x920
#define EXIF_FocalLength	0x920
#define EXIF_MakerNote	0x927
#define EXIF_UserComment	0x928
#define EXIF_SubSecTime	0x929
#define EXIF_SubSecTimeOriginal	0x929
#define EXIF_SubSecTimeDigitized	0x929
#define EXIF_FlashPixVersion	0xA00
#define EXIF_ColorSpace	0xA00
#define EXIF_ExifImageWidth	0xA00
#define EXIF_ExifImageLength	0xA00
#define EXIF_InteroperabilityOffset	0xA00
#define EXIF_FlashEnergy		     	0xA20
#define EXIF_SpatialFrequencyResponse 	0xA20
#define EXIF_FocalPlaneXResolution    	0xA20
#define EXIF_FocalPlaneYResolution    	0xA20
#define EXIF_FocalPlaneResolutionUnit 	0xA21
#define EXIF_SubjectLocation	     	0xA21
#define EXIF_ExposureIndex	     	0xA21
#define EXIF_SensingMethod	     	0xA21
#define EXIF_FileSource	0xA30
#define EXIF_Scenetype	0xA30

#endif
