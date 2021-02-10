// lvis_release_reader.c
// 
// HOWTO compile:  gcc -Wall lvis_release_reader.c -o lvis_release_reader
// 
// (or just run 'make', as there is a Makefile included)
// 
// HOWTO execute:
// ./lvis_release_reader LVIS_HB_2003_grid_0.lce
// ./lvis_release_reader LVIS_HOW_2003_grid_0.lce -lon 290.00-291.40 -lat 44.74-44.76
// ./lvis_release_reader LVIS_HOW_2003_Flux_Tower_West_grid_0.lgw -c
// ./lvis_release_reader LVIS_US_CA_day4_2008_VECT_20081120.lge.1.03 -c -i -lge -r 1.03
// 
// Version 1.0
// Begun: 2004/08/19
// David Lloyd Rabine
// Science Systems and Applications, Inc.
// NASA Goddard Space Flight Center
// Code 694.0 Laser Remote Sensing Branch
// Greenbelt, MD 20771 USA
// david.l.rabine@nasa.gov
// office 301-614-6771
// fax    301-614-6744
// For more information on the LVIS sensor, contact James Bryan Blair (james.b.blair@nasa.gov)
// and/or visit http://lvis.gsfc.nasa.gov
//
// EXAMPLE program to convert the LVIS data file formats (LCE, LGW and LGE) 
// to ASCII
// 
// Version 1.01 - 20051028
// added LFID and SHOTNUMBER to the structure
// 
// Version 1.02 - 20060418
// added TIME to the structure (for ocean data, we're going to need time for tide calculations)
//
// Version 1.03 - 20081120
// * several items have been added to the latest release version
//   azimuth, incidentangle, range in lce,lge and lgw and txwave in lgw
// * BUG Squashed!  Fixed assumption on byte size (ran into this compiling on 64 bit systems)
// * Added structures for all release versions to date
// * Introduced an automatic file version/type detection routine (not pretty, but mostly effective)
// * Added the -t header option if you want a header above each column
// * Added the -n option to only extract that many samples
// * Added the -r option to force the release version (incase the autodetection fails)
//  
// Version 1.04 - 20111213 dlr
// * added a 1.04 structure with 10 bit values and more waveform samples for RX waveform
// * modified headers to user platform consistent defines for integers for all the structure defines
// * fixed some of the auto detection code (was using double functions on float variables)
// * inserted a #define to update all of the loops with a single change (LVIS_VERSION_COUNT)
//  
// Systems Tested on:  Linux Slackware 12.1, CentOS 5.2(xi386 & x86_64)
//                     Solaris Ultra 2
//                     Mac OS X (10.5.1)
//

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lvis_release_structures.h"

typedef unsigned char  byte;
typedef unsigned short word;
typedef unsigned int  dword;

#ifndef  DEFAULT_DATA_RELEASE_VERSION
#define  DEFAULT_DATA_RELEASE_VERSION ((float) 1.03)
#endif

#ifndef  LVIS_RELEASE_READER_VERSION
#define  LVIS_RELEASE_READER_VERSION  ((float) 1.04)
#endif

#ifndef  LVIS_RELEASE_READER_VERSION_DATE
#define  LVIS_RELEASE_READER_VERSION_DATE 20111213
#endif

#ifndef  GENLIB_LITTLE_ENDIAN
#define  GENLIB_LITTLE_ENDIAN 0x00
#endif

#ifndef  GENLIB_BIG_ENDIAN
#define  GENLIB_BIG_ENDIAN 0x01
#endif

#ifndef  LVIS_VERSION_COUNT
#define  LVIS_VERSION_COUNT 5  // 1.00 -> 1.04 is 5 total versions
#endif

#ifndef  VERSION_TESTBLOCK_LENGTH
#define  VERSION_TESTBLOCK_LENGTH (128 * 1024) // 128k should be enough to figure it out
#endif

// prototypes we need
double host_double(double input_double,int host_endian);
float  host_float(float input_float,int host_endian);

// if you want a little more info to start...uncomment this
/* #define DEBUG_ON */

// self contained file version detection routine
int detect_release_version(char * filename, int * fileType, float * fileVersion, int myendian)
{
   // any misalignment of the data structure will result in HUGE double values
   // read in a block of data and sum up the abs() of some doubles and pick
   // the lowest value... that will match the version / type.
  
   struct lvis_lce_v1_00 * lce100; struct lvis_lge_v1_00 * lge100; struct lvis_lgw_v1_00 * lgw100;
   struct lvis_lce_v1_01 * lce101; struct lvis_lge_v1_01 * lge101; struct lvis_lgw_v1_01 * lgw101;
   struct lvis_lce_v1_02 * lce102; struct lvis_lge_v1_02 * lge102; struct lvis_lgw_v1_02 * lgw102;
   struct lvis_lce_v1_03 * lce103; struct lvis_lge_v1_03 * lge103; struct lvis_lgw_v1_03 * lgw103;
   struct lvis_lce_v1_04 * lce104; struct lvis_lge_v1_04 * lge104; struct lvis_lgw_v1_04 * lgw104;

   long          fileSize;
   int           i,mintype,type,minversion,version,status=0,maxpackets,maxsize;
   FILE          *fptest=NULL;
   double        testTotal;
   double        testTotalMin = (double) 0.0;
   double        testTotalsLat[3][LVIS_VERSION_COUNT];  // 3 file types and X versions to test
   double        testTotalsLon[3][LVIS_VERSION_COUNT];  // 3 file types and X versions to test
   double        testTotalsAlt[3][LVIS_VERSION_COUNT];  // 3 file types and X versions to test
   unsigned char testBlock[VERSION_TESTBLOCK_LENGTH];
   
   lce100 = (struct lvis_lce_v1_00 * ) testBlock;
   lce101 = (struct lvis_lce_v1_01 * ) testBlock;
   lce102 = (struct lvis_lce_v1_02 * ) testBlock;
   lce103 = (struct lvis_lce_v1_03 * ) testBlock;
   lce104 = (struct lvis_lce_v1_04 * ) testBlock;
   lge100 = (struct lvis_lge_v1_00 * ) testBlock;
   lge101 = (struct lvis_lge_v1_01 * ) testBlock;
   lge102 = (struct lvis_lge_v1_02 * ) testBlock;
   lge103 = (struct lvis_lge_v1_03 * ) testBlock;
   lge104 = (struct lvis_lge_v1_04 * ) testBlock;
   lgw100 = (struct lvis_lgw_v1_00 * ) testBlock;
   lgw101 = (struct lvis_lgw_v1_01 * ) testBlock;
   lgw102 = (struct lvis_lgw_v1_02 * ) testBlock;
   lgw103 = (struct lvis_lgw_v1_03 * ) testBlock;
   lgw104 = (struct lvis_lgw_v1_04 * ) testBlock;
   
   // determine the biggest packet, which limits how many to read (to be fair)
   maxsize = 1;
   if(sizeof(struct lvis_lce_v1_00) > maxsize) maxsize = sizeof(struct lvis_lce_v1_00);
   if(sizeof(struct lvis_lce_v1_01) > maxsize) maxsize = sizeof(struct lvis_lce_v1_01);
   if(sizeof(struct lvis_lce_v1_02) > maxsize) maxsize = sizeof(struct lvis_lce_v1_02);
   if(sizeof(struct lvis_lce_v1_03) > maxsize) maxsize = sizeof(struct lvis_lce_v1_03);
   if(sizeof(struct lvis_lce_v1_04) > maxsize) maxsize = sizeof(struct lvis_lce_v1_04);
   if(sizeof(struct lvis_lge_v1_00) > maxsize) maxsize = sizeof(struct lvis_lge_v1_00);
   if(sizeof(struct lvis_lge_v1_01) > maxsize) maxsize = sizeof(struct lvis_lge_v1_01);
   if(sizeof(struct lvis_lge_v1_02) > maxsize) maxsize = sizeof(struct lvis_lge_v1_02);
   if(sizeof(struct lvis_lge_v1_03) > maxsize) maxsize = sizeof(struct lvis_lge_v1_03);
   if(sizeof(struct lvis_lge_v1_04) > maxsize) maxsize = sizeof(struct lvis_lge_v1_04);
   if(sizeof(struct lvis_lgw_v1_00) > maxsize) maxsize = sizeof(struct lvis_lgw_v1_00);
   if(sizeof(struct lvis_lgw_v1_01) > maxsize) maxsize = sizeof(struct lvis_lgw_v1_01);
   if(sizeof(struct lvis_lgw_v1_02) > maxsize) maxsize = sizeof(struct lvis_lgw_v1_02);
   if(sizeof(struct lvis_lgw_v1_03) > maxsize) maxsize = sizeof(struct lvis_lgw_v1_03);
   if(sizeof(struct lvis_lgw_v1_04) > maxsize) maxsize = sizeof(struct lvis_lgw_v1_04);
      
   // open up the file for read access
   if((fptest = fopen((char *)filename,"rb"))==NULL)
     {
	fprintf(stderr,"Error opening the input file: %s\n",filename);
	exit(-1);
     }

   fileSize = VERSION_TESTBLOCK_LENGTH;
   status=fread(testBlock,1,sizeof(testBlock),fptest);
   // see if the file wasn't big enough
   if( (status!=sizeof(testBlock)) && (status>0)) fileSize = status;
   maxpackets = fileSize  / maxsize;

#ifdef DEBUG_ON  // uncomment the #define up top if you want to see these messages
   fprintf(stdout,"maxpackets = %d\n",maxpackets);
#endif
      
   // clear out the totals and our min variables
   mintype = 0; minversion = 0;
   for(type=0;type<3;type++)
     for( version=0; version<LVIS_VERSION_COUNT ; version++)
       {	  
	  testTotalsLat[type][version] = 0.0;
	  testTotalsLon[type][version] = 0.0;
	  testTotalsAlt[type][version] = 0.0;	  
       }
   
   if(status == fileSize)
     {     	
	for(i=0;i<maxpackets;i++)
	  for(type=0;type<3;type++)
	    for( version=0; version<LVIS_VERSION_COUNT ; version++)
	      {
		 if(type==0 && version==0)
		   {
		      testTotalsLat[type][version] += fabs(host_double(lce100[i].tlat,myendian));
		      testTotalsLon[type][version] += fabs(host_double(lce100[i].tlon,myendian));
		      testTotalsAlt[type][version] += fabsf(host_float(lce100[i].zt,myendian));
		   }
		 if(type==0 && version==1)
		   {
		      testTotalsLat[type][version] += fabs(host_double(lce101[i].tlat,myendian));
		      testTotalsLon[type][version] += fabs(host_double(lce101[i].tlon,myendian));
		      testTotalsAlt[type][version] += fabsf(host_float(lce101[i].zt,myendian));		      
		   }
		 if(type==0 && version==2)
		   {
		      testTotalsLat[type][version] += fabs(host_double(lce102[i].tlat,myendian));
		      testTotalsLon[type][version] += fabs(host_double(lce102[i].tlon,myendian));
		      testTotalsAlt[type][version] += fabsf(host_float(lce102[i].zt,myendian));
		   }
		 if(type==0 && version==3)
		   {
		      testTotalsLat[type][version] += fabs(host_double(lce103[i].tlat,myendian));
		      testTotalsLon[type][version] += fabs(host_double(lce103[i].tlon,myendian));
		      testTotalsAlt[type][version] += fabsf(host_float(lce103[i].zt,myendian));		      
		   }
		 if(type==0 && version==4)
		   {
		      testTotalsLat[type][version] += fabs(host_double(lce104[i].tlat,myendian));
		      testTotalsLon[type][version] += fabs(host_double(lce104[i].tlon,myendian));
		      testTotalsAlt[type][version] += fabsf(host_float(lce104[i].zt,myendian));		      
		   }
		 if(type==1 && version==0)
		   {
		      testTotalsLat[type][version] += fabs(host_double(lge100[i].glat,myendian));
		      testTotalsLon[type][version] += fabs(host_double(lge100[i].glon,myendian));
		      testTotalsAlt[type][version] += fabsf(host_float(lge100[i].zg,myendian));
		   }
		 if(type==1 && version==1)
		   {
		      testTotalsLat[type][version] += fabs(host_double(lge101[i].glat,myendian));
		      testTotalsLon[type][version] += fabs(host_double(lge101[i].glon,myendian));
		      testTotalsAlt[type][version] += fabsf(host_float(lge101[i].zg,myendian));		      
		   }
		 if(type==1 && version==2)
		   {
		      testTotalsLat[type][version] += fabs(host_double(lge102[i].glat,myendian));
		      testTotalsLon[type][version] += fabs(host_double(lge102[i].glon,myendian));
		      testTotalsAlt[type][version] += fabsf(host_float(lge102[i].zg,myendian));
		   }
		 if(type==1 && version==3)
		   {
		      testTotalsLat[type][version] += fabs(host_double(lge103[i].glat,myendian));
		      testTotalsLon[type][version] += fabs(host_double(lge103[i].glon,myendian));
		      testTotalsAlt[type][version] += fabsf(host_float(lge103[i].zg,myendian));		      
		   }
		 if(type==1 && version==4)
		   {
		      testTotalsLat[type][version] += fabs(host_double(lge104[i].glat,myendian));
		      testTotalsLon[type][version] += fabs(host_double(lge104[i].glon,myendian));
		      testTotalsAlt[type][version] += fabsf(host_float(lge104[i].zg,myendian));		      
		   }
		 if(type==2 && version==0)
		   {
		      testTotalsLat[type][version] += fabs(host_double(lgw100[i].lat0,myendian));
		      testTotalsLon[type][version] += fabs(host_double(lgw100[i].lon0,myendian));
		      testTotalsAlt[type][version] += fabsf(host_float(lgw100[i].z0,myendian));
		   }
		 if(type==2 && version==1)
		   {
		      testTotalsLat[type][version] += fabs(host_double(lgw101[i].lat0,myendian));
		      testTotalsLon[type][version] += fabs(host_double(lgw101[i].lon0,myendian));
		      testTotalsAlt[type][version] += fabsf(host_float(lgw101[i].z0,myendian));		      
		   }
		 if(type==2 && version==2)
		   {
		      testTotalsLat[type][version] += fabs(host_double(lgw102[i].lat0,myendian));
		      testTotalsLon[type][version] += fabs(host_double(lgw102[i].lon0,myendian));
		      testTotalsAlt[type][version] += fabsf(host_float(lgw102[i].z0,myendian));
		   }
		 if(type==2 && version==3)
		   {
		      testTotalsLat[type][version] += fabs(host_double(lgw103[i].lat0,myendian));
		      testTotalsLon[type][version] += fabs(host_double(lgw103[i].lon0,myendian));
		      testTotalsAlt[type][version] += fabsf(host_float(lgw103[i].z0,myendian));		      
		   }
		 if(type==2 && version==4)
		   {
		      testTotalsLat[type][version] += fabs(host_double(lgw104[i].lat0,myendian));
		      testTotalsLon[type][version] += fabs(host_double(lgw104[i].lon0,myendian));
		      testTotalsAlt[type][version] += fabsf(host_float(lgw104[i].z0,myendian));
		   }
	      }

	mintype = 0 ; minversion = 0;
	for(type=0;type<3;type++)
	  for( version=0; version<LVIS_VERSION_COUNT; version++ )
	      {
		 testTotal = testTotalsLat[type][version] +
		   testTotalsLon[type][version] +
		   testTotalsAlt[type][version];
		 // if we have not yet set a minimum and our new number is a real number
		 if( (testTotalMin == (double) 0.0) && isnan(testTotal)==0)
		   {     
		      testTotalMin = testTotal;
#ifdef DEBUG_ON  // uncomment the #define up top if you want to see these messages
		      fprintf(stdout,"SET A MIN! type = %d version = 1.0%1d = %f\n",type,version,testTotal);
#endif
		   }
		 else
		   if(testTotal < testTotalMin && isnan(testTotal)==0) 
		     {
			mintype = type;
			minversion = version;
			testTotalMin = testTotal;
#ifdef DEBUG_ON  // uncomment the #define up top if you want to see these messages
			fprintf(stdout,"SET A NEW MIN! type = %d version = 1.0%1d = %f\n",type,version,testTotal);
#endif
		     }
		 
#ifdef DEBUG_ON  // uncomment the #define up top if you want to see these messages
		 fprintf(stdout,"type = %d version = 1.0%1d = %f\n",type,version,testTotal);
#endif
	      }
	
#ifdef DEBUG_ON  // uncomment the #define up top if you want to see these messages
	fprintf(stdout,"SOLVED:  type = %d version = 1.0%1d\n",mintype,minversion);
#endif
     }
   
   if(fptest!=NULL) fclose(fptest);

   if(mintype == 0) *fileType = LVIS_RELEASE_FILETYPE_LCE;
   if(mintype == 1) *fileType = LVIS_RELEASE_FILETYPE_LGE;
   if(mintype == 2) *fileType = LVIS_RELEASE_FILETYPE_LGW;
   
   if(minversion == 0) *fileVersion = ((float)1.00);
   if(minversion == 1) *fileVersion = ((float)1.01);
   if(minversion == 2) *fileVersion = ((float)1.02);
   if(minversion == 3) *fileVersion = ((float)1.03);
   if(minversion == 4) *fileVersion = ((float)1.04);
   
   return status;
}

double host_double(double input_double,int host_endian)
{
   double return_value;
   unsigned char * inptr, * outptr;
   
   inptr = (unsigned char *) &input_double;
   outptr = (unsigned char *) &return_value;
   
   if(host_endian == GENLIB_LITTLE_ENDIAN)
     {
	outptr[0] = inptr[7];
	outptr[1] = inptr[6];
	outptr[2] = inptr[5];
	outptr[3] = inptr[4];
	outptr[4] = inptr[3];
	outptr[5] = inptr[2];
	outptr[6] = inptr[1];
	outptr[7] = inptr[0];
     }
   
   if(host_endian == GENLIB_BIG_ENDIAN)
     return input_double;
   else
     return return_value;
}

float host_float(float input_float,int host_endian)
{
   float return_value;
   unsigned char * inptr, * outptr;
   
   inptr = (unsigned char *) &input_float;
   outptr = (unsigned char *) &return_value;
   
   if(host_endian == GENLIB_LITTLE_ENDIAN)
     {
	outptr[0] = inptr[3];
	outptr[1] = inptr[2];
	outptr[2] = inptr[1];
	outptr[3] = inptr[0];
    }
   
   if(host_endian == GENLIB_BIG_ENDIAN)
     return input_float;
   else
     return return_value;
}

dword host_dword(dword input_dword,int host_endian)
{
   dword return_value;
   unsigned char * inptr, * outptr;
   
   inptr = (unsigned char *) &input_dword;
   outptr = (unsigned char *) &return_value;
   
   if(host_endian == GENLIB_LITTLE_ENDIAN)
     {
	outptr[0] = inptr[3];
	outptr[1] = inptr[2];
	outptr[2] = inptr[1];
	outptr[3] = inptr[0];
    }
   
   if(host_endian == GENLIB_BIG_ENDIAN)
     return input_dword;
   else
     return return_value;
}

word host_word(word input_word,int host_endian)
{
   word return_value;
   unsigned char * inptr, * outptr;
   
   inptr = (unsigned char *) &input_word;
   outptr = (unsigned char *) &return_value;
   
   if(host_endian == GENLIB_LITTLE_ENDIAN)
     {
	outptr[0] = inptr[1];
	outptr[1] = inptr[0];
    }
   if(host_endian == GENLIB_BIG_ENDIAN)
     return input_word;
   else
     return return_value;
}

int host_endian(void)
{
   int host_endian;
   
   union 
     {
	char c; int i; 
     }
   tester;
   
   tester.i = 0;
   tester.c = 1;
   
   if(tester.i == 1) 
     host_endian = GENLIB_LITTLE_ENDIAN;
   else
     host_endian = GENLIB_BIG_ENDIAN;
   
   return host_endian;
}

void print_lce_column_headers(float dataVersion,int indexcol,char * delim)
{
   int i;
   
   if(indexcol==1) fprintf(stdout,"%s%s",lvis_index_header_string,delim);
   if(dataVersion == ((float)1.00))
     {
	for(i=0;i<(LVIS_LCE_V1_00_ELEMENTS-1);i++) 
	  { fprintf(stdout,"%s%s",lvis_lce_v1_00_header[i],delim); }
	fprintf(stdout,"%s\n",lvis_lce_v1_00_header[i]);
     }
   if(dataVersion == ((float)1.01))
     {
	for(i=0;i<(LVIS_LCE_V1_01_ELEMENTS-1);i++) 
	  { fprintf(stdout,"%s%s",lvis_lce_v1_01_header[i],delim); }
	fprintf(stdout,"%s\n",lvis_lce_v1_01_header[i]);
     }
   if(dataVersion == ((float)1.02))
     {
	for(i=0;i<(LVIS_LCE_V1_02_ELEMENTS-1);i++) 
	  { fprintf(stdout,"%s%s",lvis_lce_v1_02_header[i],delim); }
	fprintf(stdout,"%s\n",lvis_lce_v1_02_header[i]);
     }
   if(dataVersion == ((float)1.03))
     {
	for(i=0;i<(LVIS_LCE_V1_03_ELEMENTS-1);i++) 
	  { fprintf(stdout,"%s%s",lvis_lce_v1_03_header[i],delim); }
	fprintf(stdout,"%s\n",lvis_lce_v1_03_header[i]);
     }   
   if(dataVersion == ((float)1.04))
     {
	for(i=0;i<(LVIS_LCE_V1_04_ELEMENTS-1);i++) 
	  { fprintf(stdout,"%s%s",lvis_lce_v1_04_header[i],delim); }
	fprintf(stdout,"%s\n",lvis_lce_v1_04_header[i]);
     }   
}

void print_lce_data_v1_00(unsigned char *lcedata, int indexcol, unsigned int colnum, char * delim,
			  double minlat, double maxlat, double minlon, double maxlon)
{
   struct lvis_lce_v1_00 * lce;
   lce = (struct lvis_lce_v1_00 * ) lcedata;
   
   if(lce->tlon>minlon && lce->tlon<maxlon && lce->tlat>minlat && lce->tlat<maxlat)
     {
	if(indexcol==1) fprintf(stdout,"%10i%s",colnum++,delim);
	fprintf(stdout,"%14.10f%s%14.10f%s%9.4f\n",lce->tlon,delim,lce->tlat,delim,lce->zt);
     }
}

void print_lce_data_v1_01(unsigned char *lcedata, int indexcol, unsigned int colnum, char * delim,
			  double minlat, double maxlat, double minlon, double maxlon)
{
   struct lvis_lce_v1_01 * lce;
   lce = (struct lvis_lce_v1_01 * ) lcedata;
   
   if(lce->tlon>minlon && lce->tlon<maxlon && lce->tlat>minlat && lce->tlat<maxlat)
     {
	if(indexcol==1) fprintf(stdout,"%10i%s",colnum++,delim);
	fprintf(stdout,"%u%s%u%s",lce->lfid,delim,lce->shotnumber,delim);
	fprintf(stdout,"%14.10f%s%14.10f%s%9.4f\n",lce->tlon,delim,lce->tlat,delim,lce->zt);
     }
}

void print_lce_data_v1_02(unsigned char *lcedata, int indexcol, unsigned int colnum, char * delim,
			  double minlat, double maxlat, double minlon, double maxlon)
{
   struct lvis_lce_v1_02 * lce;
   lce = (struct lvis_lce_v1_02 * ) lcedata;
   
   if(lce->tlon>minlon && lce->tlon<maxlon && lce->tlat>minlat && lce->tlat<maxlat)
     {
	if(indexcol==1) fprintf(stdout,"%10i%s",colnum++,delim);
	fprintf(stdout,"%u%s%u%s%12.6f%s",lce->lfid,delim,lce->shotnumber,delim,lce->lvistime,delim);
	fprintf(stdout,"%14.10f%s%14.10f%s%9.4f\n",lce->tlon,delim,lce->tlat,delim,lce->zt);
     }
}

void print_lce_data_v1_03(unsigned char *lcedata, int indexcol, unsigned int colnum, char * delim,
			  double minlat, double maxlat, double minlon, double maxlon)
{
   struct lvis_lce_v1_03 * lce;
   lce = (struct lvis_lce_v1_03 * ) lcedata;
   
   if(lce->tlon>minlon && lce->tlon<maxlon && lce->tlat>minlat && lce->tlat<maxlat)
     {
	if(indexcol==1) fprintf(stdout,"%10i%s",colnum++,delim);
	fprintf(stdout,"%u%s%u%s",lce->lfid,delim,lce->shotnumber,delim);
	fprintf(stdout,"%9.4f%s%9.4f%s%9.4f%s",lce->azimuth,delim,lce->incidentangle,delim,lce->range,delim);
	fprintf(stdout,"%12.6f%s",lce->lvistime,delim);
	fprintf(stdout,"%14.10f%s%14.10f%s%9.4f\n",lce->tlon,delim,lce->tlat,delim,lce->zt);
     }
}

void print_lge_column_headers(float dataVersion,int indexcol, char * delim)
{
   int i;
   
   if(indexcol==1) fprintf(stdout,"%s%s",lvis_index_header_string,delim);
   if(dataVersion == ((float)1.00))
     {
	for(i=0;i<(LVIS_LGE_V1_00_ELEMENTS-1);i++) 
	  { fprintf(stdout,"%s%s",lvis_lge_v1_00_header[i],delim); }
	fprintf(stdout,"%s\n",lvis_lge_v1_00_header[i]);
     }
   if(dataVersion == ((float)1.01))
     {
	for(i=0;i<(LVIS_LGE_V1_01_ELEMENTS-1);i++) 
	  { fprintf(stdout,"%s%s",lvis_lge_v1_01_header[i],delim); }
	fprintf(stdout,"%s\n",lvis_lge_v1_01_header[i]);
     }
   if(dataVersion == ((float)1.02))
     {
	for(i=0;i<(LVIS_LGE_V1_02_ELEMENTS-1);i++) 
	  { fprintf(stdout,"%s%s",lvis_lge_v1_02_header[i],delim); }
	fprintf(stdout,"%s\n",lvis_lge_v1_02_header[i]);
     }
   if(dataVersion == ((float)1.03))
     {
	for(i=0;i<(LVIS_LGE_V1_03_ELEMENTS-1);i++) 
	  { fprintf(stdout,"%s%s",lvis_lge_v1_03_header[i],delim); }
	fprintf(stdout,"%s\n",lvis_lge_v1_03_header[i]);
     }
   if(dataVersion == ((float)1.04))
     {
	for(i=0;i<(LVIS_LGE_V1_04_ELEMENTS-1);i++) 
	  { fprintf(stdout,"%s%s",lvis_lge_v1_04_header[i],delim); }
	fprintf(stdout,"%s\n",lvis_lge_v1_04_header[i]);
     }
}

void print_lge_data_v1_00(unsigned char *lgedata, int indexcol, unsigned int colnum, char * delim,
			  double minlat, double maxlat, double minlon, double maxlon)
{
   struct lvis_lge_v1_00 * lge;
   lge = (struct lvis_lge_v1_00 * ) lgedata;
   
   // print the data in tab delimited columns for this data block
   if(lge->glon>minlon && lge->glon<maxlon && lge->glat>minlat && lge->glat<maxlat)
     {  
	if(indexcol==1) fprintf(stdout,"%10i%s",colnum,delim);
	fprintf(stdout,"%14.10f%s%14.10f%s%9.4f%s",lge->glon,delim,lge->glat,delim,lge->zg,delim);
	fprintf(stdout,"%9.4f%s%9.4f%s%9.4f%s%9.4f\n",lge->rh25,delim,lge->rh50,delim,lge->rh75,delim,lge->rh100);
	// the format for the %f print is:
	//     # total number of digits (including decimal) . # digits after the decimal
     }
}

void print_lge_data_v1_01(unsigned char *lgedata, int indexcol, unsigned int colnum, char * delim,
			  double minlat, double maxlat, double minlon, double maxlon)
{
   struct lvis_lge_v1_01 * lge;
   lge = (struct lvis_lge_v1_01 * ) lgedata;
   
   // print the data in tab delimited columns for this data block
   if(lge->glon>minlon && lge->glon<maxlon && lge->glat>minlat && lge->glat<maxlat)
     {  
	if(indexcol==1) fprintf(stdout,"%10i%s",colnum,delim);
	fprintf(stdout,"%u%s%u%s",lge->lfid,delim,lge->shotnumber,delim);
	fprintf(stdout,"%14.10f%s%14.10f%s%9.4f%s",lge->glon,delim,lge->glat,delim,lge->zg,delim);
	fprintf(stdout,"%9.4f%s%9.4f%s%9.4f%s%9.4f\n",lge->rh25,delim,lge->rh50,delim,lge->rh75,delim,lge->rh100);
	// the format for the %f print is:
	//     # total number of digits (including decimal) . # digits after the decimal
     }
}

void print_lge_data_v1_02(unsigned char *lgedata, int indexcol, unsigned int colnum, char * delim,
			  double minlat, double maxlat, double minlon, double maxlon)
{
   struct lvis_lge_v1_02 * lge;
   lge = (struct lvis_lge_v1_02 * ) lgedata;
   
   // print the data in tab delimited columns for this data block
   if(lge->glon>minlon && lge->glon<maxlon && lge->glat>minlat && lge->glat<maxlat)
     {  
	if(indexcol==1) fprintf(stdout,"%10i%s",colnum,delim);
	fprintf(stdout,"%u%s%u%s%12.6f%s",lge->lfid,delim,lge->shotnumber,delim,lge->lvistime,delim);
	fprintf(stdout,"%14.10f%s%14.10f%s%9.4f%s",lge->glon,delim,lge->glat,delim,lge->zg,delim);
	fprintf(stdout,"%9.4f%s%9.4f%s%9.4f%s%9.4f\n",lge->rh25,delim,lge->rh50,delim,lge->rh75,delim,lge->rh100);
	// the format for the %f print is:
	//     # total number of digits (including decimal) . # digits after the decimal
     }
}

void print_lge_data_v1_03(unsigned char *lgedata, int indexcol, unsigned int colnum, char * delim,
			  double minlat, double maxlat, double minlon, double maxlon)
{
   struct lvis_lge_v1_03 * lge;
   lge = (struct lvis_lge_v1_03 * ) lgedata;
   
   // print the data in tab delimited columns for this data block
   if(lge->glon>minlon && lge->glon<maxlon && lge->glat>minlat && lge->glat<maxlat)
     {  
	if(indexcol==1) fprintf(stdout,"%10i%s",colnum,delim);
	fprintf(stdout,"%u%s%u%s%12.6f%s",lge->lfid,delim,lge->shotnumber,delim,lge->lvistime,delim);
	fprintf(stdout,"%9.4f%s%9.4f%s%9.4f%s",lge->azimuth,delim,lge->incidentangle,delim,lge->range,delim);
	fprintf(stdout,"%14.10f%s%14.10f%s%9.4f%s",lge->glon,delim,lge->glat,delim,lge->zg,delim);
	fprintf(stdout,"%9.4f%s%9.4f%s%9.4f%s%9.4f\n",lge->rh25,delim,lge->rh50,delim,lge->rh75,delim,lge->rh100);
	// the format for the %f print is:
	//     # total number of digits (including decimal) . # digits after the decimal
     }
}

void print_lgw_column_headers(float dataVersion,int indexcol,char *delim)
{
   int i;
   struct lvis_lgw_v1_00 * lgw100;
   struct lvis_lgw_v1_01 * lgw101;
   struct lvis_lgw_v1_02 * lgw102;
   struct lvis_lgw_v1_03 * lgw103;
   struct lvis_lgw_v1_04 * lgw104;
   
   if(indexcol==1) fprintf(stdout,"%s%s",lvis_index_header_string,delim);
   if(dataVersion == ((float)1.00))
     {
	for(i=0;i<(LVIS_LGW_V1_00_ELEMENTS);i++) 
	  { fprintf(stdout,"%s%s",lvis_lgw_v1_00_header[i],delim); }
	for(i=0;i<(sizeof(lgw100->wave)-1);i++)
	  { fprintf(stdout,"rx%03d%s",i,delim); }
	fprintf(stdout,"rx%03d\n",i);
	
     }
   if(dataVersion == ((float)1.01))
     {
	for(i=0;i<(LVIS_LGW_V1_01_ELEMENTS);i++) 
	  { fprintf(stdout,"%s%s",lvis_lgw_v1_01_header[i],delim); }
	for(i=0;i<(sizeof(lgw101->wave)-1);i++)
	  { fprintf(stdout,"rx%03d%s",i,delim); }
	fprintf(stdout,"rx%03d\n",i);
     }
   if(dataVersion == ((float)1.02))
     {
	for(i=0;i<(LVIS_LGW_V1_02_ELEMENTS);i++) 
	  { fprintf(stdout,"%s%s",lvis_lgw_v1_02_header[i],delim); }
	for(i=0;i<(sizeof(lgw102->wave)-1);i++)
	  { fprintf(stdout,"rx%03d%s",i,delim); }
	fprintf(stdout,"rx%03d\n",i);
     }
   if(dataVersion == ((float)1.03))
     {
	for(i=0;i<(LVIS_LGW_V1_03_ELEMENTS);i++) 
	  { fprintf(stdout,"%s%s",lvis_lgw_v1_03_header[i],delim); }
	for(i=0;i<(sizeof(lgw103->txwave));i++)
	  { fprintf(stdout,"tx%02d%s",i,delim); }
	for(i=0;i<(sizeof(lgw103->rxwave)-1);i++)
	  { fprintf(stdout,"rx%03d%s",i,delim); }
	fprintf(stdout,"rx%03d\n",i);
     }   
   if(dataVersion == ((float)1.04))
     {
	for(i=0;i<(LVIS_LGW_V1_04_ELEMENTS);i++) 
	  { fprintf(stdout,"%s%s",lvis_lgw_v1_04_header[i],delim); }
	for(i=0;i<(sizeof(lgw104->txwave)/sizeof(lgw104->txwave[0]));i++)
	  { fprintf(stdout,"tx%03d%s",i,delim); }
	for(i=0;i<(sizeof(lgw104->rxwave)/sizeof(lgw104->rxwave[0]))-1;i++)
	  { fprintf(stdout,"rx%03d%s",i,delim); }
	fprintf(stdout,"rx%03d\n",i);
     }   
}

void print_lgw_data_v1_00(unsigned char *lgwdata, int indexcol, unsigned int colnum, char * delim,
			  double minlat, double maxlat, double minlon, double maxlon)
{
   int j;
   struct lvis_lgw_v1_00 * lgw;
   lgw = (struct lvis_lgw_v1_00 * ) lgwdata;
   
   if(lgw->lon431>minlon && lgw->lon431<maxlon && lgw->lat431>minlat && lgw->lat431<maxlat)
     {
	if(indexcol==1) fprintf(stdout,"%10i%s",colnum++,delim);
	fprintf(stdout,"%14.10f%s%14.10f%s%9.4f%s",lgw->lon0,delim,lgw->lat0,delim,lgw->z0,delim);
	fprintf(stdout,"%14.10f%s%14.10f%s%9.4f%s",lgw->lon431,delim,lgw->lat431,delim,lgw->z431,delim);
	fprintf(stdout,"%9.4f%s",lgw->sigmean,delim);
	for(j=0;j<sizeof(lgw->wave)-1;j++) fprintf(stdout,"%03d%s",lgw->wave[j],delim);
	fprintf(stdout,"%03d\n",lgw->wave[j]);
     }
}

void print_lgw_data_v1_01(unsigned char *lgwdata, int indexcol, unsigned int colnum, char * delim,
			  double minlat, double maxlat, double minlon, double maxlon)
{
   int j;
   struct lvis_lgw_v1_01 * lgw;
   lgw = (struct lvis_lgw_v1_01 * ) lgwdata;
   
   if(lgw->lon431>minlon && lgw->lon431<maxlon && lgw->lat431>minlat && lgw->lat431<maxlat)
     {
	if(indexcol==1) fprintf(stdout,"%10i%s",colnum++,delim);
	fprintf(stdout,"%u%s%u%s",lgw->lfid,delim,lgw->shotnumber,delim);
	fprintf(stdout,"%14.10f%s%14.10f%s%9.4f%s",lgw->lon0,delim,lgw->lat0,delim,lgw->z0,delim);
	fprintf(stdout,"%14.10f%s%14.10f%s%9.4f%s",lgw->lon431,delim,lgw->lat431,delim,lgw->z431,delim);
	fprintf(stdout,"%9.4f%s",lgw->sigmean,delim);
	for(j=0;j<sizeof(lgw->wave)-1;j++) fprintf(stdout,"%03d%s",lgw->wave[j],delim);
	fprintf(stdout,"%03d\n",lgw->wave[j]);
     }
}

void print_lgw_data_v1_02(unsigned char *lgwdata, int indexcol, unsigned int colnum, char * delim,
			  double minlat, double maxlat, double minlon, double maxlon)
{
   int j;
   struct lvis_lgw_v1_02 * lgw;
   lgw = (struct lvis_lgw_v1_02 * ) lgwdata;
   
   if(lgw->lon431>minlon && lgw->lon431<maxlon && lgw->lat431>minlat && lgw->lat431<maxlat)
     {
	if(indexcol==1) fprintf(stdout,"%10i%s",colnum++,delim);
	fprintf(stdout,"%u%s%u%s%12.6f%s",lgw->lfid,delim,lgw->shotnumber,delim,lgw->lvistime,delim);
	fprintf(stdout,"%14.10f%s%14.10f%s%9.4f%s",lgw->lon0,delim,lgw->lat0,delim,lgw->z0,delim);
	fprintf(stdout,"%14.10f%s%14.10f%s%9.4f%s",lgw->lon431,delim,lgw->lat431,delim,lgw->z431,delim);
	fprintf(stdout,"%9.4f%s",lgw->sigmean,delim);
	for(j=0;j<sizeof(lgw->wave)-1;j++) fprintf(stdout,"%03d%s",lgw->wave[j],delim);
	fprintf(stdout,"%03d\n",lgw->wave[j]);
     }
}

void print_lgw_data_v1_03(unsigned char *lgwdata, int indexcol, unsigned int colnum, char * delim,
			  double minlat, double maxlat, double minlon, double maxlon)
{
   int j;
   struct lvis_lgw_v1_03 * lgw;
   lgw = (struct lvis_lgw_v1_03 * ) lgwdata;
   
   if(lgw->lon431>minlon && lgw->lon431<maxlon && lgw->lat431>minlat && lgw->lat431<maxlat)
     {
	if(indexcol==1) fprintf(stdout,"%10i%s",colnum++,delim);
	fprintf(stdout,"%u%s%u%s",lgw->lfid,delim,lgw->shotnumber,delim);
	fprintf(stdout,"%9.4f%s%9.4f%s%9.4f%s",lgw->azimuth,delim,lgw->incidentangle,delim,lgw->range,delim);
	fprintf(stdout,"%12.6f%s",lgw->lvistime,delim);
	fprintf(stdout,"%14.10f%s%14.10f%s%9.4f%s",lgw->lon0,delim,lgw->lat0,delim,lgw->z0,delim);
	fprintf(stdout,"%14.10f%s%14.10f%s%9.4f%s",lgw->lon431,delim,lgw->lat431,delim,lgw->z431,delim);
	fprintf(stdout,"%9.4f%s",lgw->sigmean,delim);
	for(j=0;j<sizeof(lgw->txwave)-1;j++) fprintf(stdout,"%03d%s",lgw->txwave[j],delim);
	fprintf(stdout,"%03d\n",lgw->txwave[j]);
	for(j=0;j<sizeof(lgw->rxwave)-1;j++) fprintf(stdout,"%03d%s",lgw->rxwave[j],delim);
	fprintf(stdout,"%03d\n",lgw->rxwave[j]);
     }
}

void print_lgw_data_v1_04(unsigned char *lgwdata, int indexcol, unsigned int colnum, char * delim,
			  double minlat, double maxlat, double minlon, double maxlon)
{
   int j,myendian;
   struct lvis_lgw_v1_04 * lgw;
   lgw = (struct lvis_lgw_v1_04 * ) lgwdata;
   
   myendian = host_endian();
   
   if(lgw->lon527>minlon && lgw->lon527<maxlon && lgw->lat527>minlat && lgw->lat527<maxlat)
     {
	if(indexcol==1) fprintf(stdout,"%10i%s",colnum++,delim);
	fprintf(stdout,"%u%s%u%s",lgw->lfid,delim,lgw->shotnumber,delim);
	fprintf(stdout,"%9.4f%s%9.4f%s%9.4f%s",lgw->azimuth,delim,lgw->incidentangle,delim,lgw->range,delim);
	fprintf(stdout,"%12.6f%s",lgw->lvistime,delim);
	fprintf(stdout,"%14.10f%s%14.10f%s%9.4f%s",lgw->lon0,delim,lgw->lat0,delim,lgw->z0,delim);
	fprintf(stdout,"%14.10f%s%14.10f%s%9.4f%s",lgw->lon527,delim,lgw->lat527,delim,lgw->z527,delim);
	fprintf(stdout,"%9.4f%s",lgw->sigmean,delim);
	for(j=0;j<(sizeof(lgw->txwave)/sizeof(lgw->txwave[0]))-1;j++) fprintf(stdout,"%04d%s",host_word(lgw->txwave[j],myendian),delim);
	fprintf(stdout,"%04d\n",host_word(lgw->txwave[j],myendian));
	for(j=0;j<(sizeof(lgw->rxwave)/sizeof(lgw->rxwave[0]))-1;j++) fprintf(stdout,"%04d%s",host_word(lgw->rxwave[j],myendian),delim);
	fprintf(stdout,"%04d\n",host_word(lgw->rxwave[j],myendian));
     }
}

void print_lce_data
  (unsigned char *lcedata, float dataVersion, int indexcol, unsigned int colnum, char * delim,
   double minlat, double maxlat, double minlon, double maxlon)
{
   if(dataVersion == ((float)1.00)) print_lce_data_v1_00(lcedata,indexcol,colnum,delim,
						minlat,maxlat,minlon,maxlon);
   if(dataVersion == ((float)1.01)) print_lce_data_v1_01(lcedata,indexcol,colnum,delim,
						minlat,maxlat,minlon,maxlon);
   if(dataVersion == ((float)1.02)) print_lce_data_v1_02(lcedata,indexcol,colnum,delim,
						minlat,maxlat,minlon,maxlon);
   if(dataVersion == ((float)1.03)) print_lce_data_v1_03(lcedata,indexcol,colnum,delim,
						minlat,maxlat,minlon,maxlon);
}

void print_lge_data
  (unsigned char *lgedata, float dataVersion, int indexcol, unsigned int colnum, char * delim,
   double minlat, double maxlat, double minlon, double maxlon)
{
   if(dataVersion == ((float)1.00)) print_lge_data_v1_00(lgedata,indexcol,colnum,delim,
						minlat,maxlat,minlon,maxlon);
   if(dataVersion == ((float)1.01)) print_lge_data_v1_01(lgedata,indexcol,colnum,delim,
						minlat,maxlat,minlon,maxlon);
   if(dataVersion == ((float)1.02)) print_lge_data_v1_02(lgedata,indexcol,colnum,delim,
						minlat,maxlat,minlon,maxlon);
   if(dataVersion == ((float)1.03)) print_lge_data_v1_03(lgedata,indexcol,colnum,delim,
						minlat,maxlat,minlon,maxlon);
}

void print_lgw_data
  (unsigned char *lgwdata, float dataVersion, int indexcol, unsigned int colnum, char * delim,
   double minlat, double maxlat, double minlon, double maxlon)
{
   if(dataVersion == ((float)1.00)) print_lgw_data_v1_00(lgwdata,indexcol,colnum,delim,
						minlat,maxlat,minlon,maxlon);
   if(dataVersion == ((float)1.01)) print_lgw_data_v1_01(lgwdata,indexcol,colnum,delim,
						minlat,maxlat,minlon,maxlon);
   if(dataVersion == ((float)1.02)) print_lgw_data_v1_02(lgwdata,indexcol,colnum,delim,
						minlat,maxlat,minlon,maxlon);
   if(dataVersion == ((float)1.03)) print_lgw_data_v1_03(lgwdata,indexcol,colnum,delim,
						minlat,maxlat,minlon,maxlon);
   if(dataVersion == ((float)1.04)) print_lgw_data_v1_04(lgwdata,indexcol,colnum,delim,
						minlat,maxlat,minlon,maxlon);
}

void swap_lce_data_v1_00(unsigned char *lcedata, int myendian)
{
   struct lvis_lce_v1_00 * lce;
   lce = (struct lvis_lce_v1_00 * ) lcedata;

   lce->tlon        = host_double(lce->tlon,myendian);
   lce->tlat        = host_double(lce->tlat,myendian);
   lce->zt          = host_float(lce->zt,myendian);
}

void swap_lce_data_v1_01(unsigned char *lcedata, int myendian)
{
   struct lvis_lce_v1_01 * lce;
   lce = (struct lvis_lce_v1_01 * ) lcedata;
   
   lce->lfid        = host_dword(lce->lfid,myendian);
   lce->shotnumber  = host_dword(lce->shotnumber,myendian);
   lce->tlon        = host_double(lce->tlon,myendian);
   lce->tlat        = host_double(lce->tlat,myendian);
   lce->zt          = host_float(lce->zt,myendian);
}

void swap_lce_data_v1_02(unsigned char *lcedata, int myendian)
{
   struct lvis_lce_v1_02 * lce;
   lce = (struct lvis_lce_v1_02 * ) lcedata;
   
   lce->lfid        = host_dword(lce->lfid,myendian);
   lce->shotnumber  = host_dword(lce->shotnumber,myendian);
   lce->lvistime    = host_double(lce->lvistime,myendian);
   lce->tlon        = host_double(lce->tlon,myendian);
   lce->tlat        = host_double(lce->tlat,myendian);
   lce->zt          = host_float(lce->zt,myendian);
}

void swap_lce_data_v1_03(unsigned char *lcedata, int myendian)
{
   struct lvis_lce_v1_03 * lce;
   lce = (struct lvis_lce_v1_03 * ) lcedata;
   
   lce->lfid          = host_dword(lce->lfid,myendian);
   lce->shotnumber    = host_dword(lce->shotnumber,myendian);
   lce->azimuth       = host_float(lce->azimuth,myendian);
   lce->incidentangle = host_float(lce->incidentangle,myendian);
   lce->range         = host_float(lce->range,myendian);
   lce->lvistime      = host_double(lce->lvistime,myendian);
   lce->tlon          = host_double(lce->tlon,myendian);
   lce->tlat          = host_double(lce->tlat,myendian);
   lce->zt            = host_float(lce->zt,myendian);
}

void swap_lge_data_v1_00(unsigned char *lgedata, int myendian)
{
   struct lvis_lge_v1_00 * lge;
   lge = (struct lvis_lge_v1_00 * ) lgedata;
   
   lge->glon        = host_double(lge->glon,myendian);
   lge->glat        = host_double(lge->glat,myendian);
   lge->zg          = host_float(lge->zg,myendian);
   lge->rh25        = host_float(lge->rh25,myendian);
   lge->rh50        = host_float(lge->rh50,myendian);
   lge->rh75        = host_float(lge->rh75,myendian);
   lge->rh100       = host_float(lge->rh100,myendian);   
}

void swap_lge_data_v1_01(unsigned char *lgedata, int myendian)
{
   struct lvis_lge_v1_01 * lge;
   lge = (struct lvis_lge_v1_01 * ) lgedata;
   
   lge->lfid        = host_dword(lge->lfid,myendian);
   lge->shotnumber  = host_dword(lge->shotnumber,myendian);
   lge->glon        = host_double(lge->glon,myendian);
   lge->glat        = host_double(lge->glat,myendian);
   lge->zg          = host_float(lge->zg,myendian);
   lge->rh25        = host_float(lge->rh25,myendian);
   lge->rh50        = host_float(lge->rh50,myendian);
   lge->rh75        = host_float(lge->rh75,myendian);
   lge->rh100       = host_float(lge->rh100,myendian);   
}

void swap_lge_data_v1_02(unsigned char *lgedata, int myendian)
{
   struct lvis_lge_v1_02 * lge;
   lge = (struct lvis_lge_v1_02 * ) lgedata;
   
   lge->lfid        = host_dword(lge->lfid,myendian);
   lge->shotnumber  = host_dword(lge->shotnumber,myendian);
   lge->lvistime    = host_double(lge->lvistime,myendian);
   lge->glon        = host_double(lge->glon,myendian);
   lge->glat        = host_double(lge->glat,myendian);
   lge->zg          = host_float(lge->zg,myendian);
   lge->rh25        = host_float(lge->rh25,myendian);
   lge->rh50        = host_float(lge->rh50,myendian);
   lge->rh75        = host_float(lge->rh75,myendian);
   lge->rh100       = host_float(lge->rh100,myendian);   
}

void swap_lge_data_v1_03(unsigned char *lgedata, int myendian)
{
   struct lvis_lge_v1_03 * lge;
   lge = (struct lvis_lge_v1_03 * ) lgedata;
   
   lge->lfid          = host_dword(lge->lfid,myendian);
   lge->shotnumber    = host_dword(lge->shotnumber,myendian);
   lge->azimuth       = host_float(lge->azimuth,myendian);
   lge->incidentangle = host_float(lge->incidentangle,myendian);
   lge->range         = host_float(lge->range,myendian);
   lge->lvistime      = host_double(lge->lvistime,myendian);
   lge->glon          = host_double(lge->glon,myendian);
   lge->glat          = host_double(lge->glat,myendian);
   lge->zg            = host_float(lge->zg,myendian);
   lge->rh25          = host_float(lge->rh25,myendian);
   lge->rh50          = host_float(lge->rh50,myendian);
   lge->rh75          = host_float(lge->rh75,myendian);
   lge->rh100         = host_float(lge->rh100,myendian);   
}

void swap_lgw_data_v1_00(unsigned char *lgwdata, int myendian)
{
   struct lvis_lgw_v1_00 * lgw;
   lgw = (struct lvis_lgw_v1_00 * ) lgwdata;
   
   lgw->lon0        = host_double(lgw->lon0,myendian);
   lgw->lat0        = host_double(lgw->lat0,myendian);
   lgw->z0          = host_float(lgw->z0,myendian);
   lgw->lon431      = host_double(lgw->lon431,myendian);
   lgw->lat431      = host_double(lgw->lat431,myendian);
   lgw->z431        = host_float(lgw->z431,myendian);
   lgw->sigmean     = host_float(lgw->sigmean,myendian);
}

void swap_lgw_data_v1_01(unsigned char *lgwdata, int myendian)
{
   struct lvis_lgw_v1_01 * lgw;
   lgw = (struct lvis_lgw_v1_01 * ) lgwdata;
   
   lgw->lfid        = host_dword(lgw->lfid,myendian);
   lgw->shotnumber  = host_dword(lgw->shotnumber,myendian);
   lgw->lon0        = host_double(lgw->lon0,myendian);
   lgw->lat0        = host_double(lgw->lat0,myendian);
   lgw->z0          = host_float(lgw->z0,myendian);
   lgw->lon431      = host_double(lgw->lon431,myendian);
   lgw->lat431      = host_double(lgw->lat431,myendian);
   lgw->z431        = host_float(lgw->z431,myendian);
   lgw->sigmean     = host_float(lgw->sigmean,myendian);
}

void swap_lgw_data_v1_02(unsigned char *lgwdata, int myendian)
{
   struct lvis_lgw_v1_02 * lgw;
   lgw = (struct lvis_lgw_v1_02 * ) lgwdata;
   
   lgw->lfid        = host_dword(lgw->lfid,myendian);
   lgw->shotnumber  = host_dword(lgw->shotnumber,myendian);
   lgw->lvistime    = host_double(lgw->lvistime,myendian);
   lgw->lon0        = host_double(lgw->lon0,myendian);
   lgw->lat0        = host_double(lgw->lat0,myendian);
   lgw->z0          = host_float(lgw->z0,myendian);
   lgw->lon431      = host_double(lgw->lon431,myendian);
   lgw->lat431      = host_double(lgw->lat431,myendian);
   lgw->z431        = host_float(lgw->z431,myendian);
   lgw->sigmean     = host_float(lgw->sigmean,myendian);
}

void swap_lgw_data_v1_03(unsigned char *lgwdata, int myendian)
{
   struct lvis_lgw_v1_03 * lgw;
   lgw = (struct lvis_lgw_v1_03 * ) lgwdata;
   
   lgw->lfid          = host_dword(lgw->lfid,myendian);
   lgw->shotnumber    = host_dword(lgw->shotnumber,myendian);
   lgw->azimuth       = host_float(lgw->azimuth,myendian);
   lgw->incidentangle = host_float(lgw->incidentangle,myendian);
   lgw->range         = host_float(lgw->range,myendian);
   lgw->lvistime      = host_double(lgw->lvistime,myendian);
   lgw->lon0          = host_double(lgw->lon0,myendian);
   lgw->lat0          = host_double(lgw->lat0,myendian);
   lgw->z0            = host_float(lgw->z0,myendian);
   lgw->lon431        = host_double(lgw->lon431,myendian);
   lgw->lat431        = host_double(lgw->lat431,myendian);
   lgw->z431          = host_float(lgw->z431,myendian);
   lgw->sigmean       = host_float(lgw->sigmean,myendian);
}

void swap_lgw_data_v1_04(unsigned char *lgwdata, int myendian)
{
   struct lvis_lgw_v1_04 * lgw;
   lgw = (struct lvis_lgw_v1_04 * ) lgwdata;
   
   lgw->lfid          = host_dword(lgw->lfid,myendian);
   lgw->shotnumber    = host_dword(lgw->shotnumber,myendian);
   lgw->azimuth       = host_float(lgw->azimuth,myendian);
   lgw->incidentangle = host_float(lgw->incidentangle,myendian);
   lgw->range         = host_float(lgw->range,myendian);
   lgw->lvistime      = host_double(lgw->lvistime,myendian);
   lgw->lon0          = host_double(lgw->lon0,myendian);
   lgw->lat0          = host_double(lgw->lat0,myendian);
   lgw->z0            = host_float(lgw->z0,myendian);
   lgw->lon527        = host_double(lgw->lon527,myendian);
   lgw->lat527        = host_double(lgw->lat527,myendian);
   lgw->z527          = host_float(lgw->z527,myendian);
   lgw->sigmean       = host_float(lgw->sigmean,myendian);
}

void swap_lce_data(unsigned char *lcedata, float dataVersion, int myendian)
{
   if(dataVersion == ((float)1.00)) swap_lce_data_v1_00(lcedata,myendian);
   if(dataVersion == ((float)1.01)) swap_lce_data_v1_01(lcedata,myendian);
   if(dataVersion == ((float)1.02)) swap_lce_data_v1_02(lcedata,myendian);
   if(dataVersion == ((float)1.03)) swap_lce_data_v1_03(lcedata,myendian);
}

void swap_lge_data(unsigned char *lgedata, float dataVersion, int myendian)
{
   if(dataVersion == ((float)1.00)) swap_lge_data_v1_00(lgedata,myendian);
   if(dataVersion == ((float)1.01)) swap_lge_data_v1_01(lgedata,myendian);
   if(dataVersion == ((float)1.02)) swap_lge_data_v1_02(lgedata,myendian);
   if(dataVersion == ((float)1.03)) swap_lge_data_v1_03(lgedata,myendian);
}

void swap_lgw_data(unsigned char *lgwdata, float dataVersion, int myendian)
{
   if(dataVersion == ((float)1.00)) swap_lgw_data_v1_00(lgwdata,myendian);
   if(dataVersion == ((float)1.01)) swap_lgw_data_v1_01(lgwdata,myendian);
   if(dataVersion == ((float)1.02)) swap_lgw_data_v1_02(lgwdata,myendian);
   if(dataVersion == ((float)1.03)) swap_lgw_data_v1_03(lgwdata,myendian);
   if(dataVersion == ((float)1.04)) swap_lgw_data_v1_04(lgwdata,myendian);
}

void display_usage(char * proggy)
{
   fprintf(stdout,"USAGE: %s <input> [options]\n",proggy);
   fprintf(stdout,"\n");
   fprintf(stdout,"-c                    Delimit data with commas (default = TAB)\n");
   fprintf(stdout,"-endianbig            Force the software to assume system is BIG Endian\n");
   fprintf(stdout,"-endianlittle         Force the software to assume system is LITTLE Endian\n");
   fprintf(stdout,"-h                    Display this help / usage menu\n");
   fprintf(stdout,"-i                    Generate an index column\n");
   fprintf(stdout,"-lat dd.dddd-dd.dddd  Cut by latitude  (min -> max decimal degrees)\n");
   fprintf(stdout,"-lon dd.dddd-dd.dddd  Cut by longitude (min -> max decimal degrees)\n");
   fprintf(stdout,"-lce                  Force file type to LCE (normally chooses by file extension)\n");
   fprintf(stdout,"-lge                  Force file type to LGE\n");
   fprintf(stdout,"-lgw                  Force file type to LGW\n");
   fprintf(stdout,"-n N                  Number of samples to read (1000 for example)\n");
   fprintf(stdout,"-r V.VV               Force version to release version V.VV (1.02 for example)\n");
   fprintf(stdout,"-t                    Top each column with a header\n");
   fprintf(stdout,"-v                    Print program version and exit\n");
   fprintf(stdout,"\n");
   fprintf(stdout,"\n");
   
}

int main(int argc, char *argv[])
{
   long          sampleNumber,maxSampleNumber;
   float         dataReleaseVersion,tempVersion;
   double        minlon,maxlon,minlat,maxlat;
   unsigned char lcedata[1024],lgedata[1024],lgwdata[2048];
   char          filename[1024],delim[16],temp[1024],tempa[1024],tempb[1024];
   int           i,j,myendian,filetype,indexcol,topcol,keepReading;
   int           lcesize=0,lgesize=0,lgwsize=0;
   unsigned int  colnum;
   
   FILE *fp;
   // set up variable defaults
   minlat = -400.0; maxlat = 400.0;
   minlon = -400.0; maxlon = 400.0;
   memset(delim,0,sizeof(delim));
   strncpy(delim,"\t",1);
   indexcol=0; // display an index column?
   topcol=0;   // display column headers? 
   maxSampleNumber = 0; // upper limit on sample number (0 means no limit)
   sampleNumber = 0;    // what sample are we on in the file?
   keepReading = 1;     // simple go / no go flag
   dataReleaseVersion = -1.0;
   filetype = -1;
   
   // check size of variable types and exit if not as expected
   if(sizeof(double) != 8)
     { 
	fprintf(stderr, "double variable is not expected size (8), exiting...\n");
	fprintf(stderr, "please report this to the author (http://lvis.gsfc.nasa.gov)\n");
	exit(16);
     }
   if(sizeof(float) != 4)
     { 
	fprintf(stderr, "float variable is not expected size (4), exiting...\n");
	fprintf(stderr, "please report this to the author (http://lvis.gsfc.nasa.gov)\n");
	exit(17);
     }
   if(sizeof(int) != 4)
     { 
	fprintf(stderr, "int variable is not expected size (4), exiting...\n");
	fprintf(stderr, "please report this to the author (http://lvis.gsfc.nasa.gov)\n");
	exit(18);
     }
   
   // figure out what endian this host machine is
   myendian = host_endian();
   
   // less than 2 arguments? we need at least an input file name
   if(argc<2)
     {
	display_usage(argv[0]);
	exit(-1);
     }

   // check for a version flag
   strcpy(temp,argv[1]);
   strcpy((char *) filename,argv[1]);
   if(strncmp(temp,"-v",2)==0)
     { 
	minlon = LVIS_RELEASE_READER_VERSION;
	maxlon = LVIS_RELEASE_READER_VERSION_DATE;
	fprintf(stdout,"LVIS Release Reader Version %5.2f (%u)\n",minlon,(dword) maxlon);
	minlon = LVIS_RELEASE_STRUCTURE_VERSION;
	maxlon = LVIS_RELEASE_STRUCTURE_DATE;
	fprintf(stdout,"LVIS Release Data Structures supported up to and including version %5.2f (%u)\n",minlon,(dword) maxlon);
	exit(1);
     }
   if(strncmp(temp,"-h",2)==0 || strncmp(temp,"-H",2)==0 || strncmp(temp,"-?",2)==0)
     { 
	display_usage(argv[0]);
	exit(2);
     }
      
   // open up the file for read access
   if((fp = fopen((char *)filename,"rb"))==NULL)
     {
	fprintf(stderr,"Error opening the input file: %s\n",filename);
	exit(-1);
     }
   
   // check for command line arguments
   i=2;  // set a pointer at arguement 2 of the command line
   while(i<argc)
     {
	strcpy(temp,argv[i]);
	if(strncmp(temp,"-v",2)==0)
	  { 
	     minlon = LVIS_RELEASE_READER_VERSION;
	     fprintf(stdout,"LVIS (Land, Vegetation and Ice Sensor) Release Reader Version %5.2f\n",minlon);
	     exit(1);
	  }
	if(strncmp(temp,"-c",2)==0)
	  { strncpy(delim,",",1);}
	if(strncmp(temp,"-endianbig",10)==0)
	  { myendian = GENLIB_BIG_ENDIAN;}	  
	if(strncmp(temp,"-endianlittle",13)==0)
	  { myendian = GENLIB_LITTLE_ENDIAN;}	  
	if(strncmp(temp,"-h",2)==0 || strncmp(temp,"-H",2)==0 || strncmp(temp,"-?",2)==0)
	  { 
	     display_usage(argv[0]);
	     exit(2);
	  }
	if(strncmp(temp,"-i",2)==0)
	  { indexcol=1; }
	if(strncmp(temp,"-lce",4)==0)
	  { filetype = LVIS_RELEASE_FILETYPE_LCE; }
	if(strncmp(temp,"-lge",4)==0)
	  { filetype = LVIS_RELEASE_FILETYPE_LGE; }
	if(strncmp(temp,"-lgw",4)==0) 
	  { filetype = LVIS_RELEASE_FILETYPE_LGW; }
	if(strncmp(temp,"-lat",4)==0)
	  {
	     i++;
	     if(i<argc) 
	       {	  
		  strcpy(tempa,argv[i]);
		  // find the '-' character in the string
		  j=0; // j will be our pointer
		  while(j++<strlen(tempa) && tempa[j]!='-'); 
		  if(j>=strlen(tempa)) fprintf(stderr,"Invalid argument to -lat parameter!\n");
		  else
		    {  // split the string around j into min/max
		       memset(tempb,0,sizeof(tempb));
		       strncpy(tempb,&tempa[0],j);
		       minlat = atof(tempb);
		       memset(tempb,0,sizeof(tempb));
		       strncpy(tempb,&tempa[j+1],strlen(tempa)-j);
		       maxlat = atof(tempb);
		    }
	       }
	  }
	if(strncmp(temp,"-lon",4)==0)
	  {
	     i++;
	     if(i<argc) 
	       {	  
		  strcpy(tempa,argv[i]);
		  // find the '-' character in the string
		  j=0;
		  while(j++<strlen(tempa) && tempa[j]!='-');
		  if(j>=strlen(tempa)) fprintf(stderr,"Invalid argument to -lon parameter!\n");
		  else
		    {
		       memset(tempb,0,sizeof(tempb));
		       strncpy(tempb,&tempa[0],j);
		       minlon = atof(tempb);
		       memset(tempb,0,sizeof(tempb));
		       strncpy(tempb,&tempa[j+1],strlen(tempa)-j);
		       maxlon = atof(tempb);
		    }
	       }
	  }
	if(strncmp(temp,"-n",2)==0)  // number of samples
	  {
	     i++;
	     if(i<argc) 
	       {	  
		  strcpy(tempa,argv[i]);
		  maxSampleNumber = atol(tempa);
		  if(maxSampleNumber < 0) maxSampleNumber = 0; // do not allow negative values
	       }
	  }
	if(strncmp(temp,"-r",2)==0)  // release version force flag
	  {
	     i++;
	     if(i<argc) 
	       {	  
		  strcpy(tempa,argv[i]);
		  tempVersion = atof(tempa);
		  if(tempVersion >= 1.0 && tempVersion <= LVIS_RELEASE_READER_VERSION)
		    dataReleaseVersion = tempVersion;
	       }
	  }
	if(strncmp(temp,"-t",2)==0)
	  { topcol=1; }
	i++;
     }

   // before doing anything, test the file and take a guess what it is
   // but do this after the flag checking (incase endian is set)
   // ONLY check if these have not been set on the command line (forcing the issue)
   if(filetype < 0 && dataReleaseVersion < 0)  // are set negative by default
     detect_release_version(filename,&filetype,&dataReleaseVersion,myendian);
   
   // allocate our memory for the data structure depending on the release version
   if(dataReleaseVersion == ((float)1.00))
     {
	lcesize = sizeof(struct lvis_lce_v1_00);
	lgesize = sizeof(struct lvis_lge_v1_00);
	lgwsize = sizeof(struct lvis_lgw_v1_00);
     }
   if(dataReleaseVersion == ((float)1.01))
     {
	lcesize = sizeof(struct lvis_lce_v1_01);
	lgesize = sizeof(struct lvis_lge_v1_01);
	lgwsize = sizeof(struct lvis_lgw_v1_01);
     }
   if(dataReleaseVersion == ((float) 1.02))
     {
	lcesize = sizeof(struct lvis_lce_v1_02);
	lgesize = sizeof(struct lvis_lge_v1_02);
	lgwsize = sizeof(struct lvis_lgw_v1_02);
     }
   if(dataReleaseVersion == ((float) 1.03))
     {
	lcesize = sizeof(struct lvis_lce_v1_03);
	lgesize = sizeof(struct lvis_lge_v1_03);
	lgwsize = sizeof(struct lvis_lgw_v1_03);
     }   
   if(dataReleaseVersion == ((float) 1.04))
     {
	lcesize = sizeof(struct lvis_lce_v1_04);
	lgesize = sizeof(struct lvis_lge_v1_04);
	lgwsize = sizeof(struct lvis_lgw_v1_04);
     }   
   
#ifdef DEBUG_ON  // uncomment the #define up top if you want to see these messages
   fprintf(stdout,"input file name = %s (strlen=%d)\n",(char *)filename,(int)strlen((char *)filename));
   fprintf(stdout,"input file type = %d\n",filetype);
   fprintf(stdout,"lce structure size = %d bytes\n",lcesize);
   fprintf(stdout,"lge structure size = %d bytes\n",lgesize);
   fprintf(stdout,"lgw structure size = %d bytes\n",lgwsize);
   fprintf(stdout,"data release version used = %f\n",dataReleaseVersion);
   if(myendian == GENLIB_LITTLE_ENDIAN) fprintf(stdout,"system is LITTLE ENDIAN.\n");
   if(myendian == GENLIB_BIG_ENDIAN) fprintf(stdout,"system is BIG ENDIAN.\n");
#endif
   
   colnum = 1;  // set the index column counter to one (1)

   switch(filetype)
     {
	
      case LVIS_RELEASE_FILETYPE_LCE:
	if(topcol == 1) print_lce_column_headers(dataReleaseVersion,indexcol,delim);
	while(fread(lcedata,lcesize,1,fp)==1 && keepReading==1) 
	  {
	     // swap each item of this block if necessary
	     swap_lce_data(lcedata,dataReleaseVersion,myendian);
	     print_lce_data(lcedata,dataReleaseVersion,indexcol,colnum++,delim,
			    minlat,maxlat,minlon,maxlon);
	     sampleNumber++;
	     if(maxSampleNumber != 0 && sampleNumber >= maxSampleNumber) keepReading = 0;
	  }
	break;

      case LVIS_RELEASE_FILETYPE_LGE:
	if(topcol == 1) print_lge_column_headers(dataReleaseVersion,indexcol,delim);
	while(fread(lgedata,lgesize,1,fp)==1 && keepReading==1)
	  {
	     // swap each item of this block if necessary
	     swap_lge_data(lgedata,dataReleaseVersion,myendian);
	     print_lge_data(lgedata,dataReleaseVersion,indexcol,colnum++,delim,
			    minlat,maxlat,minlon,maxlon);
	     sampleNumber++;
	     if(maxSampleNumber != 0 && sampleNumber >= maxSampleNumber) keepReading = 0;
	  }
	break;
	
      case LVIS_RELEASE_FILETYPE_LGW:
	if(topcol == 1) print_lgw_column_headers(dataReleaseVersion,indexcol,delim);
	while(fread(lgwdata,lgwsize,1,fp)==1 && keepReading==1) 
	  {
	     // swap each item of this block if necessary
	     swap_lgw_data(lgwdata,dataReleaseVersion,myendian);
	     print_lgw_data(lgwdata,dataReleaseVersion,indexcol,colnum++,delim,
			    minlat,maxlat,minlon,maxlon);
	     sampleNumber++;
	     if(maxSampleNumber != 0 && sampleNumber >= maxSampleNumber) keepReading = 0;
	  }
	break;
	
      default:
	break;
     }
   
   if(fp!=NULL) fclose(fp);
   return(1);
}
