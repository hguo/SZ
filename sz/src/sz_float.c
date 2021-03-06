/**
 *  @file sz_float.c
 *  @author Sheng Di and Dingwen Tao
 *  @date Aug, 2016
 *  @brief SZ_Init, Compression and Decompression functions
 *  (C) 2016 by Mathematics and Computer Science (MCS), Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include "sz.h"
#include "CompressElement.h"
#include "DynamicByteArray.h"
#include "DynamicIntArray.h"
#include "TightDataPointStorageF.h"
#include "sz_float.h"
#include "sz_float_pwr.h"
#include "szd_float.h"
#include "szd_float_pwr.h"
#include "zlib.h"
#include "rw.h"
#include "sz_float_ts.h"

unsigned char* SZ_skip_compress_float(float* data, size_t dataLength, size_t* outSize)
{
	*outSize = dataLength*sizeof(float);
	unsigned char* out = (unsigned char*)malloc(dataLength*sizeof(float));
	memcpy(out, data, dataLength*sizeof(float));
	return out;
}
unsigned int optimize_intervals_float_1D(float *oriData, size_t dataLength, double realPrecision)
{	
	size_t i = 0, radiusIndex;
	float pred_value = 0, pred_err;
	size_t *intervals = (size_t*)malloc(conf_params->maxRangeRadius*sizeof(size_t));
	memset(intervals, 0, conf_params->maxRangeRadius*sizeof(size_t));
	size_t totalSampleSize = dataLength/conf_params->sampleDistance;
	for(i=2;i<dataLength;i++)
	{
		if(i%conf_params->sampleDistance==0)
		{
			//pred_value = 2*oriData[i-1] - oriData[i-2];
			pred_value = oriData[i-1];
			pred_err = fabs(pred_value - oriData[i]);
			radiusIndex = (unsigned long)((pred_err/realPrecision+1)/2);
			if(radiusIndex>=conf_params->maxRangeRadius)
				radiusIndex = conf_params->maxRangeRadius - 1;			
			intervals[radiusIndex]++;
		}
	}
	//compute the appropriate number
	size_t targetCount = totalSampleSize*conf_params->predThreshold;
	size_t sum = 0;
	for(i=0;i<conf_params->maxRangeRadius;i++)
	{
		sum += intervals[i];
		if(sum>targetCount)
			break;
	}
	if(i>=conf_params->maxRangeRadius)
		i = conf_params->maxRangeRadius-1;
		
	unsigned int accIntervals = 2*(i+1);
	unsigned int powerOf2 = roundUpToPowerOf2(accIntervals);
	
	if(powerOf2<32)
		powerOf2 = 32;
	
	free(intervals);
	//printf("accIntervals=%d, powerOf2=%d\n", accIntervals, powerOf2);
	return powerOf2;
}

unsigned int optimize_intervals_float_2D(float *oriData, size_t r1, size_t r2, double realPrecision)
{	
	size_t i,j, index;
	size_t radiusIndex;
	float pred_value = 0, pred_err;
	size_t *intervals = (size_t*)malloc(conf_params->maxRangeRadius*sizeof(size_t));
	memset(intervals, 0, conf_params->maxRangeRadius*sizeof(size_t));
	size_t totalSampleSize = (r1-1)*(r2-1)/conf_params->sampleDistance;

	//float max = oriData[0];
	//float min = oriData[0];

	for(i=1;i<r1;i++)
	{
		for(j=1;j<r2;j++)
		{
			if((i+j)%conf_params->sampleDistance==0)
			{
				index = i*r2+j;
				pred_value = oriData[index-1] + oriData[index-r2] - oriData[index-r2-1];
				pred_err = fabs(pred_value - oriData[index]);
				radiusIndex = (unsigned long)((pred_err/realPrecision+1)/2);
				if(radiusIndex>=conf_params->maxRangeRadius)
					radiusIndex = conf_params->maxRangeRadius - 1;
				intervals[radiusIndex]++;

			//	if (max < oriData[index]) max = oriData[index];
			//	if (min > oriData[index]) min = oriData[index];
			}			
		}
	}
	//compute the appropriate number
	size_t targetCount = totalSampleSize*conf_params->predThreshold;
	size_t sum = 0;
	for(i=0;i<conf_params->maxRangeRadius;i++)
	{
		sum += intervals[i];
		if(sum>targetCount)
			break;
	}
	if(i>=conf_params->maxRangeRadius)
		i = conf_params->maxRangeRadius-1;
	unsigned int accIntervals = 2*(i+1);
	unsigned int powerOf2 = roundUpToPowerOf2(accIntervals);

	if(powerOf2<32)
		powerOf2 = 32;

	//	struct timeval costStart, costEnd;
	//	double cost_est = 0;
	//
	//	gettimeofday(&costStart, NULL);
	//
	//	//compute estimate of bit-rate and distortion
	//	double est_br = 0;
	//	double est_psnr = 0;
	//	double c1 = log2(targetCount)+1;
	//	double c2 = -20.0*log10(realPrecision) + 20.0*log10(max-min) + 10.0*log10(3);
	//
	//	for (i = 0; i < powerOf2/2; i++)
	//	{
	//		int count = intervals[i];
	//		if (count != 0)
	//			est_br += count*log2(count);
	//		est_psnr += count;
	//	}
	//
	//	//compute estimate of bit-rate
	//	est_br -= c1*est_psnr;
	//	est_br /= totalSampleSize;
	//	est_br = -est_br;
	//
	//	//compute estimate of psnr
	//	est_psnr /= totalSampleSize;
	//	printf ("sum of P(i) = %lf\n", est_psnr);
	//	est_psnr = -10.0*log10(est_psnr);
	//	est_psnr += c2;
	//
	//	printf ("estimate bitrate = %.2f\n", est_br);
	//	printf ("estimate psnr = %.2f\n",est_psnr);
	//
	//	gettimeofday(&costEnd, NULL);
	//	cost_est = ((costEnd.tv_sec*1000000+costEnd.tv_usec)-(costStart.tv_sec*1000000+costStart.tv_usec))/1000000.0;
	//
	//	printf ("analysis time = %f\n", cost_est);

	free(intervals);
	//printf("conf_params->maxRangeRadius = %d, accIntervals=%d, powerOf2=%d\n", conf_params->maxRangeRadius, accIntervals, powerOf2);
	return powerOf2;
}

unsigned int optimize_intervals_float_3D(float *oriData, size_t r1, size_t r2, size_t r3, double realPrecision)
{	
	size_t i,j,k, index;
	size_t radiusIndex;
	size_t r23=r2*r3;
	float pred_value = 0, pred_err;
	size_t *intervals = (size_t*)malloc(conf_params->maxRangeRadius*sizeof(size_t));
	memset(intervals, 0, conf_params->maxRangeRadius*sizeof(size_t));
	size_t totalSampleSize = (r1-1)*(r2-1)*(r3-1)/conf_params->sampleDistance;

	//float max = oriData[0];
	//float min = oriData[0];

	for(i=1;i<r1;i++)
	{
		for(j=1;j<r2;j++)
		{
			for(k=1;k<r3;k++)
			{			
				if((i+j+k)%conf_params->sampleDistance==0)
				{
					index = i*r23+j*r3+k;
					pred_value = oriData[index-1] + oriData[index-r3] + oriData[index-r23] 
					- oriData[index-1-r23] - oriData[index-r3-1] - oriData[index-r3-r23] + oriData[index-r3-r23-1];
					pred_err = fabs(pred_value - oriData[index]);
					radiusIndex = (pred_err/realPrecision+1)/2;
					if(radiusIndex>=conf_params->maxRangeRadius)
					{
						radiusIndex = conf_params->maxRangeRadius - 1;
						//printf("radiusIndex=%d\n", radiusIndex);
					}
					intervals[radiusIndex]++;

					//	if (max < oriData[index]) max = oriData[index];
					//	if (min > oriData[index]) min = oriData[index];
				}
			}
		}
	}
	//compute the appropriate number
	size_t targetCount = totalSampleSize*conf_params->predThreshold;
	size_t sum = 0;
	for(i=0;i<conf_params->maxRangeRadius;i++)
	{
		sum += intervals[i];
		if(sum>targetCount)
			break;
	}
	if(i>=conf_params->maxRangeRadius)
		i = conf_params->maxRangeRadius-1;
	unsigned int accIntervals = 2*(i+1);
	unsigned int powerOf2 = roundUpToPowerOf2(accIntervals);

	if(powerOf2<32)
		powerOf2 = 32;
	
	//	struct timeval costStart, costEnd;
	//	double cost_est = 0;
	//
	//	gettimeofday(&costStart, NULL);
	//
	//	//compute estimate of bit-rate and distortion
	//	double est_br = 0;
	//	double est_psnr = 0;
	//	double c1 = log2(targetCount)+1;
	//	double c2 = -20.0*log10(realPrecision) + 20.0*log10(max-min) + 10.0*log10(3);
	//
	//	for (i = 0; i < powerOf2/2; i++)
	//	{
	//		int count = intervals[i];
	//		if (count != 0)
	//			est_br += count*log2(count);
	//		est_psnr += count;
	//	}
	//
	//	//compute estimate of bit-rate
	//	est_br -= c1*est_psnr;
	//	est_br /= totalSampleSize;
	//	est_br = -est_br;
	//
	//	//compute estimate of psnr
	//	est_psnr /= totalSampleSize;
	//	printf ("sum of P(i) = %lf\n", est_psnr);
	//	est_psnr = -10.0*log10(est_psnr);
	//	est_psnr += c2;
	//
	//	printf ("estimate bitrate = %.2f\n", est_br);
	//	printf ("estimate psnr = %.2f\n",est_psnr);
	//
	//	gettimeofday(&costEnd, NULL);
	//	cost_est = ((costEnd.tv_sec*1000000+costEnd.tv_usec)-(costStart.tv_sec*1000000+costStart.tv_usec))/1000000.0;
	//
	//	printf ("analysis time = %f\n", cost_est);

	free(intervals);
	//printf("targetCount=%d, sum=%d, totalSampleSize=%d, ratio=%f, accIntervals=%d, powerOf2=%d\n", targetCount, sum, totalSampleSize, (double)sum/(double)totalSampleSize, accIntervals, powerOf2);
	return powerOf2;
}


unsigned int optimize_intervals_float_4D(float *oriData, size_t r1, size_t r2, size_t r3, size_t r4, double realPrecision)
{
	size_t i,j,k,l, index;
	size_t radiusIndex;
	size_t r234=r2*r3*r4;
	size_t r34=r3*r4;
	float pred_value = 0, pred_err;
	size_t *intervals = (size_t*)malloc(conf_params->maxRangeRadius*sizeof(size_t));
	memset(intervals, 0, conf_params->maxRangeRadius*sizeof(size_t));
	size_t totalSampleSize = (r1-1)*(r2-1)*(r3-1)*(r4-1)/conf_params->sampleDistance;
	for(i=1;i<r1;i++)
	{
		for(j=1;j<r2;j++)
		{
			for(k=1;k<r3;k++)
			{
				for (l=1;l<r4;l++)
				{
					if((i+j+k+l)%conf_params->sampleDistance==0)
					{
						index = i*r234+j*r34+k*r4+l;
						pred_value = oriData[index-1] + oriData[index-r3] + oriData[index-r34]
								- oriData[index-1-r34] - oriData[index-r4-1] - oriData[index-r4-r34] + oriData[index-r4-r34-1];
						pred_err = fabs(pred_value - oriData[index]);
						radiusIndex = (unsigned long)((pred_err/realPrecision+1)/2);
						if(radiusIndex>=conf_params->maxRangeRadius)
							radiusIndex = conf_params->maxRangeRadius - 1;
						intervals[radiusIndex]++;
					}
				}
			}
		}
	}
	//compute the appropriate number
	size_t targetCount = totalSampleSize*conf_params->predThreshold;
	size_t sum = 0;
	for(i=0;i<conf_params->maxRangeRadius;i++)
	{
		sum += intervals[i];
		if(sum>targetCount)
			break;
	}
	if(i>=conf_params->maxRangeRadius)
		i = conf_params->maxRangeRadius-1;

	unsigned int accIntervals = 2*(i+1);
	unsigned int powerOf2 = roundUpToPowerOf2(accIntervals);

	if(powerOf2<32)
		powerOf2 = 32;

	free(intervals);
	return powerOf2;
}

TightDataPointStorageF* SZ_compress_float_1D_MDQ(float *oriData, 
size_t dataLength, double realPrecision, float valueRangeSize, float medianValue_f)
{
	float* decData = NULL;
	if(conf_params->szMode == SZ_TEMPORAL_COMPRESSION)
		decData = (float*)(multisteps->hist_data);
	
	unsigned int quantization_intervals;
	if(exe_params->optQuantMode==1)
		quantization_intervals = optimize_intervals_float_1D_opt(oriData, dataLength, realPrecision);
	else
		quantization_intervals = exe_params->intvCapacity;
	updateQuantizationInfo(quantization_intervals);	

	size_t i;
	int reqLength;
	float medianValue = medianValue_f;
	short radExpo = getExponent_float(valueRangeSize/2);
	
	computeReqLength_float(realPrecision, radExpo, &reqLength, &medianValue);	

	int* type = (int*) malloc(dataLength*sizeof(int));
		
	float* spaceFillingValue = oriData; //
	
	DynamicIntArray *exactLeadNumArray;
	new_DIA(&exactLeadNumArray, DynArrayInitLen);
	
	DynamicByteArray *exactMidByteArray;
	new_DBA(&exactMidByteArray, DynArrayInitLen);
	
	DynamicIntArray *resiBitArray;
	new_DIA(&resiBitArray, DynArrayInitLen);
	
	unsigned char preDataBytes[4];
	intToBytes_bigEndian(preDataBytes, 0);
	
	int reqBytesLength = reqLength/8;
	int resiBitsLength = reqLength%8;
	float last3CmprsData[3] = {0};

	FloatValueCompressElement *vce = (FloatValueCompressElement*)malloc(sizeof(FloatValueCompressElement));
	LossyCompressionElement *lce = (LossyCompressionElement*)malloc(sizeof(LossyCompressionElement));
				
	//add the first data	
	type[0] = 0;
	compressSingleFloatValue(vce, spaceFillingValue[0], realPrecision, medianValue, reqLength, reqBytesLength, resiBitsLength);
	updateLossyCompElement_Float(vce->curBytes, preDataBytes, reqBytesLength, resiBitsLength, lce);
	memcpy(preDataBytes,vce->curBytes,4);
	addExactData(exactMidByteArray, exactLeadNumArray, resiBitArray, lce);
	listAdd_float(last3CmprsData, vce->data);
	if(conf_params->szMode == SZ_TEMPORAL_COMPRESSION)
		decData[0] = vce->data;
		
	//add the second data
	type[1] = 0;
	compressSingleFloatValue(vce, spaceFillingValue[1], realPrecision, medianValue, reqLength, reqBytesLength, resiBitsLength);
	updateLossyCompElement_Float(vce->curBytes, preDataBytes, reqBytesLength, resiBitsLength, lce);
	memcpy(preDataBytes,vce->curBytes,4);
	addExactData(exactMidByteArray, exactLeadNumArray, resiBitArray, lce);
	listAdd_float(last3CmprsData, vce->data);
	if(conf_params->szMode == SZ_TEMPORAL_COMPRESSION)
		decData[1] = vce->data;
	
	int state;
	double checkRadius;
	float curData;
	float pred;
	float predAbsErr;
	checkRadius = (exe_params->intvCapacity-1)*realPrecision;
	double interval = 2*realPrecision;
	
	for(i=2;i<dataLength;i++)
	{	
		curData = spaceFillingValue[i];
		//pred = 2*last3CmprsData[0] - last3CmprsData[1];
		pred = last3CmprsData[0];
		predAbsErr = fabs(curData - pred);	
		if(predAbsErr<=checkRadius)
		{
			state = (predAbsErr/realPrecision+1)/2;
			if(curData>=pred)
			{
				type[i] = exe_params->intvRadius+state;
				pred = pred + state*interval;
			}
			else //curData<pred
			{
				type[i] = exe_params->intvRadius-state;
				pred = pred - state*interval;
			}
				
			//double-check the prediction error in case of machine-epsilon impact	
			if(fabs(curData-pred)>realPrecision)
			{	
				type[i] = 0;				
				compressSingleFloatValue(vce, curData, realPrecision, medianValue, reqLength, reqBytesLength, resiBitsLength);
				updateLossyCompElement_Float(vce->curBytes, preDataBytes, reqBytesLength, resiBitsLength, lce);
				memcpy(preDataBytes,vce->curBytes,4);
				addExactData(exactMidByteArray, exactLeadNumArray, resiBitArray, lce);		
				
				listAdd_float(last3CmprsData, vce->data);	
				if(conf_params->szMode == SZ_TEMPORAL_COMPRESSION)
					decData[i] = vce->data;					
			}
			else
			{
				listAdd_float(last3CmprsData, pred);
				if(conf_params->szMode == SZ_TEMPORAL_COMPRESSION)
					decData[i] = pred;				
			}	
			continue;
		}
		
		//unpredictable data processing		
		type[i] = 0;		
		compressSingleFloatValue(vce, curData, realPrecision, medianValue, reqLength, reqBytesLength, resiBitsLength);
		updateLossyCompElement_Float(vce->curBytes, preDataBytes, reqBytesLength, resiBitsLength, lce);
		memcpy(preDataBytes,vce->curBytes,4);
		addExactData(exactMidByteArray, exactLeadNumArray, resiBitArray, lce);

		listAdd_float(last3CmprsData, vce->data);					listAdd_float(last3CmprsData, vce->data);	
		if(conf_params->szMode == SZ_TEMPORAL_COMPRESSION)
			decData[i] = vce->data;	
		
	}//end of for
		
//	char* expSegmentsInBytes;
//	int expSegmentsInBytes_size = convertESCToBytes(esc, &expSegmentsInBytes);
	size_t exactDataNum = exactLeadNumArray->size;
	
	TightDataPointStorageF* tdps;
			
	new_TightDataPointStorageF(&tdps, dataLength, exactDataNum, 
			type, exactMidByteArray->array, exactMidByteArray->size,  
			exactLeadNumArray->array,  
			resiBitArray->array, resiBitArray->size, 
			resiBitsLength,
			realPrecision, medianValue, (char)reqLength, quantization_intervals, NULL, 0, 0);

//sdi:Debug
/*	int sum =0;
	for(i=0;i<dataLength;i++)
		if(type[i]==0) sum++;
	printf("opt_quantizations=%d, exactDataNum=%d, sum=%d\n",quantization_intervals, exactDataNum, sum);*/
	
	//free memory
	free_DIA(exactLeadNumArray);
	free_DIA(resiBitArray);
	free(type);	
	free(vce);
	free(lce);	
	free(exactMidByteArray); //exactMidByteArray->array has been released in free_TightDataPointStorageF(tdps);
	
	return tdps;
}

void SZ_compress_args_float_StoreOriData(float* oriData, size_t dataLength, TightDataPointStorageF* tdps, 
unsigned char** newByteData, size_t *outSize)
{
	int floatSize=sizeof(float);	
	size_t k = 0, i;
	tdps->isLossless = 1;
	size_t totalByteLength = 3 + MetaDataByteLength + exe_params->SZ_SIZE_TYPE + 1 + floatSize*dataLength;
	*newByteData = (unsigned char*)malloc(totalByteLength);
	
	unsigned char dsLengthBytes[8];
	for (i = 0; i < 3; i++)//3
		(*newByteData)[k++] = versionNumber[i];

	if(exe_params->SZ_SIZE_TYPE==4)//1
		(*newByteData)[k++] = 16; //00010000
	else
		(*newByteData)[k++] = 80;	//01010000: 01000000 indicates the SZ_SIZE_TYPE=8
	
	convertSZParamsToBytes(conf_params, &((*newByteData)[k]));
	k = k + MetaDataByteLength;	
	
	sizeToBytes(dsLengthBytes,dataLength); //SZ_SIZE_TYPE: 4 or 8	
	for (i = 0; i < exe_params->SZ_SIZE_TYPE; i++)
		(*newByteData)[k++] = dsLengthBytes[i];
		
	if(exe_params->sysEndianType==BIG_ENDIAN_SYSTEM)
		memcpy((*newByteData)+4+MetaDataByteLength+exe_params->SZ_SIZE_TYPE, oriData, dataLength*floatSize);
	else
	{
		unsigned char* p = (*newByteData)+4+MetaDataByteLength+exe_params->SZ_SIZE_TYPE;
		for(i=0;i<dataLength;i++,p+=floatSize)
			floatToBytes(p, oriData[i]);
	}	
	*outSize = totalByteLength;
}

char SZ_compress_args_float_NoCkRngeNoGzip_1D(unsigned char** newByteData, float *oriData, 
size_t dataLength, double realPrecision, size_t *outSize, float valueRangeSize, float medianValue_f)
{		
	char compressionType = 0;	
	TightDataPointStorageF* tdps = NULL;	

#ifdef HAVE_TIMECMPR
	if(conf_params->szMode == SZ_TEMPORAL_COMPRESSION)
	{
		int timestep = sz_tsc->currentStep;
		if(timestep % conf_params->snapshotCmprStep != 0)
		{
			tdps = SZ_compress_float_1D_MDQ_ts(oriData, dataLength, multisteps, realPrecision, valueRangeSize, medianValue_f);
			compressionType = 1; //time-series based compression 
		}
		else
		{	
			tdps = SZ_compress_float_1D_MDQ(oriData, dataLength, realPrecision, valueRangeSize, medianValue_f);
			compressionType = 0; //snapshot-based compression
			multisteps->lastSnapshotStep = timestep;
		}		
	}
	else
#endif
		tdps = SZ_compress_float_1D_MDQ(oriData, dataLength, realPrecision, valueRangeSize, medianValue_f);	

	convertTDPStoFlatBytes_float(tdps, newByteData, outSize);
	
	if(*outSize>dataLength*sizeof(float))
		SZ_compress_args_float_StoreOriData(oriData, dataLength+2, tdps, newByteData, outSize);
	
	free_TightDataPointStorageF(tdps);
	return compressionType;
}

TightDataPointStorageF* SZ_compress_float_2D_MDQ(float *oriData, size_t r1, size_t r2, double realPrecision, float valueRangeSize, float medianValue_f)
{
	unsigned int quantization_intervals;
	if(exe_params->optQuantMode==1)
	{
		quantization_intervals = optimize_intervals_float_2D_opt(oriData, r1, r2, realPrecision);
		updateQuantizationInfo(quantization_intervals);
	}	
	else
		quantization_intervals = exe_params->intvCapacity;
	size_t i,j; 
	int reqLength;
	float pred1D, pred2D;
	float diff = 0.0;
	double itvNum = 0;
	float *P0, *P1;
		
	size_t dataLength = r1*r2;	
	
	P0 = (float*)malloc(r2*sizeof(float));
	memset(P0, 0, r2*sizeof(float));
	P1 = (float*)malloc(r2*sizeof(float));
	memset(P1, 0, r2*sizeof(float));
		
	float medianValue = medianValue_f;
	short radExpo = getExponent_float(valueRangeSize/2);
	computeReqLength_float(realPrecision, radExpo, &reqLength, &medianValue);	

	int* type = (int*) malloc(dataLength*sizeof(int));
	//type[dataLength]=0;
		
	float* spaceFillingValue = oriData; //

	DynamicIntArray *exactLeadNumArray;
	new_DIA(&exactLeadNumArray, DynArrayInitLen);
	
	DynamicByteArray *exactMidByteArray;
	new_DBA(&exactMidByteArray, DynArrayInitLen);
	
	DynamicIntArray *resiBitArray;
	new_DIA(&resiBitArray, DynArrayInitLen);
	
	type[0] = 0;
	unsigned char preDataBytes[4];
	intToBytes_bigEndian(preDataBytes, 0);
	
	int reqBytesLength = reqLength/8;
	int resiBitsLength = reqLength%8;

	FloatValueCompressElement *vce = (FloatValueCompressElement*)malloc(sizeof(FloatValueCompressElement));
	LossyCompressionElement *lce = (LossyCompressionElement*)malloc(sizeof(LossyCompressionElement));
			
	/* Process Row-0 data 0*/
	type[0] = 0;
	compressSingleFloatValue(vce, spaceFillingValue[0], realPrecision, medianValue, reqLength, reqBytesLength, resiBitsLength);
	updateLossyCompElement_Float(vce->curBytes, preDataBytes, reqBytesLength, resiBitsLength, lce);
	memcpy(preDataBytes,vce->curBytes,4);
	addExactData(exactMidByteArray, exactLeadNumArray, resiBitArray, lce);
	P1[0] = vce->data;

	float curData;

	/* Process Row-0 data 1*/
	pred1D = P1[0];
	curData = spaceFillingValue[1];
	diff = curData - pred1D;

	itvNum =  fabs(diff)/realPrecision + 1;

	if (itvNum < exe_params->intvCapacity)
	{
		if (diff < 0) itvNum = -itvNum;
		type[1] = (int) (itvNum/2) + exe_params->intvRadius;
		P1[1] = pred1D + 2 * (type[1] - exe_params->intvRadius) * realPrecision;

		//ganrantee comporession error against the case of machine-epsilon
		if(fabs(spaceFillingValue[1]-P1[1])>realPrecision)
		{	
			type[1] = 0;			
			compressSingleFloatValue(vce, curData, realPrecision, medianValue, reqLength, reqBytesLength, resiBitsLength);
			updateLossyCompElement_Float(vce->curBytes, preDataBytes, reqBytesLength, resiBitsLength, lce);
			memcpy(preDataBytes,vce->curBytes,4);
			addExactData(exactMidByteArray, exactLeadNumArray, resiBitArray, lce);	
			
			P1[1] = vce->data;	
		}
	}
	else
	{
		type[1] = 0;
		compressSingleFloatValue(vce, curData, realPrecision, medianValue, reqLength, reqBytesLength, resiBitsLength);
		updateLossyCompElement_Float(vce->curBytes, preDataBytes, reqBytesLength, resiBitsLength, lce);
		memcpy(preDataBytes,vce->curBytes,4);
		addExactData(exactMidByteArray, exactLeadNumArray, resiBitArray, lce);
		P1[1] = vce->data;
	}

    /* Process Row-0 data 2 --> data r2-1 */
	for (j = 2; j < r2; j++)
	{
		pred1D = 2*P1[j-1] - P1[j-2];
		curData = spaceFillingValue[j];
		diff = curData - pred1D;

		itvNum = fabs(diff)/realPrecision + 1;

		if (itvNum < exe_params->intvCapacity)
		{
			if (diff < 0) itvNum = -itvNum;
			type[j] = (int) (itvNum/2) + exe_params->intvRadius;
			P1[j] = pred1D + 2 * (type[j] - exe_params->intvRadius) * realPrecision;
		
			//ganrantee comporession error against the case of machine-epsilon
			if(fabs(curData-P1[j])>realPrecision)
			{	
				type[j] = 0;				
				compressSingleFloatValue(vce, curData, realPrecision, medianValue, reqLength, reqBytesLength, resiBitsLength);
				updateLossyCompElement_Float(vce->curBytes, preDataBytes, reqBytesLength, resiBitsLength, lce);
				memcpy(preDataBytes,vce->curBytes,4);
				addExactData(exactMidByteArray, exactLeadNumArray, resiBitArray, lce);	
				
				P1[j] = vce->data;	
			}
		}
		else
		{
			type[j] = 0;
			compressSingleFloatValue(vce,curData, realPrecision, medianValue, reqLength, reqBytesLength, resiBitsLength);
			updateLossyCompElement_Float(vce->curBytes, preDataBytes, reqBytesLength, resiBitsLength, lce);
			memcpy(preDataBytes,vce->curBytes,4);
			addExactData(exactMidByteArray, exactLeadNumArray, resiBitArray, lce);
			P1[j] = vce->data;
		}
	}

	/* Process Row-1 --> Row-r1-1 */
	size_t index;
	for (i = 1; i < r1; i++)
	{	
		/* Process row-i data 0 */
		index = i*r2;
		pred1D = P1[0];
		curData = spaceFillingValue[index];
		diff = curData - pred1D;

		itvNum = fabs(diff)/realPrecision + 1;

		if (itvNum < exe_params->intvCapacity)
		{
			if (diff < 0) itvNum = -itvNum;
			type[index] = (int) (itvNum/2) + exe_params->intvRadius;
			P0[0] = pred1D + 2 * (type[index] - exe_params->intvRadius) * realPrecision;

			//ganrantee comporession error against the case of machine-epsilon
			if(fabs(curData-P0[0])>realPrecision)
			{	
				type[index] = 0;				
				compressSingleFloatValue(vce, curData, realPrecision, medianValue, reqLength, reqBytesLength, resiBitsLength);
				updateLossyCompElement_Float(vce->curBytes, preDataBytes, reqBytesLength, resiBitsLength, lce);
				memcpy(preDataBytes,vce->curBytes,4);
				addExactData(exactMidByteArray, exactLeadNumArray, resiBitArray, lce);	
				
				P0[0] = vce->data;	
			}
		}
		else
		{
			type[index] = 0;
			compressSingleFloatValue(vce, curData, realPrecision, medianValue, reqLength, reqBytesLength, resiBitsLength);
			updateLossyCompElement_Float(vce->curBytes, preDataBytes, reqBytesLength, resiBitsLength, lce);
			memcpy(preDataBytes,vce->curBytes,4);
			addExactData(exactMidByteArray, exactLeadNumArray, resiBitArray, lce);
			P0[0] = vce->data;
		}
									
		/* Process row-i data 1 --> r2-1*/
		for (j = 1; j < r2; j++)
		{
			index = i*r2+j;
			pred2D = P0[j-1] + P1[j] - P1[j-1];

			curData = spaceFillingValue[index];
			diff = curData - pred2D;

			itvNum = fabs(diff)/realPrecision + 1;

			if (itvNum < exe_params->intvCapacity)
			{
				if (diff < 0) itvNum = -itvNum;
				type[index] = (int) (itvNum/2) + exe_params->intvRadius;
				P0[j] = pred2D + 2 * (type[index] - exe_params->intvRadius) * realPrecision;
			
				//ganrantee comporession error against the case of machine-epsilon
				if(fabs(curData-P0[j])>realPrecision)
				{	
					type[index] = 0;					
					compressSingleFloatValue(vce, curData, realPrecision, medianValue, reqLength, reqBytesLength, resiBitsLength);
					updateLossyCompElement_Float(vce->curBytes, preDataBytes, reqBytesLength, resiBitsLength, lce);
					memcpy(preDataBytes,vce->curBytes,4);
					addExactData(exactMidByteArray, exactLeadNumArray, resiBitArray, lce);	
					
					P0[j] = vce->data;	
				}			
			}
			else
			{
				type[index] = 0;
				compressSingleFloatValue(vce, curData, realPrecision, medianValue, reqLength, reqBytesLength, resiBitsLength);
				updateLossyCompElement_Float(vce->curBytes, preDataBytes, reqBytesLength, resiBitsLength, lce);
				memcpy(preDataBytes,vce->curBytes,4);
				addExactData(exactMidByteArray, exactLeadNumArray, resiBitArray, lce);
				P0[j] = vce->data;
			}
		}

		float *Pt;
		Pt = P1;
		P1 = P0;
		P0 = Pt;
	}
	
	if(r2!=1)
		free(P0);
	free(P1);			
	size_t exactDataNum = exactLeadNumArray->size;
	
	TightDataPointStorageF* tdps;
			
	new_TightDataPointStorageF(&tdps, dataLength, exactDataNum, 
			type, exactMidByteArray->array, exactMidByteArray->size,  
			exactLeadNumArray->array,  
			resiBitArray->array, resiBitArray->size, 
			resiBitsLength, 
			realPrecision, medianValue, (char)reqLength, quantization_intervals, NULL, 0, 0);

//	printf("exactDataNum=%d, expSegmentsInBytes_size=%d, exactMidByteArray->size=%d\n", 
//			exactDataNum, expSegmentsInBytes_size, exactMidByteArray->size);
	
//	for(i = 3800;i<3844;i++)
//		printf("exactLeadNumArray->array[%d]=%d\n",i,exactLeadNumArray->array[i]);
	
	//free memory
	free_DIA(exactLeadNumArray);
	free_DIA(resiBitArray);
	free(type);
	free(vce);
	free(lce);
	free(exactMidByteArray); //exactMidByteArray->array has been released in free_TightDataPointStorageF(tdps);
	
	return tdps;	
}

/**
 * 
 * Note: @r1 is high dimension
 * 		 @r2 is low dimension 
 * */
char SZ_compress_args_float_NoCkRngeNoGzip_2D(unsigned char** newByteData, float *oriData, size_t r1, size_t r2, double realPrecision, size_t *outSize, float valueRangeSize, float medianValue_f)
{	
	size_t dataLength = r1*r2;
	char compressionType = 0;	
	TightDataPointStorageF* tdps = NULL; 

#ifdef HAVE_TIMECMPR
	if(conf_params->szMode == SZ_TEMPORAL_COMPRESSION)
	{
		int timestep = sz_tsc->currentStep;
		if(timestep % conf_params->snapshotCmprStep != 0)
		{
			tdps = SZ_compress_float_1D_MDQ_ts(oriData, dataLength, multisteps, realPrecision, valueRangeSize, medianValue_f);
			compressionType = 1; //time-series based compression 
		}
		else
		{	
			tdps = SZ_compress_float_2D_MDQ(oriData, r1, r2, realPrecision, valueRangeSize, medianValue_f);
			compressionType = 0; //snapshot-based compression
			multisteps->lastSnapshotStep = timestep;
		}		
	}
	else
#endif
		tdps = SZ_compress_float_2D_MDQ(oriData, r1, r2, realPrecision, valueRangeSize, medianValue_f);	

	convertTDPStoFlatBytes_float(tdps, newByteData, outSize);

	if(*outSize>dataLength*sizeof(float))
		SZ_compress_args_float_StoreOriData(oriData, dataLength, tdps, newByteData, outSize);
	
	free_TightDataPointStorageF(tdps);	
	
	return compressionType;
}

TightDataPointStorageF* SZ_compress_float_3D_MDQ(float *oriData, size_t r1, size_t r2, size_t r3, double realPrecision, float valueRangeSize, float medianValue_f)
{
	unsigned int quantization_intervals;
	if(exe_params->optQuantMode==1)
	{
		quantization_intervals = optimize_intervals_float_3D_opt(oriData, r1, r2, r3, realPrecision);
		updateQuantizationInfo(quantization_intervals);
	}	
	else
		quantization_intervals = exe_params->intvCapacity;
	size_t i,j,k; 
	int reqLength;
	float pred1D, pred2D, pred3D;
	float diff = 0.0;
	double itvNum = 0;
	float *P0, *P1;

	size_t dataLength = r1*r2*r3;
	size_t r23 = r2*r3;
	P0 = (float*)malloc(r23*sizeof(float));
	P1 = (float*)malloc(r23*sizeof(float));

	float medianValue = medianValue_f;
	short radExpo = getExponent_float(valueRangeSize/2);
	computeReqLength_float(realPrecision, radExpo, &reqLength, &medianValue);	

	int* type = (int*) malloc(dataLength*sizeof(int));

	float* spaceFillingValue = oriData; //

	DynamicIntArray *exactLeadNumArray;
	new_DIA(&exactLeadNumArray, DynArrayInitLen);

	DynamicByteArray *exactMidByteArray;
	new_DBA(&exactMidByteArray, DynArrayInitLen);

	DynamicIntArray *resiBitArray;
	new_DIA(&resiBitArray, DynArrayInitLen);

	unsigned char preDataBytes[4];
	intToBytes_bigEndian(preDataBytes, 0);
	
	int reqBytesLength = reqLength/8;
	int resiBitsLength = reqLength%8;

	FloatValueCompressElement *vce = (FloatValueCompressElement*)malloc(sizeof(FloatValueCompressElement));
	LossyCompressionElement *lce = (LossyCompressionElement*)malloc(sizeof(LossyCompressionElement));


	///////////////////////////	Process layer-0 ///////////////////////////
	/* Process Row-0 data 0*/
	type[0] = 0;
	compressSingleFloatValue(vce, spaceFillingValue[0], realPrecision, medianValue, reqLength, reqBytesLength, resiBitsLength);
	updateLossyCompElement_Float(vce->curBytes, preDataBytes, reqBytesLength, resiBitsLength, lce);
	memcpy(preDataBytes,vce->curBytes,4);
	addExactData(exactMidByteArray, exactLeadNumArray, resiBitArray, lce);
	P1[0] = vce->data;

	float curData;

	/* Process Row-0 data 1*/
	pred1D = P1[0];
	curData = spaceFillingValue[1];
	diff = curData - pred1D;

	itvNum = fabs(diff)/realPrecision + 1;

	if (itvNum < exe_params->intvCapacity)
	{
		if (diff < 0) itvNum = -itvNum;
		type[1] = (int) (itvNum/2) + exe_params->intvRadius;
		P1[1] = pred1D + 2 * (type[1] - exe_params->intvRadius) * realPrecision;
		
		//ganrantee comporession error against the case of machine-epsilon
		if(fabs(curData-P1[1])>realPrecision)
		{	
			type[1] = 0;			
			compressSingleFloatValue(vce, curData, realPrecision, medianValue, reqLength, reqBytesLength, resiBitsLength);
			updateLossyCompElement_Float(vce->curBytes, preDataBytes, reqBytesLength, resiBitsLength, lce);
			memcpy(preDataBytes,vce->curBytes,4);
			addExactData(exactMidByteArray, exactLeadNumArray, resiBitArray, lce);	
			
			P1[1] = vce->data;	
		}		
	}
	else
	{
		type[1] = 0;
		compressSingleFloatValue(vce, curData, realPrecision, medianValue, reqLength, reqBytesLength, resiBitsLength);
		updateLossyCompElement_Float(vce->curBytes, preDataBytes, reqBytesLength, resiBitsLength, lce);
		memcpy(preDataBytes,vce->curBytes,4);
		addExactData(exactMidByteArray, exactLeadNumArray, resiBitArray, lce);
		P1[1] = vce->data;
	}

    /* Process Row-0 data 2 --> data r3-1 */
	for (j = 2; j < r3; j++)
	{
		pred1D = 2*P1[j-1] - P1[j-2];
		curData = spaceFillingValue[j];
		diff = curData - pred1D;

		itvNum = fabs(diff)/realPrecision + 1;

		if (itvNum < exe_params->intvCapacity)
		{
			if (diff < 0) itvNum = -itvNum;
			type[j] = (int) (itvNum/2) + exe_params->intvRadius;
			P1[j] = pred1D + 2 * (type[j] - exe_params->intvRadius) * realPrecision;
			
			//ganrantee comporession error against the case of machine-epsilon
			if(fabs(curData-P1[j])>realPrecision)
			{	
				type[j] = 0;				
				compressSingleFloatValue(vce, curData, realPrecision, medianValue, reqLength, reqBytesLength, resiBitsLength);
				updateLossyCompElement_Float(vce->curBytes, preDataBytes, reqBytesLength, resiBitsLength, lce);
				memcpy(preDataBytes,vce->curBytes,4);
				addExactData(exactMidByteArray, exactLeadNumArray, resiBitArray, lce);	
				
				P1[j] = vce->data;	
			}			
		}
		else
		{
			type[j] = 0;
			compressSingleFloatValue(vce, curData, realPrecision, medianValue, reqLength, reqBytesLength, resiBitsLength);
			updateLossyCompElement_Float(vce->curBytes, preDataBytes, reqBytesLength, resiBitsLength, lce);
			memcpy(preDataBytes,vce->curBytes,4);
			addExactData(exactMidByteArray, exactLeadNumArray, resiBitArray, lce);
			P1[j] = vce->data;
		}
	}

	/* Process Row-1 --> Row-r2-1 */
	size_t index;
	for (i = 1; i < r2; i++)
	{
		/* Process row-i data 0 */
		index = i*r3;	
		pred1D = P1[index-r3];
		curData = spaceFillingValue[index];
		diff = curData - pred1D;

		itvNum = fabs(diff)/realPrecision + 1;

		if (itvNum < exe_params->intvCapacity)
		{
			if (diff < 0) itvNum = -itvNum;
			type[index] = (int) (itvNum/2) + exe_params->intvRadius;
			P1[index] = pred1D + 2 * (type[index] - exe_params->intvRadius) * realPrecision;
			
			//ganrantee comporession error against the case of machine-epsilon
			if(fabs(curData-P1[index])>realPrecision)
			{	
				type[index] = 0;				
				compressSingleFloatValue(vce, curData, realPrecision, medianValue, reqLength, reqBytesLength, resiBitsLength);
				updateLossyCompElement_Float(vce->curBytes, preDataBytes, reqBytesLength, resiBitsLength, lce);
				memcpy(preDataBytes,vce->curBytes,4);
				addExactData(exactMidByteArray, exactLeadNumArray, resiBitArray, lce);	
				
				P1[index] = vce->data;	
			}			
		}
		else
		{
			type[index] = 0;
			compressSingleFloatValue(vce, curData, realPrecision, medianValue, reqLength, reqBytesLength, resiBitsLength);
			updateLossyCompElement_Float(vce->curBytes, preDataBytes, reqBytesLength, resiBitsLength, lce);
			memcpy(preDataBytes,vce->curBytes,4);
			addExactData(exactMidByteArray, exactLeadNumArray, resiBitArray, lce);
			P1[index] = vce->data;
		}

		/* Process row-i data 1 --> data r3-1*/
		for (j = 1; j < r3; j++)
		{
			index = i*r3+j;
			pred2D = P1[index-1] + P1[index-r3] - P1[index-r3-1];

			curData = spaceFillingValue[index];
			diff = curData - pred2D;

			itvNum = fabs(diff)/realPrecision + 1;

			if (itvNum < exe_params->intvCapacity)
			{
				if (diff < 0) itvNum = -itvNum;
				type[index] = (int) (itvNum/2) + exe_params->intvRadius;
				P1[index] = pred2D + 2 * (type[index] - exe_params->intvRadius) * realPrecision;
				
				//ganrantee comporession error against the case of machine-epsilon
				if(fabs(curData-P1[index])>realPrecision)
				{	
					type[index] = 0;					
					compressSingleFloatValue(vce, curData, realPrecision, medianValue, reqLength, reqBytesLength, resiBitsLength);
					updateLossyCompElement_Float(vce->curBytes, preDataBytes, reqBytesLength, resiBitsLength, lce);
					memcpy(preDataBytes,vce->curBytes,4);
					addExactData(exactMidByteArray, exactLeadNumArray, resiBitArray, lce);	
					
					P1[index] = vce->data;	
				}				
			}
			else
			{
				type[index] = 0;
				compressSingleFloatValue(vce, curData, realPrecision, medianValue, reqLength, reqBytesLength, resiBitsLength);
				updateLossyCompElement_Float(vce->curBytes, preDataBytes, reqBytesLength, resiBitsLength, lce);
				memcpy(preDataBytes,vce->curBytes,4);
				addExactData(exactMidByteArray, exactLeadNumArray, resiBitArray, lce);
				P1[index] = vce->data;
			}
		}
	}


	///////////////////////////	Process layer-1 --> layer-r1-1 ///////////////////////////

	for (k = 1; k < r1; k++)
	{
		/* Process Row-0 data 0*/
		index = k*r23;
		pred1D = P1[0];
		curData = spaceFillingValue[index];
		diff = curData - pred1D;

		itvNum = fabs(diff)/realPrecision + 1;

		if (itvNum < exe_params->intvCapacity)
		{
			if (diff < 0) itvNum = -itvNum;
			type[index] = (int) (itvNum/2) + exe_params->intvRadius;
			P0[0] = pred1D + 2 * (type[index] - exe_params->intvRadius) * realPrecision;
			
			//ganrantee comporession error against the case of machine-epsilon
			if(fabs(curData-P0[0])>realPrecision)
			{	
				type[index] = 0;				
				compressSingleFloatValue(vce, curData, realPrecision, medianValue, reqLength, reqBytesLength, resiBitsLength);
				updateLossyCompElement_Float(vce->curBytes, preDataBytes, reqBytesLength, resiBitsLength, lce);
				memcpy(preDataBytes,vce->curBytes,4);
				addExactData(exactMidByteArray, exactLeadNumArray, resiBitArray, lce);	
				
				P0[0] = vce->data;	
			}			
		}
		else
		{
			type[index] = 0;
			compressSingleFloatValue(vce, curData, realPrecision, medianValue, reqLength, reqBytesLength, resiBitsLength);
			updateLossyCompElement_Float(vce->curBytes, preDataBytes, reqBytesLength, resiBitsLength, lce);
			memcpy(preDataBytes,vce->curBytes,4);
			addExactData(exactMidByteArray, exactLeadNumArray, resiBitArray, lce);
			P0[0] = vce->data;
		}


	    /* Process Row-0 data 1 --> data r3-1 */
		for (j = 1; j < r3; j++)
		{
			//index = k*r2*r3+j;
			index ++;
			pred2D = P0[j-1] + P1[j] - P1[j-1];
			curData = spaceFillingValue[index];
			diff = spaceFillingValue[index] - pred2D;

			itvNum = fabs(diff)/realPrecision + 1;

			if (itvNum < exe_params->intvCapacity)
			{
				if (diff < 0) itvNum = -itvNum;
				type[index] = (int) (itvNum/2) + exe_params->intvRadius;
				P0[j] = pred2D + 2 * (type[index] - exe_params->intvRadius) * realPrecision;
				//ganrantee comporession error against the case of machine-epsilon
				if(fabs(curData-P0[j])>realPrecision)
				{	
					type[index] = 0;					
					compressSingleFloatValue(vce, curData, realPrecision, medianValue, reqLength, reqBytesLength, resiBitsLength);
					updateLossyCompElement_Float(vce->curBytes, preDataBytes, reqBytesLength, resiBitsLength, lce);
					memcpy(preDataBytes,vce->curBytes,4);
					addExactData(exactMidByteArray, exactLeadNumArray, resiBitArray, lce);	
					
					P0[j] = vce->data;	
				}
			}
			else
			{
				type[index] = 0;
				compressSingleFloatValue(vce, curData, realPrecision, medianValue, reqLength, reqBytesLength, resiBitsLength);
				updateLossyCompElement_Float(vce->curBytes, preDataBytes, reqBytesLength, resiBitsLength, lce);
				memcpy(preDataBytes,vce->curBytes,4);
				addExactData(exactMidByteArray, exactLeadNumArray, resiBitArray, lce);
				P0[j] = vce->data;
			}
		}

	    /* Process Row-1 --> Row-r2-1 */
		size_t index2D;
		for (i = 1; i < r2; i++)
		{
			/* Process Row-i data 0 */
			index = k*r23 + i*r3;
			index2D = i*r3;		
			pred2D = P0[index2D-r3] + P1[index2D] - P1[index2D-r3];
			curData = spaceFillingValue[index];
			diff = spaceFillingValue[index] - pred2D;

			itvNum = fabs(diff)/realPrecision + 1;

			if (itvNum < exe_params->intvCapacity)
			{
				if (diff < 0) itvNum = -itvNum;
				type[index] = (int) (itvNum/2) + exe_params->intvRadius;
				P0[index2D] = pred2D + 2 * (type[index] - exe_params->intvRadius) * realPrecision;
				//ganrantee comporession error against the case of machine-epsilon
				if(fabs(curData-P0[index2D])>realPrecision)
				{	
					type[index] = 0;					
					compressSingleFloatValue(vce, curData, realPrecision, medianValue, reqLength, reqBytesLength, resiBitsLength);
					updateLossyCompElement_Float(vce->curBytes, preDataBytes, reqBytesLength, resiBitsLength, lce);
					memcpy(preDataBytes,vce->curBytes,4);
					addExactData(exactMidByteArray, exactLeadNumArray, resiBitArray, lce);	
					
					P0[index2D] = vce->data;	
				}				
			}
			else
			{
				type[index] = 0;
				compressSingleFloatValue(vce, curData, realPrecision, medianValue, reqLength, reqBytesLength, resiBitsLength);
				updateLossyCompElement_Float(vce->curBytes, preDataBytes, reqBytesLength, resiBitsLength, lce);
				memcpy(preDataBytes,vce->curBytes,4);
				addExactData(exactMidByteArray, exactLeadNumArray, resiBitArray, lce);
				P0[index2D] = vce->data;
			}

			/* Process Row-i data 1 --> data r3-1 */
			for (j = 1; j < r3; j++)
			{
//				if(k==63&&i==43&&j==27)
//					printf("i=%d\n", i);
				//index = k*r2*r3 + i*r3 + j;			
				index ++;
				index2D = i*r3 + j;
				pred3D = P0[index2D-1] + P0[index2D-r3]+ P1[index2D] - P0[index2D-r3-1] - P1[index2D-r3] - P1[index2D-1] + P1[index2D-r3-1];
				curData = spaceFillingValue[index];
				diff = curData - pred3D;

				itvNum = fabs(diff)/realPrecision + 1;

				if (itvNum < exe_params->intvCapacity)
				{
					if (diff < 0) itvNum = -itvNum;
					type[index] = (int) (itvNum/2) + exe_params->intvRadius;
					P0[index2D] = pred3D + 2 * (type[index] - exe_params->intvRadius) * realPrecision;
					
					//ganrantee comporession error against the case of machine-epsilon
					if(fabs(curData-P0[index2D])>realPrecision)
					{	
						type[index] = 0;						
						compressSingleFloatValue(vce, curData, realPrecision, medianValue, reqLength, reqBytesLength, resiBitsLength);
						updateLossyCompElement_Float(vce->curBytes, preDataBytes, reqBytesLength, resiBitsLength, lce);
						memcpy(preDataBytes,vce->curBytes,4);
						addExactData(exactMidByteArray, exactLeadNumArray, resiBitArray, lce);	
						
						P0[index2D] = vce->data;	
					}					
				}
				else
				{
					type[index] = 0;
					compressSingleFloatValue(vce, curData, realPrecision, medianValue, reqLength, reqBytesLength, resiBitsLength);
					updateLossyCompElement_Float(vce->curBytes, preDataBytes, reqBytesLength, resiBitsLength, lce);
					memcpy(preDataBytes,vce->curBytes,4);
					addExactData(exactMidByteArray, exactLeadNumArray, resiBitArray, lce);
					P0[index2D] = vce->data;
				}
			}
		}

		float *Pt;
		Pt = P1;
		P1 = P0;
		P0 = Pt;
	}
	if(r23!=1)
		free(P0);
	free(P1);
	size_t exactDataNum = exactLeadNumArray->size;

	TightDataPointStorageF* tdps;

	new_TightDataPointStorageF(&tdps, dataLength, exactDataNum,
			type, exactMidByteArray->array, exactMidByteArray->size,
			exactLeadNumArray->array,
			resiBitArray->array, resiBitArray->size,
			resiBitsLength, 
			realPrecision, medianValue, (char)reqLength, quantization_intervals, NULL, 0, 0);

//sdi:Debug
/*	int sum =0;
	for(i=0;i<dataLength;i++)
		if(type[i]==0) sum++;
	printf("opt_quantizations=%d, exactDataNum=%d, sum=%d\n",quantization_intervals, exactDataNum, sum);*/


//	printf("exactDataNum=%d, expSegmentsInBytes_size=%d, exactMidByteArray->size=%d\n",
//			exactDataNum, expSegmentsInBytes_size, exactMidByteArray->size);

	//free memory
	free_DIA(exactLeadNumArray);
	free_DIA(resiBitArray);
	free(type);	
	free(vce);
	free(lce);
	free(exactMidByteArray); //exactMidByteArray->array has been released in free_TightDataPointStorageF(tdps);
	
	return tdps;	
}

char SZ_compress_args_float_NoCkRngeNoGzip_3D(unsigned char** newByteData, float *oriData, size_t r1, size_t r2, size_t r3, double realPrecision, size_t *outSize, float valueRangeSize, float medianValue_f)
{
	size_t dataLength = r1*r2*r3;
	char compressionType = 0;	
	TightDataPointStorageF* tdps = NULL; 

#ifdef HAVE_TIMECMPR
	if(conf_params->szMode == SZ_TEMPORAL_COMPRESSION)
	{
		int timestep = sz_tsc->currentStep;
		if(timestep % conf_params->snapshotCmprStep != 0)
		{
			tdps = SZ_compress_float_1D_MDQ_ts(oriData, dataLength, multisteps, realPrecision, valueRangeSize, medianValue_f);
			compressionType = 1; //time-series based compression 
		}
		else
		{	
			tdps = SZ_compress_float_3D_MDQ(oriData, r1, r2, r3, realPrecision, valueRangeSize, medianValue_f);
			compressionType = 0; //snapshot-based compression
			multisteps->lastSnapshotStep = timestep;
		}		
	}
	else
#endif
		tdps = SZ_compress_float_3D_MDQ(oriData, r1, r2, r3, realPrecision, valueRangeSize, medianValue_f);


	convertTDPStoFlatBytes_float(tdps, newByteData, outSize);

	if(*outSize>dataLength*sizeof(float))
		SZ_compress_args_float_StoreOriData(oriData, dataLength, tdps, newByteData, outSize);

	free_TightDataPointStorageF(tdps);
	
	return compressionType;
}


TightDataPointStorageF* SZ_compress_float_4D_MDQ(float *oriData, size_t r1, size_t r2, size_t r3, size_t r4, double realPrecision, float valueRangeSize, float medianValue_f)
{
	unsigned int quantization_intervals;
	if(exe_params->optQuantMode==1)
	{
		quantization_intervals = optimize_intervals_float_4D(oriData, r1, r2, r3, r4, realPrecision);
		updateQuantizationInfo(quantization_intervals);
	}
	else
		quantization_intervals = exe_params->intvCapacity;

	size_t i,j,k; 
	int reqLength;
	float pred1D, pred2D, pred3D;
	float diff = 0.0;
	double itvNum = 0;
	float *P0, *P1;

	size_t dataLength = r1*r2*r3*r4;

	size_t r234 = r2*r3*r4;
	size_t r34 = r3*r4;

	P0 = (float*)malloc(r34*sizeof(float));
	P1 = (float*)malloc(r34*sizeof(float));

	float medianValue = medianValue_f;
	short radExpo = getExponent_float(valueRangeSize/2);
	computeReqLength_float(realPrecision, radExpo, &reqLength, &medianValue);

	int* type = (int*) malloc(dataLength*sizeof(int));

	float* spaceFillingValue = oriData; //

	DynamicIntArray *exactLeadNumArray;
	new_DIA(&exactLeadNumArray, DynArrayInitLen);

	DynamicByteArray *exactMidByteArray;
	new_DBA(&exactMidByteArray, DynArrayInitLen);

	DynamicIntArray *resiBitArray;
	new_DIA(&resiBitArray, DynArrayInitLen);

	unsigned char preDataBytes[4];
	intToBytes_bigEndian(preDataBytes, 0);

	int reqBytesLength = reqLength/8;
	int resiBitsLength = reqLength%8;

	FloatValueCompressElement *vce = (FloatValueCompressElement*)malloc(sizeof(FloatValueCompressElement));
	LossyCompressionElement *lce = (LossyCompressionElement*)malloc(sizeof(LossyCompressionElement));


	size_t l;
	for (l = 0; l < r1; l++)
	{

		///////////////////////////	Process layer-0 ///////////////////////////
		/* Process Row-0 data 0*/
		size_t index = l*r234;
		size_t index2D = 0;

		type[index] = 0;
		compressSingleFloatValue(vce, spaceFillingValue[index], realPrecision, medianValue, reqLength, reqBytesLength, resiBitsLength);
		updateLossyCompElement_Float(vce->curBytes, preDataBytes, reqBytesLength, resiBitsLength, lce);
		memcpy(preDataBytes,vce->curBytes,4);
		addExactData(exactMidByteArray, exactLeadNumArray, resiBitArray, lce);
		P1[index2D] = vce->data;

		/* Process Row-0 data 1*/
		index = l*r234+1;
		index2D = 1;

		pred1D = P1[index2D-1];
		diff = spaceFillingValue[index] - pred1D;

		itvNum = fabs(diff)/realPrecision + 1;

		if (itvNum < exe_params->intvCapacity)
		{
			if (diff < 0) itvNum = -itvNum;
			type[index] = (int) (itvNum/2) + exe_params->intvRadius;
			P1[index2D] = pred1D + 2 * (type[index] - exe_params->intvRadius) * realPrecision;
		}
		else
		{
			type[index] = 0;
			compressSingleFloatValue(vce, spaceFillingValue[index], realPrecision, medianValue, reqLength, reqBytesLength, resiBitsLength);
			updateLossyCompElement_Float(vce->curBytes, preDataBytes, reqBytesLength, resiBitsLength, lce);
			memcpy(preDataBytes,vce->curBytes,4);
			addExactData(exactMidByteArray, exactLeadNumArray, resiBitArray, lce);
			P1[index2D] = vce->data;
		}

		/* Process Row-0 data 2 --> data r4-1 */
		for (j = 2; j < r4; j++)
		{
			index = l*r234+j;
			index2D = j;

			pred1D = 2*P1[index2D-1] - P1[index2D-2];
			diff = spaceFillingValue[index] - pred1D;

			itvNum = fabs(diff)/realPrecision + 1;

			if (itvNum < exe_params->intvCapacity)
			{
				if (diff < 0) itvNum = -itvNum;
				type[index] = (int) (itvNum/2) + exe_params->intvRadius;
				P1[index2D] = pred1D + 2 * (type[index] - exe_params->intvRadius) * realPrecision;
			}
			else
			{
				type[index] = 0;
				compressSingleFloatValue(vce, spaceFillingValue[index], realPrecision, medianValue, reqLength, reqBytesLength, resiBitsLength);
				updateLossyCompElement_Float(vce->curBytes, preDataBytes, reqBytesLength, resiBitsLength, lce);
				memcpy(preDataBytes,vce->curBytes,4);
				addExactData(exactMidByteArray, exactLeadNumArray, resiBitArray, lce);
				P1[index2D] = vce->data;
			}
		}

		/* Process Row-1 --> Row-r3-1 */
		for (i = 1; i < r3; i++)
		{
			/* Process row-i data 0 */
			index = l*r234+i*r4;
			index2D = i*r4;

			pred1D = P1[index2D-r4];
			diff = spaceFillingValue[index] - pred1D;

			itvNum = fabs(diff)/realPrecision + 1;

			if (itvNum < exe_params->intvCapacity)
			{
				if (diff < 0) itvNum = -itvNum;
				type[index] = (int) (itvNum/2) + exe_params->intvRadius;
				P1[index2D] = pred1D + 2 * (type[index] - exe_params->intvRadius) * realPrecision;
			}
			else
			{
				type[index] = 0;
				compressSingleFloatValue(vce, spaceFillingValue[index], realPrecision, medianValue, reqLength, reqBytesLength, resiBitsLength);
				updateLossyCompElement_Float(vce->curBytes, preDataBytes, reqBytesLength, resiBitsLength, lce);
				memcpy(preDataBytes,vce->curBytes,4);
				addExactData(exactMidByteArray, exactLeadNumArray, resiBitArray, lce);
				P1[index2D] = vce->data;
			}

			/* Process row-i data 1 --> data r4-1*/
			for (j = 1; j < r4; j++)
			{
				index = l*r234+i*r4+j;
				index2D = i*r4+j;

				pred2D = P1[index2D-1] + P1[index2D-r4] - P1[index2D-r4-1];

				diff = spaceFillingValue[index] - pred2D;

				itvNum = fabs(diff)/realPrecision + 1;

				if (itvNum < exe_params->intvCapacity)
				{
					if (diff < 0) itvNum = -itvNum;
					type[index] = (int) (itvNum/2) + exe_params->intvRadius;
					P1[index2D] = pred2D + 2 * (type[index] - exe_params->intvRadius) * realPrecision;
				}
				else
				{
					type[index] = 0;
					compressSingleFloatValue(vce, spaceFillingValue[index], realPrecision, medianValue, reqLength, reqBytesLength, resiBitsLength);
					updateLossyCompElement_Float(vce->curBytes, preDataBytes, reqBytesLength, resiBitsLength, lce);
					memcpy(preDataBytes,vce->curBytes,4);
					addExactData(exactMidByteArray, exactLeadNumArray, resiBitArray, lce);
					P1[index2D] = vce->data;
				}
			}
		}


		///////////////////////////	Process layer-1 --> layer-r2-1 ///////////////////////////

		for (k = 1; k < r2; k++)
		{
			/* Process Row-0 data 0*/
			index = l*r234+k*r34;
			index2D = 0;

			pred1D = P1[index2D];
			diff = spaceFillingValue[index] - pred1D;

			itvNum = fabs(diff)/realPrecision + 1;

			if (itvNum < exe_params->intvCapacity)
			{
				if (diff < 0) itvNum = -itvNum;
				type[index] = (int) (itvNum/2) + exe_params->intvRadius;
				P0[index2D] = pred1D + 2 * (type[index] - exe_params->intvRadius) * realPrecision;
			}
			else
			{
				type[index] = 0;
				compressSingleFloatValue(vce, spaceFillingValue[index], realPrecision, medianValue, reqLength, reqBytesLength, resiBitsLength);
				updateLossyCompElement_Float(vce->curBytes, preDataBytes, reqBytesLength, resiBitsLength, lce);
				memcpy(preDataBytes,vce->curBytes,4);
				addExactData(exactMidByteArray, exactLeadNumArray, resiBitArray, lce);
				P0[index2D] = vce->data;
			}

			/* Process Row-0 data 1 --> data r4-1 */
			for (j = 1; j < r4; j++)
			{
				index = l*r234+k*r34+j;
				index2D = j;

				pred2D = P0[index2D-1] + P1[index2D] - P1[index2D-1];
				diff = spaceFillingValue[index] - pred2D;

				itvNum = fabs(diff)/realPrecision + 1;

				if (itvNum < exe_params->intvCapacity)
				{
					if (diff < 0) itvNum = -itvNum;
					type[index] = (int) (itvNum/2) + exe_params->intvRadius;
					P0[index2D] = pred2D + 2 * (type[index] - exe_params->intvRadius) * realPrecision;
				}
				else
				{
					type[index] = 0;
					compressSingleFloatValue(vce, spaceFillingValue[index], realPrecision, medianValue, reqLength, reqBytesLength, resiBitsLength);
					updateLossyCompElement_Float(vce->curBytes, preDataBytes, reqBytesLength, resiBitsLength, lce);
					memcpy(preDataBytes,vce->curBytes,4);
					addExactData(exactMidByteArray, exactLeadNumArray, resiBitArray, lce);
					P0[index2D] = vce->data;
				}
			}

			/* Process Row-1 --> Row-r3-1 */
			for (i = 1; i < r3; i++)
			{
				/* Process Row-i data 0 */
				index = l*r234+k*r34+i*r4;
				index2D = i*r4;

				pred2D = P0[index2D-r4] + P1[index2D] - P1[index2D-r4];
				diff = spaceFillingValue[index] - pred2D;

				itvNum = fabs(diff)/realPrecision + 1;

				if (itvNum < exe_params->intvCapacity)
				{
					if (diff < 0) itvNum = -itvNum;
					type[index] = (int) (itvNum/2) + exe_params->intvRadius;
					P0[index2D] = pred2D + 2 * (type[index] - exe_params->intvRadius) * realPrecision;
				}
				else
				{
					type[index] = 0;
					compressSingleFloatValue(vce, spaceFillingValue[index], realPrecision, medianValue, reqLength, reqBytesLength, resiBitsLength);
					updateLossyCompElement_Float(vce->curBytes, preDataBytes, reqBytesLength, resiBitsLength, lce);
					memcpy(preDataBytes,vce->curBytes,4);
					addExactData(exactMidByteArray, exactLeadNumArray, resiBitArray, lce);
					P0[index2D] = vce->data;
				}

				/* Process Row-i data 1 --> data r4-1 */
				for (j = 1; j < r4; j++)
				{
					index = l*r234+k*r34+i*r4+j;
					index2D = i*r4+j;

					pred3D = P0[index2D-1] + P0[index2D-r4]+ P1[index2D] - P0[index2D-r4-1] - P1[index2D-r4] - P1[index2D-1] + P1[index2D-r4-1];
					diff = spaceFillingValue[index] - pred3D;


					itvNum = fabs(diff)/realPrecision + 1;

					if (itvNum < exe_params->intvCapacity)
					{
						if (diff < 0) itvNum = -itvNum;
						type[index] = (int) (itvNum/2) + exe_params->intvRadius;
						P0[index2D] = pred3D + 2 * (type[index] - exe_params->intvRadius) * realPrecision;
					}
					else
					{
						type[index] = 0;
						compressSingleFloatValue(vce, spaceFillingValue[index], realPrecision, medianValue, reqLength, reqBytesLength, resiBitsLength);
						updateLossyCompElement_Float(vce->curBytes, preDataBytes, reqBytesLength, resiBitsLength, lce);
						memcpy(preDataBytes,vce->curBytes,4);
						addExactData(exactMidByteArray, exactLeadNumArray, resiBitArray, lce);
						P0[index2D] = vce->data;
					}
				}
			}

			float *Pt;
			Pt = P1;
			P1 = P0;
			P0 = Pt;
		}
	}

	free(P0);
	free(P1);
	size_t exactDataNum = exactLeadNumArray->size;

	TightDataPointStorageF* tdps;

	new_TightDataPointStorageF(&tdps, dataLength, exactDataNum,
			type, exactMidByteArray->array, exactMidByteArray->size,
			exactLeadNumArray->array,
			resiBitArray->array, resiBitArray->size,
			resiBitsLength,
			realPrecision, medianValue, (char)reqLength, quantization_intervals, NULL, 0, 0);

	//free memory
	free_DIA(exactLeadNumArray);
	free_DIA(resiBitArray);
	free(type);
	free(vce);
	free(lce);
	free(exactMidByteArray); //exactMidByteArray->array has been released in free_TightDataPointStorageF(tdps);

	return tdps;
}

char SZ_compress_args_float_NoCkRngeNoGzip_4D(unsigned char** newByteData, float *oriData, size_t r1, size_t r2, size_t r3, size_t r4, double realPrecision, size_t *outSize, float valueRangeSize, float medianValue_f)
{
	TightDataPointStorageF* tdps = SZ_compress_float_4D_MDQ(oriData, r1, r2, r3, r4, realPrecision, valueRangeSize, medianValue_f);

	convertTDPStoFlatBytes_float(tdps, newByteData, outSize);

	int dataLength = r1*r2*r3*r4;
	if(*outSize>dataLength*sizeof(float))
		SZ_compress_args_float_StoreOriData(oriData, dataLength, tdps, newByteData, outSize);

	free_TightDataPointStorageF(tdps);
	
	return 0;
}

void SZ_compress_args_float_withinRange(unsigned char** newByteData, float *oriData, size_t dataLength, size_t *outSize)
{
	TightDataPointStorageF* tdps = (TightDataPointStorageF*) malloc(sizeof(TightDataPointStorageF));
	tdps->rtypeArray = NULL;
	tdps->typeArray = NULL;	
	tdps->leadNumArray = NULL;
	tdps->residualMidBits = NULL;
	
	tdps->allSameData = 1;
	tdps->dataSeriesLength = dataLength;
	tdps->exactMidBytes = (unsigned char*)malloc(sizeof(unsigned char)*4);
	tdps->pwrErrBoundBytes = NULL;
	tdps->isLossless = 0;
	float value = oriData[0];
	floatToBytes(tdps->exactMidBytes, value);
	tdps->exactMidBytes_size = 4;
	
	size_t tmpOutSize;
	//unsigned char *tmpByteData;
	convertTDPStoFlatBytes_float(tdps, newByteData, &tmpOutSize);

	//*newByteData = (unsigned char*)malloc(sizeof(unsigned char)*12); //for floating-point data (1+3+4+4)
	//memcpy(*newByteData, tmpByteData, 12);
	*outSize = tmpOutSize; //8+SZ_SIZE_TYPE; //8==3+1+4(float_size)
	free_TightDataPointStorageF(tdps);	
}

int SZ_compress_args_float_wRngeNoGzip(unsigned char** newByteData, float *oriData, 
size_t r5, size_t r4, size_t r3, size_t r2, size_t r1, size_t *outSize, 
int errBoundMode, double absErr_Bound, double relBoundRatio, double pwrErrRatio)
{
	int status = SZ_SCES;
	size_t dataLength = computeDataLength(r5,r4,r3,r2,r1);
	float valueRangeSize = 0, medianValue = 0;
	
	float min = computeRangeSize_float(oriData, dataLength, &valueRangeSize, &medianValue);
	float max = min+valueRangeSize;
	double realPrecision = getRealPrecision_float(valueRangeSize, errBoundMode, absErr_Bound, relBoundRatio, &status);
		
	if(valueRangeSize <= realPrecision)
	{
		SZ_compress_args_float_withinRange(newByteData, oriData, dataLength, outSize);
	}
	else
	{
//		SZ_compress_args_float_NoCkRngeNoGzip_2D(newByteData, oriData, r2, r1, realPrecision, outSize);
		if(r5==0&&r4==0&&r3==0&&r2==0)
		{
			if(errBoundMode>=PW_REL)
			{	
				//SZ_compress_args_float_NoCkRngeNoGzip_1D_pwr(newByteData, oriData, realPrecision, r1, outSize, min, max);
				SZ_compress_args_float_NoCkRngeNoGzip_1D_pwrgroup(newByteData, oriData, r1, absErr_Bound, relBoundRatio, pwrErrRatio, valueRangeSize, medianValue, outSize);
			}
			else
				SZ_compress_args_float_NoCkRngeNoGzip_1D(newByteData, oriData, r1, realPrecision, outSize, valueRangeSize, medianValue);
		}
		else if(r5==0&&r4==0&&r3==0)
		{
			if(errBoundMode>=PW_REL)
				SZ_compress_args_float_NoCkRngeNoGzip_2D_pwr(newByteData, oriData, realPrecision, r2, r1, outSize, min, max);
			else
				SZ_compress_args_float_NoCkRngeNoGzip_2D(newByteData, oriData, r2, r1, realPrecision, outSize, valueRangeSize, medianValue);
		}
		else if(r5==0&&r4==0)
		{
			if(errBoundMode>=PW_REL)
				SZ_compress_args_float_NoCkRngeNoGzip_3D_pwr(newByteData, oriData, realPrecision, r3, r2, r1, outSize, min, max);
			else
				SZ_compress_args_float_NoCkRngeNoGzip_3D(newByteData, oriData, r3, r2, r1, realPrecision, outSize, valueRangeSize, medianValue);
		}
		else if(r5==0)
		{
			if(errBoundMode>=PW_REL)
				SZ_compress_args_float_NoCkRngeNoGzip_3D_pwr(newByteData, oriData, realPrecision, r4*r3, r2, r1, outSize, min, max);
			else
				SZ_compress_args_float_NoCkRngeNoGzip_3D(newByteData, oriData, r4*r3, r2, r1, realPrecision, outSize, valueRangeSize, medianValue);
		}
	}
	return status;
}

int SZ_compress_args_float(unsigned char** newByteData, float *oriData, 
size_t r5, size_t r4, size_t r3, size_t r2, size_t r1, size_t *outSize, 
int errBoundMode, double absErr_Bound, double relBoundRatio, double pwRelBoundRatio, int pwrType)
{
	conf_params->errorBoundMode = errBoundMode;
	if(errBoundMode==PW_REL)
	{
		conf_params->pw_relBoundRatio = pwRelBoundRatio;	
		conf_params->pwr_type = pwrType;
		if(pwrType==SZ_PWR_AVG_TYPE && r3 != 0 )
		{
			printf("Error: Current version doesn't support 3D data compression with point-wise relative error bound being based on pwrType=AVG\n");
			exit(0);
			return SZ_NSCS;
		}
	}
	int status = SZ_SCES;
	size_t dataLength = computeDataLength(r5,r4,r3,r2,r1);
	
	if(dataLength <= MIN_NUM_OF_ELEMENTS)
	{
		*newByteData = SZ_skip_compress_float(oriData, dataLength, outSize);
		return status;
	}
	
	float valueRangeSize = 0, medianValue = 0;
	
	float min = computeRangeSize_float(oriData, dataLength, &valueRangeSize, &medianValue);
	float max = min+valueRangeSize;
	double realPrecision = 0; 
	
	if(conf_params->errorBoundMode==PSNR)
	{
		conf_params->errorBoundMode = ABS;
		realPrecision = conf_params->absErrBound = computeABSErrBoundFromPSNR(conf_params->psnr, (double)conf_params->predThreshold, (double)valueRangeSize);
		//printf("realPrecision=%lf\n", realPrecision);
	}
	else
		realPrecision = getRealPrecision_float(valueRangeSize, errBoundMode, absErr_Bound, relBoundRatio, &status);
		
	if(valueRangeSize <= realPrecision)
	{
		SZ_compress_args_float_withinRange(newByteData, oriData, dataLength, outSize);
	}
	else
	{
		size_t tmpOutSize = 0;
		unsigned char* tmpByteData;
		
		if (r2==0)
		{
			if(conf_params->errorBoundMode>=PW_REL)
			{
				//SZ_compress_args_float_NoCkRngeNoGzip_1D_pwr(&tmpByteData, oriData, realPrecision, r1, &tmpOutSize, min, max);
				SZ_compress_args_float_NoCkRngeNoGzip_1D_pwrgroup(&tmpByteData, oriData, r1, absErr_Bound, relBoundRatio, pwRelBoundRatio, 
				valueRangeSize, medianValue, &tmpOutSize);
			}
			else
#ifdef HAVE_TIMECMPR
				if(conf_params->szMode == SZ_TEMPORAL_COMPRESSION)
					multisteps->compressionType = SZ_compress_args_float_NoCkRngeNoGzip_1D(&tmpByteData, oriData, r1, realPrecision, &tmpOutSize, valueRangeSize, medianValue);
				else
#endif				
					SZ_compress_args_float_NoCkRngeNoGzip_1D(&tmpByteData, oriData, r1, realPrecision, &tmpOutSize, valueRangeSize, medianValue);
		}
		else
		if (r3==0)
		{			
			if(conf_params->errorBoundMode>=PW_REL)
				SZ_compress_args_float_NoCkRngeNoGzip_2D_pwr(&tmpByteData, oriData, realPrecision, r2, r1, &tmpOutSize, min, max);
			else
#ifdef HAVE_TIMECMPR
				if(conf_params->szMode == SZ_TEMPORAL_COMPRESSION)				
					multisteps->compressionType = SZ_compress_args_float_NoCkRngeNoGzip_2D(&tmpByteData, oriData, r2, r1, realPrecision, &tmpOutSize, valueRangeSize, medianValue);
				else
#endif
					SZ_compress_args_float_NoCkRngeNoGzip_2D(&tmpByteData, oriData, r2, r1, realPrecision, &tmpOutSize, valueRangeSize, medianValue);
		}
		else
		if (r4==0)
		{
			if(conf_params->errorBoundMode>=PW_REL)
				SZ_compress_args_float_NoCkRngeNoGzip_3D_pwr(&tmpByteData, oriData, realPrecision, r3, r2, r1, &tmpOutSize, min, max);
			else
#ifdef HAVE_TIMECMPR
				if(conf_params->szMode == SZ_TEMPORAL_COMPRESSION)				
					multisteps->compressionType = SZ_compress_args_float_NoCkRngeNoGzip_3D(&tmpByteData, oriData, r3, r2, r1, realPrecision, &tmpOutSize, valueRangeSize, medianValue);
				else
#endif
					SZ_compress_args_float_NoCkRngeNoGzip_3D(&tmpByteData, oriData, r3, r2, r1, realPrecision, &tmpOutSize, valueRangeSize, medianValue);
		}
		else
		if (r5==0)
		{
			if(conf_params->errorBoundMode>=PW_REL)		
				SZ_compress_args_float_NoCkRngeNoGzip_3D_pwr(&tmpByteData, oriData, realPrecision, r4*r3, r2, r1, &tmpOutSize, min, max);
				//ToDO
				//SZ_compress_args_float_NoCkRngeNoGzip_4D_pwr(&tmpByteData, oriData, r4, r3, r2, r1, &tmpOutSize, min, max);
			else
#ifdef HAVE_TIMECMPR
				if(conf_params->szMode == SZ_TEMPORAL_COMPRESSION)				
					multisteps->compressionType = SZ_compress_args_float_NoCkRngeNoGzip_4D(&tmpByteData, oriData, r4, r3, r2, r1, realPrecision, &tmpOutSize, valueRangeSize, medianValue);
				else
#endif
					SZ_compress_args_float_NoCkRngeNoGzip_4D(&tmpByteData, oriData, r4, r3, r2, r1, realPrecision, &tmpOutSize, valueRangeSize, medianValue);
		}
		else
		{
			printf("Error: doesn't support 5 dimensions for now.\n");
			status = SZ_DERR; //dimension error
		}
		//Call Gzip to do the further compression.
		if(conf_params->szMode==SZ_BEST_SPEED)
		{
			*outSize = tmpOutSize;
			*newByteData = tmpByteData;
		}
		else if(conf_params->szMode==SZ_BEST_COMPRESSION || conf_params->szMode==SZ_DEFAULT_COMPRESSION || conf_params->szMode==SZ_TEMPORAL_COMPRESSION)
		{
			*outSize = zlib_compress5(tmpByteData, tmpOutSize, newByteData, conf_params->gzipMode);
			free(tmpByteData);
		}
		else
		{
			printf("Error: Wrong setting of conf_params->szMode in the float compression.\n");
			status = SZ_MERR; //mode error			
		}
	}
	
	return status;
}


void computeReqLength_float(double realPrecision, short radExpo, int* reqLength, float* medianValue)
{
	short reqExpo = getPrecisionReqLength_double(realPrecision);
	*reqLength = 9+radExpo - reqExpo; //radExpo-reqExpo == reqMantiLength
	if(*reqLength<9)
		*reqLength = 9;
	if(*reqLength>32)
	{	
		*reqLength = 32;
		*medianValue = 0;
	}			
}

//TODO
int SZ_compress_args_float_subblock(unsigned char* compressedBytes, float *oriData,
size_t r5, size_t r4, size_t r3, size_t r2, size_t r1,
size_t s5, size_t s4, size_t s3, size_t s2, size_t s1,
size_t e5, size_t e4, size_t e3, size_t e2, size_t e1,
size_t *outSize, int errBoundMode, double absErr_Bound, double relBoundRatio)
{
	int status = SZ_SCES;
	float valueRangeSize = 0, medianValue = 0;
	computeRangeSize_float_subblock(oriData, &valueRangeSize, &medianValue, r5, r4, r3, r2, r1, s5, s4, s3, s2, s1, e5, e4, e3, e2, e1);

	double realPrecision = getRealPrecision_float(valueRangeSize, errBoundMode, absErr_Bound, relBoundRatio, &status);

	if(valueRangeSize <= realPrecision)
	{
		//TODO
		//SZ_compress_args_float_withinRange_subblock();
	}
	else
	{
		if (r2==0)
		{
			if(errBoundMode>=PW_REL)
			{
				//TODO
				//SZ_compress_args_float_NoCkRngeNoGzip_1D_pwr_subblock();
				printf ("Current subblock version does not support point-wise relative error bound.\n");
			}
			else
				SZ_compress_args_float_NoCkRnge_1D_subblock(compressedBytes, oriData, realPrecision, outSize, valueRangeSize, medianValue, r1, s1, e1);
		}
		else
		if (r3==0)
		{
			//TODO
			if(errBoundMode>=PW_REL)
			{
				//TODO
				//SZ_compress_args_float_NoCkRngeNoGzip_2D_pwr_subblock();
				printf ("Current subblock version does not support point-wise relative error bound.\n");
			}
			else
				SZ_compress_args_float_NoCkRnge_2D_subblock(compressedBytes, oriData, realPrecision, outSize, valueRangeSize, medianValue, r2, r1, s2, s1, e2, e1);
		}
		else
		if (r4==0)
		{
			if(errBoundMode>=PW_REL)
			{
				//TODO
				//SZ_compress_args_float_NoCkRngeNoGzip_3D_pwr_subblock();
				printf ("Current subblock version does not support point-wise relative error bound.\n");
			}
			else
				SZ_compress_args_float_NoCkRnge_3D_subblock(compressedBytes, oriData, realPrecision, outSize, valueRangeSize, medianValue, r3, r2, r1, s3, s2, s1, e3, e2, e1);
		}
		else
		if (r5==0)
		{
			if(errBoundMode>=PW_REL)
			{
				//TODO
				//SZ_compress_args_float_NoCkRngeNoGzip_4D_pwr_subblock();
				printf ("Current subblock version does not support point-wise relative error bound.\n");
			}
			else
				SZ_compress_args_float_NoCkRnge_4D_subblock(compressedBytes, oriData, realPrecision, outSize, valueRangeSize, medianValue, r4, r3, r2, r1, s4, s3, s2, s1, e4, e3, e2, e1);
		}
		else
		{
			printf("Error: doesn't support 5 dimensions for now.\n");
			status = SZ_DERR; //dimension error
		}
	}
	return status;
}

void SZ_compress_args_float_NoCkRnge_1D_subblock(unsigned char* compressedBytes, float *oriData, double realPrecision, size_t *outSize, float valueRangeSize, float medianValue_f,
size_t r1, size_t s1, size_t e1)
{
	TightDataPointStorageF* tdps = SZ_compress_float_1D_MDQ_subblock(oriData, realPrecision, valueRangeSize, medianValue_f, r1, s1, e1);

	if (conf_params->szMode==SZ_BEST_SPEED)
		convertTDPStoFlatBytes_float_args(tdps, compressedBytes, outSize);
	else if(conf_params->szMode==SZ_BEST_COMPRESSION || conf_params->szMode==SZ_DEFAULT_COMPRESSION)
	{
		unsigned char *tmpCompBytes;
		size_t tmpOutSize;
		convertTDPStoFlatBytes_float(tdps, &tmpCompBytes, &tmpOutSize);
		*outSize = zlib_compress3(tmpCompBytes, tmpOutSize, compressedBytes, conf_params->gzipMode);
		free(tmpCompBytes);
	}
	else
	{
		printf ("Error: Wrong setting of conf_params->szMode in the double compression.\n");
	}

	//TODO
//	if(*outSize>dataLength*sizeof(float))
//		SZ_compress_args_float_StoreOriData(oriData, dataLength, tdps, newByteData, outSize);

	free_TightDataPointStorageF(tdps);
}

void SZ_compress_args_float_NoCkRnge_2D_subblock(unsigned char* compressedBytes, float *oriData, double realPrecision, size_t *outSize, float valueRangeSize, float medianValue_f,
size_t r2, size_t r1, size_t s2, size_t s1, size_t e2, size_t e1)
{
	TightDataPointStorageF* tdps = SZ_compress_float_2D_MDQ_subblock(oriData, realPrecision, valueRangeSize, medianValue_f, r2, r1, s2, s1, e2, e1);

	if (conf_params->szMode==SZ_BEST_SPEED)
		convertTDPStoFlatBytes_float_args(tdps, compressedBytes, outSize);
	else if(conf_params->szMode==SZ_BEST_COMPRESSION || conf_params->szMode==SZ_DEFAULT_COMPRESSION)
	{
		unsigned char *tmpCompBytes;
		size_t tmpOutSize;
		convertTDPStoFlatBytes_float(tdps, &tmpCompBytes, &tmpOutSize);
		*outSize = zlib_compress3(tmpCompBytes, tmpOutSize, compressedBytes, conf_params->gzipMode);
		free(tmpCompBytes);
	}
	else
	{
		printf ("Error: Wrong setting of conf_params->szMode in the double compression.\n");
	}

	//TODO
//	if(*outSize>dataLength*sizeof(float))
//		SZ_compress_args_float_StoreOriData(oriData, dataLength, tdps, newByteData, outSize);

	free_TightDataPointStorageF(tdps);
}

void SZ_compress_args_float_NoCkRnge_3D_subblock(unsigned char* compressedBytes, float *oriData, double realPrecision, size_t *outSize, float valueRangeSize, float medianValue_f,
size_t r3, size_t r2, size_t r1, size_t s3, size_t s2, size_t s1, size_t e3, size_t e2, size_t e1)
{
	TightDataPointStorageF* tdps = SZ_compress_float_3D_MDQ_subblock(oriData, realPrecision, valueRangeSize, medianValue_f, r3, r2, r1, s3, s2, s1, e3, e2, e1);

	if (conf_params->szMode==SZ_BEST_SPEED)
		convertTDPStoFlatBytes_float_args(tdps, compressedBytes, outSize);
	else if(conf_params->szMode==SZ_BEST_COMPRESSION || conf_params->szMode==SZ_DEFAULT_COMPRESSION)
	{
		unsigned char *tmpCompBytes;
		size_t tmpOutSize;
		convertTDPStoFlatBytes_float(tdps, &tmpCompBytes, &tmpOutSize);
		*outSize = zlib_compress3(tmpCompBytes, tmpOutSize, compressedBytes, conf_params->gzipMode);
		free(tmpCompBytes);
	}
	else
	{
		printf ("Error: Wrong setting of conf_params->szMode in the double compression.\n");
	}

	//TODO
//	if(*outSize>dataLength*sizeof(float))
//		SZ_compress_args_float_StoreOriData(oriData, dataLength, tdps, newByteData, outSize);

	free_TightDataPointStorageF(tdps);
}

void SZ_compress_args_float_NoCkRnge_4D_subblock(unsigned char* compressedBytes, float *oriData, double realPrecision, size_t *outSize, float valueRangeSize, float medianValue_f,
size_t r4, size_t r3, size_t r2, size_t r1, size_t s4, size_t s3, size_t s2, size_t s1, size_t e4, size_t e3, size_t e2, size_t e1)
{
	TightDataPointStorageF* tdps = SZ_compress_float_4D_MDQ_subblock(oriData, realPrecision, valueRangeSize, medianValue_f, r4, r3, r2, r1, s4, s3, s2, s1, e4, e3, e2, e1);

	if (conf_params->szMode==SZ_BEST_SPEED)
		convertTDPStoFlatBytes_float_args(tdps, compressedBytes, outSize);
	else if(conf_params->szMode==SZ_BEST_COMPRESSION || conf_params->szMode==SZ_DEFAULT_COMPRESSION)
	{
		unsigned char *tmpCompBytes;
		size_t tmpOutSize;
		convertTDPStoFlatBytes_float(tdps, &tmpCompBytes, &tmpOutSize);
		*outSize = zlib_compress3(tmpCompBytes, tmpOutSize, compressedBytes, conf_params->gzipMode);
		free(tmpCompBytes);
	}
	else
	{
		printf ("Error: Wrong setting of conf_params->szMode in the double compression.\n");
	}

	//TODO
//	if(*outSize>dataLength*sizeof(float))
//		SZ_compress_args_float_StoreOriData(oriData, dataLength, tdps, newByteData, outSize);

	free_TightDataPointStorageF(tdps);

}

unsigned int optimize_intervals_float_1D_subblock(float *oriData, double realPrecision, size_t r1, size_t s1, size_t e1)
{
	size_t dataLength = e1 - s1 + 1;
	oriData = oriData + s1;

	size_t i = 0;
	unsigned long radiusIndex;
	float pred_value = 0, pred_err;
	int *intervals = (int*)malloc(conf_params->maxRangeRadius*sizeof(int));
	memset(intervals, 0, conf_params->maxRangeRadius*sizeof(int));
	size_t totalSampleSize = dataLength/conf_params->sampleDistance;
	for(i=2;i<dataLength;i++)
	{
		if(i%conf_params->sampleDistance==0)
		{
			pred_value = 2*oriData[i-1] - oriData[i-2];
			//pred_value = oriData[i-1];
			pred_err = fabs(pred_value - oriData[i]);
			radiusIndex = (unsigned long)((pred_err/realPrecision+1)/2);
			if(radiusIndex>=conf_params->maxRangeRadius)
				radiusIndex = conf_params->maxRangeRadius - 1;
			intervals[radiusIndex]++;
		}
	}
	//compute the appropriate number
	size_t targetCount = totalSampleSize*conf_params->predThreshold;
	size_t sum = 0;
	for(i=0;i<conf_params->maxRangeRadius;i++)
	{
		sum += intervals[i];
		if(sum>targetCount)
			break;
	}
	if(i>=conf_params->maxRangeRadius)
		i = conf_params->maxRangeRadius-1;

	unsigned int accIntervals = 2*(i+1);
	unsigned int powerOf2 = roundUpToPowerOf2(accIntervals);

	if(powerOf2<32)
		powerOf2 = 32;

	free(intervals);
	//printf("accIntervals=%d, powerOf2=%d\n", accIntervals, powerOf2);
	return powerOf2;
}

unsigned int optimize_intervals_float_2D_subblock(float *oriData, double realPrecision, size_t r1, size_t r2, size_t s1, size_t s2, size_t e1, size_t e2)
{
	size_t R1 = e1 - s1 + 1;
	size_t R2 = e2 - s2 + 1;

	size_t i,j, index;
	unsigned long radiusIndex;
	float pred_value = 0, pred_err;
	int *intervals = (int*)malloc(conf_params->maxRangeRadius*sizeof(int));
	memset(intervals, 0, conf_params->maxRangeRadius*sizeof(int));
	size_t totalSampleSize = R1*R2/conf_params->sampleDistance;
	for(i=s1+1;i<=e1;i++)
	{
		for(j=s2+1;j<=e2;j++)
		{
			if((i+j)%conf_params->sampleDistance==0)
			{
				index = i*r2+j;
				pred_value = oriData[index-1] + oriData[index-r2] - oriData[index-r2-1];
				pred_err = fabs(pred_value - oriData[index]);
				radiusIndex = (unsigned long)((pred_err/realPrecision+1)/2);
				if(radiusIndex>=conf_params->maxRangeRadius)
					radiusIndex = conf_params->maxRangeRadius - 1;
				intervals[radiusIndex]++;
			}
		}
	}
	//compute the appropriate number
	size_t targetCount = totalSampleSize*conf_params->predThreshold;
	size_t sum = 0;
	for(i=0;i<conf_params->maxRangeRadius;i++)
	{
		sum += intervals[i];
		if(sum>targetCount)
			break;
	}
	if(i>=conf_params->maxRangeRadius)
		i = conf_params->maxRangeRadius-1;
	unsigned int accIntervals = 2*(i+1);
	unsigned int powerOf2 = roundUpToPowerOf2(accIntervals);

	if(powerOf2<32)
		powerOf2 = 32;

	free(intervals);
	//printf("conf_params->maxRangeRadius = %d, accIntervals=%d, powerOf2=%d\n", conf_params->maxRangeRadius, accIntervals, powerOf2);
	return powerOf2;
}

unsigned int optimize_intervals_float_3D_subblock(float *oriData, double realPrecision, size_t r1, size_t r2, size_t r3, size_t s1, size_t s2, size_t s3, size_t e1, size_t e2, size_t e3)
{
	size_t R1 = e1 - s1 + 1;
	size_t R2 = e2 - s2 + 1;
	size_t R3 = e3 - s3 + 1;

	size_t r23 = r2*r3;

	size_t i,j,k, index;
	unsigned long radiusIndex;
	float pred_value = 0, pred_err;
	int *intervals = (int*)malloc(conf_params->maxRangeRadius*sizeof(int));
	memset(intervals, 0, conf_params->maxRangeRadius*sizeof(int));
	size_t totalSampleSize = R1*R2*R3/conf_params->sampleDistance;
	for(i=s1+1;i<=e1;i++)
	{
		for(j=s2+1;j<=e2;j++)
		{
			for(k=s3+1;k<=e3;k++)
			{
				if((i+j+k)%conf_params->sampleDistance==0)
				{
					index = i*r23+j*r3+k;
					pred_value = oriData[index-1] + oriData[index-r3] + oriData[index-r23]
					- oriData[index-1-r23] - oriData[index-r3-1] - oriData[index-r3-r23] + oriData[index-r3-r23-1];
					pred_err = fabs(pred_value - oriData[index]);
					radiusIndex = (unsigned long)((pred_err/realPrecision+1)/2);
					if(radiusIndex>=conf_params->maxRangeRadius)
						radiusIndex = conf_params->maxRangeRadius - 1;
					intervals[radiusIndex]++;
				}
			}
		}
	}
	//compute the appropriate number
	size_t targetCount = totalSampleSize*conf_params->predThreshold;
	size_t sum = 0;
	for(i=0;i<conf_params->maxRangeRadius;i++)
	{
		sum += intervals[i];
		if(sum>targetCount)
			break;
	}
	if(i>=conf_params->maxRangeRadius)
		i = conf_params->maxRangeRadius-1;
	unsigned int accIntervals = 2*(i+1);
	unsigned int powerOf2 = roundUpToPowerOf2(accIntervals);

	if(powerOf2<32)
		powerOf2 = 32;

	free(intervals);
	return powerOf2;
}

unsigned int optimize_intervals_float_4D_subblock(float *oriData, double realPrecision,
size_t r1, size_t r2, size_t r3, size_t r4, size_t s1, size_t s2, size_t s3, size_t s4, size_t e1, size_t e2, size_t e3, size_t e4)
{
	size_t R1 = e1 - s1 + 1;
	size_t R2 = e2 - s2 + 1;
	size_t R3 = e3 - s3 + 1;
	size_t R4 = e4 - s4 + 1;

	size_t r34 = r3*r4;
	size_t r234 = r2*r3*r4;

	size_t i,j,k,l, index;
	unsigned long radiusIndex;
	float pred_value = 0, pred_err;
	int *intervals = (int*)malloc(conf_params->maxRangeRadius*sizeof(int));
	memset(intervals, 0, conf_params->maxRangeRadius*sizeof(int));
	size_t totalSampleSize = R1*R2*R3*R4/conf_params->sampleDistance;
	for(i=s1+1;i<=e1;i++)
	{
		for(j=s2+1;j<=e2;j++)
		{
			for(k=s3+1;k<=e3;k++)
			{
				for (l=s4+1;l<=e4;l++)
				{
					if((i+j+k+l)%conf_params->sampleDistance==0)
					{
						index = i*r234+j*r34+k*r4+l;
						pred_value = oriData[index-1] + oriData[index-r4] + oriData[index-r34]
									- oriData[index-1-r34] - oriData[index-r4-1] - oriData[index-r4-r34] + oriData[index-r4-r34-1];
						pred_err = fabs(pred_value - oriData[index]);
						radiusIndex = (unsigned long)((pred_err/realPrecision+1)/2);
						if(radiusIndex>=conf_params->maxRangeRadius)
							radiusIndex = conf_params->maxRangeRadius - 1;
						intervals[radiusIndex]++;
					}
				}
			}
		}
	}
	//compute the appropriate number
	size_t targetCount = totalSampleSize*conf_params->predThreshold;
	size_t sum = 0;
	for(i=0;i<conf_params->maxRangeRadius;i++)
	{
		sum += intervals[i];
		if(sum>targetCount)
			break;
	}
	if(i>=conf_params->maxRangeRadius)
		i = conf_params->maxRangeRadius-1;

	unsigned int accIntervals = 2*(i+1);
	unsigned int powerOf2 = roundUpToPowerOf2(accIntervals);

	if(powerOf2<32)
		powerOf2 = 32;

	free(intervals);
	return powerOf2;
}

TightDataPointStorageF* SZ_compress_float_1D_MDQ_subblock(float *oriData, double realPrecision, float valueRangeSize, float medianValue_f,
size_t r1, size_t s1, size_t e1)
{
	size_t dataLength = e1 - s1 + 1;
	unsigned int quantization_intervals;
	if(exe_params->optQuantMode==1)
		quantization_intervals = optimize_intervals_float_1D_subblock(oriData, realPrecision, r1, s1, e1);
	else
		quantization_intervals = exe_params->intvCapacity;
	updateQuantizationInfo(quantization_intervals);

	size_t i; 
	int reqLength;
	float medianValue = medianValue_f;
	short radExpo = getExponent_float(valueRangeSize/2);

	computeReqLength_float(realPrecision, radExpo, &reqLength, &medianValue);

	int* type = (int*) malloc(dataLength*sizeof(int));

	float* spaceFillingValue = oriData + s1;

	DynamicIntArray *exactLeadNumArray;
	new_DIA(&exactLeadNumArray, DynArrayInitLen);

	DynamicByteArray *exactMidByteArray;
	new_DBA(&exactMidByteArray, DynArrayInitLen);

	DynamicIntArray *resiBitArray;
	new_DIA(&resiBitArray, DynArrayInitLen);

	type[0] = 0;

	unsigned char preDataBytes[4];
	intToBytes_bigEndian(preDataBytes, 0);

	int reqBytesLength = reqLength/8;
	int resiBitsLength = reqLength%8;
	float last3CmprsData[3] = {0};

	FloatValueCompressElement *vce = (FloatValueCompressElement*)malloc(sizeof(FloatValueCompressElement));
	LossyCompressionElement *lce = (LossyCompressionElement*)malloc(sizeof(LossyCompressionElement));

	//add the first data
	compressSingleFloatValue(vce, spaceFillingValue[0], realPrecision, medianValue, reqLength, reqBytesLength, resiBitsLength);
	updateLossyCompElement_Float(vce->curBytes, preDataBytes, reqBytesLength, resiBitsLength, lce);
	memcpy(preDataBytes,vce->curBytes,4);
	addExactData(exactMidByteArray, exactLeadNumArray, resiBitArray, lce);
	listAdd_float(last3CmprsData, vce->data);

	//add the second data
	type[1] = 0;
	compressSingleFloatValue(vce, spaceFillingValue[1], realPrecision, medianValue, reqLength, reqBytesLength, resiBitsLength);
	updateLossyCompElement_Float(vce->curBytes, preDataBytes, reqBytesLength, resiBitsLength, lce);
	memcpy(preDataBytes,vce->curBytes,4);
	addExactData(exactMidByteArray, exactLeadNumArray, resiBitArray, lce);
	listAdd_float(last3CmprsData, vce->data);

	int state;
	double checkRadius;
	float curData;
	float pred;
	float predAbsErr;
	checkRadius = (exe_params->intvCapacity-1)*realPrecision;
	double interval = 2*realPrecision;

	for(i=2;i<dataLength;i++)
	{
		curData = spaceFillingValue[i];
		pred = 2*last3CmprsData[0] - last3CmprsData[1];
		predAbsErr = fabs(curData - pred);
		if(predAbsErr<=checkRadius)
		{
			state = (predAbsErr/realPrecision+1)/2;
			if(curData>=pred)
			{
				type[i] = exe_params->intvRadius+state;
				pred = pred + state*interval;
			}
			else
			{
				type[i] = exe_params->intvRadius-state;
				pred = pred - state*interval;
			}

			listAdd_float(last3CmprsData, pred);
			continue;
		}

		//unpredictable data processing
		type[i] = 0;
		compressSingleFloatValue(vce, curData, realPrecision, medianValue, reqLength, reqBytesLength, resiBitsLength);
		updateLossyCompElement_Float(vce->curBytes, preDataBytes, reqBytesLength, resiBitsLength, lce);
		memcpy(preDataBytes,vce->curBytes,4);
		addExactData(exactMidByteArray, exactLeadNumArray, resiBitArray, lce);

		listAdd_float(last3CmprsData, vce->data);
	}

	size_t exactDataNum = exactLeadNumArray->size;

	TightDataPointStorageF* tdps;

	new_TightDataPointStorageF(&tdps, dataLength, exactDataNum,
			type, exactMidByteArray->array, exactMidByteArray->size,
			exactLeadNumArray->array,
			resiBitArray->array, resiBitArray->size,
			resiBitsLength,
			realPrecision, medianValue, (char)reqLength, quantization_intervals, NULL, 0, 0);

	//free memory
	free_DIA(exactLeadNumArray);
	free_DIA(resiBitArray);
	free(type);
	free(vce);
	free(lce);
	free(exactMidByteArray); //exactMidByteArray->array has been released in free_TightDataPointStorageF(tdps);

	return tdps;
}

TightDataPointStorageF* SZ_compress_float_2D_MDQ_subblock(float *oriData, double realPrecision, float valueRangeSize, float medianValue_f,
size_t r1, size_t r2, size_t s1, size_t s2, size_t e1, size_t e2)
{
	unsigned int quantization_intervals;
	if(exe_params->optQuantMode==1)
	{
		quantization_intervals = optimize_intervals_float_2D_subblock(oriData, realPrecision, r1, r2, s1, s2, e1, e2);
		updateQuantizationInfo(quantization_intervals);
	}
	else
		quantization_intervals = exe_params->intvCapacity;

	size_t i,j; 
	int reqLength;
	float pred1D, pred2D;
	float diff = 0.0;
	double itvNum = 0;
	float *P0, *P1;

	size_t R1 = e1 - s1 + 1;
	size_t R2 = e2 - s2 + 1;
	size_t dataLength = R1*R2;

	P0 = (float*)malloc(R2*sizeof(float));
	memset(P0, 0, R2*sizeof(float));
	P1 = (float*)malloc(R2*sizeof(float));
	memset(P1, 0, R2*sizeof(float));

	float medianValue = medianValue_f;
	short radExpo = getExponent_float(valueRangeSize/2);
	computeReqLength_float(realPrecision, radExpo, &reqLength, &medianValue);

	int* type = (int*) malloc(dataLength*sizeof(int));

	float* spaceFillingValue = oriData; //

	DynamicIntArray *exactLeadNumArray;
	new_DIA(&exactLeadNumArray, DynArrayInitLen);

	DynamicByteArray *exactMidByteArray;
	new_DBA(&exactMidByteArray, DynArrayInitLen);

	DynamicIntArray *resiBitArray;
	new_DIA(&resiBitArray, DynArrayInitLen);

	unsigned char preDataBytes[4];
	intToBytes_bigEndian(preDataBytes, 0);

	int reqBytesLength = reqLength/8;
	int resiBitsLength = reqLength%8;

	FloatValueCompressElement *vce = (FloatValueCompressElement*)malloc(sizeof(FloatValueCompressElement));
	LossyCompressionElement *lce = (LossyCompressionElement*)malloc(sizeof(LossyCompressionElement));

	/* Process Row-s1 data s2*/
	size_t gIndex;
	size_t lIndex;

	gIndex = s1*r2+s2;
	lIndex = 0;

	type[lIndex] = 0;
	compressSingleFloatValue(vce, spaceFillingValue[gIndex], realPrecision, medianValue, reqLength, reqBytesLength, resiBitsLength);
	updateLossyCompElement_Float(vce->curBytes, preDataBytes, reqBytesLength, resiBitsLength, lce);
	memcpy(preDataBytes,vce->curBytes,4);
	addExactData(exactMidByteArray, exactLeadNumArray, resiBitArray, lce);
	P1[0] = vce->data;

	/* Process Row-s1 data s2+1*/
	gIndex = s1*r2+(s2+1);
	lIndex = 1;

	pred1D = P1[0];
	diff = spaceFillingValue[gIndex] - pred1D;

	itvNum =  fabs(diff)/realPrecision + 1;

	if (itvNum < exe_params->intvCapacity)
	{
		if (diff < 0) itvNum = -itvNum;
		type[lIndex] = (int) (itvNum/2) + exe_params->intvRadius;
		P1[1] = pred1D + 2 * (type[lIndex] - exe_params->intvRadius) * realPrecision;
	}
	else
	{
		type[lIndex] = 0;
		compressSingleFloatValue(vce, spaceFillingValue[gIndex], realPrecision, medianValue, reqLength, reqBytesLength, resiBitsLength);
		updateLossyCompElement_Float(vce->curBytes, preDataBytes, reqBytesLength, resiBitsLength, lce);
		memcpy(preDataBytes,vce->curBytes,4);
		addExactData(exactMidByteArray, exactLeadNumArray, resiBitArray, lce);
		P1[1] = vce->data;
	}

    /* Process Row-s1 data s2+2 --> data e2 */
	for (j = 2; j < R2; j++)
	{
		gIndex = s1*r2+(s2+j);
		lIndex = j;

		pred1D = 2*P1[j-1] - P1[j-2];
		diff = spaceFillingValue[gIndex] - pred1D;

		itvNum = fabs(diff)/realPrecision + 1;

		if (itvNum < exe_params->intvCapacity)
		{
			if (diff < 0) itvNum = -itvNum;
			type[lIndex] = (int) (itvNum/2) + exe_params->intvRadius;
			P1[j] = pred1D + 2 * (type[lIndex] - exe_params->intvRadius) * realPrecision;
		}
		else
		{
			type[lIndex] = 0;
			compressSingleFloatValue(vce, spaceFillingValue[gIndex], realPrecision, medianValue, reqLength, reqBytesLength, resiBitsLength);
			updateLossyCompElement_Float(vce->curBytes, preDataBytes, reqBytesLength, resiBitsLength, lce);
			memcpy(preDataBytes,vce->curBytes,4);
			addExactData(exactMidByteArray, exactLeadNumArray, resiBitArray, lce);
			P1[j] = vce->data;
		}
	}

	/* Process Row-s1+1 --> Row-e1 */
	for (i = 1; i < R1; i++)
	{
		/* Process row-s1+i data s2 */
		gIndex = (s1+i)*r2+s2;
		lIndex = i*R2;

		pred1D = P1[0];
		diff = spaceFillingValue[gIndex] - pred1D;

		itvNum = fabs(diff)/realPrecision + 1;

		if (itvNum < exe_params->intvCapacity)
		{
			if (diff < 0) itvNum = -itvNum;
			type[lIndex] = (int) (itvNum/2) + exe_params->intvRadius;
			P0[0] = pred1D + 2 * (type[lIndex] - exe_params->intvRadius) * realPrecision;
		}
		else
		{
			type[lIndex] = 0;
			compressSingleFloatValue(vce, spaceFillingValue[gIndex], realPrecision, medianValue, reqLength, reqBytesLength, resiBitsLength);
			updateLossyCompElement_Float(vce->curBytes, preDataBytes, reqBytesLength, resiBitsLength, lce);
			memcpy(preDataBytes,vce->curBytes,4);
			addExactData(exactMidByteArray, exactLeadNumArray, resiBitArray, lce);
			P0[0] = vce->data;
		}

		/* Process row-s1+i data s2+1 --> e2 */
		for (j = 1; j < R2; j++)
		{
			gIndex = (s1+i)*r2+(s2+j);
			lIndex = i*R2+j;

//			printf ("global index = %d, local index = %d\n", gIndex, lIndex);

			pred2D = P0[j-1] + P1[j] - P1[j-1];

			diff = spaceFillingValue[gIndex] - pred2D;

			itvNum = fabs(diff)/realPrecision + 1;

			if (itvNum < exe_params->intvCapacity)
			{
				if (diff < 0) itvNum = -itvNum;
				type[lIndex] = (int) (itvNum/2) + exe_params->intvRadius;
				P0[j] = pred2D + 2 * (type[lIndex] - exe_params->intvRadius) * realPrecision;
			}
			else
			{
				type[lIndex] = 0;
				compressSingleFloatValue(vce, spaceFillingValue[gIndex], realPrecision, medianValue, reqLength, reqBytesLength, resiBitsLength);
				updateLossyCompElement_Float(vce->curBytes, preDataBytes, reqBytesLength, resiBitsLength, lce);
				memcpy(preDataBytes,vce->curBytes,4);
				addExactData(exactMidByteArray, exactLeadNumArray, resiBitArray, lce);
				P0[j] = vce->data;
			}
		}

		float *Pt;
		Pt = P1;
		P1 = P0;
		P0 = Pt;
	}

	free(P0);
	free(P1);
	size_t exactDataNum = exactLeadNumArray->size;

	TightDataPointStorageF* tdps;

	new_TightDataPointStorageF(&tdps, dataLength, exactDataNum,
			type, exactMidByteArray->array, exactMidByteArray->size,
			exactLeadNumArray->array,
			resiBitArray->array, resiBitArray->size,
			resiBitsLength,
			realPrecision, medianValue, (char)reqLength, quantization_intervals, NULL, 0, 0);

	//free memory
	free_DIA(exactLeadNumArray);
	free_DIA(resiBitArray);
	free(type);
	free(vce);
	free(lce);
	free(exactMidByteArray); //exactMidByteArray->array has been released in free_TightDataPointStorageF(tdps);

	return tdps;
}

TightDataPointStorageF* SZ_compress_float_3D_MDQ_subblock(float *oriData, double realPrecision, float valueRangeSize, float medianValue_f,
size_t r1, size_t r2, size_t r3, size_t s1, size_t s2, size_t s3, size_t e1, size_t e2, size_t e3)
{
	unsigned int quantization_intervals;
	if(exe_params->optQuantMode==1)
	{
		quantization_intervals = optimize_intervals_float_3D_subblock(oriData, realPrecision, r1, r2, r3, s1, s2, s3, e1, e2, e3);
		updateQuantizationInfo(quantization_intervals);
	}
	else
		quantization_intervals = exe_params->intvCapacity;

	size_t i,j,k; 
	int reqLength;
	float pred1D, pred2D, pred3D;
	float diff = 0.0;
	double itvNum = 0;
	float *P0, *P1;

	size_t R1 = e1 - s1 + 1;
	size_t R2 = e2 - s2 + 1;
	size_t R3 = e3 - s3 + 1;
	size_t dataLength = R1*R2*R3;

	size_t r23 = r2*r3;
	size_t R23 = R2*R3;

	P0 = (float*)malloc(R23*sizeof(float));
	P1 = (float*)malloc(R23*sizeof(float));

	float medianValue = medianValue_f;
	short radExpo = getExponent_float(valueRangeSize/2);
	computeReqLength_float(realPrecision, radExpo, &reqLength, &medianValue);

	int* type = (int*) malloc(dataLength*sizeof(int));
	//type[dataLength]=0;

	float* spaceFillingValue = oriData; //

	DynamicIntArray *exactLeadNumArray;
	new_DIA(&exactLeadNumArray, DynArrayInitLen);

	DynamicByteArray *exactMidByteArray;
	new_DBA(&exactMidByteArray, DynArrayInitLen);

	DynamicIntArray *resiBitArray;
	new_DIA(&resiBitArray, DynArrayInitLen);

	unsigned char preDataBytes[4];
	intToBytes_bigEndian(preDataBytes, 0);

	int reqBytesLength = reqLength/8;
	int resiBitsLength = reqLength%8;

	FloatValueCompressElement *vce = (FloatValueCompressElement*)malloc(sizeof(FloatValueCompressElement));
	LossyCompressionElement *lce = (LossyCompressionElement*)malloc(sizeof(LossyCompressionElement));


	///////////////////////////	Process layer-s1 ///////////////////////////
	/* Process Row-s2 data s3*/
	size_t gIndex; 	//global index
	size_t lIndex; 	//local index
	size_t index2D; 	//local 2D index

	gIndex = s1*r23+s2*r3+s3;
	lIndex = 0;
	index2D = 0;

	type[lIndex] = 0;
	compressSingleFloatValue(vce, spaceFillingValue[gIndex], realPrecision, medianValue, reqLength, reqBytesLength, resiBitsLength);
	updateLossyCompElement_Float(vce->curBytes, preDataBytes, reqBytesLength, resiBitsLength, lce);
	memcpy(preDataBytes,vce->curBytes,4);
	addExactData(exactMidByteArray, exactLeadNumArray, resiBitArray, lce);
	P1[index2D] = vce->data;

	/* Process Row-s2 data s3+1*/
	gIndex = s1*r23+s2*r3+s3+1;
	lIndex = 1;
	index2D = 1;

	pred1D = P1[index2D-1];
	diff = spaceFillingValue[gIndex] - pred1D;

	itvNum = fabs(diff)/realPrecision + 1;

	if (itvNum < exe_params->intvCapacity)
	{
		if (diff < 0) itvNum = -itvNum;
		type[lIndex] = (int) (itvNum/2) + exe_params->intvRadius;
		P1[index2D] = pred1D + 2 * (type[lIndex] - exe_params->intvRadius) * realPrecision;
	}
	else
	{
		type[lIndex] = 0;
		compressSingleFloatValue(vce, spaceFillingValue[gIndex], realPrecision, medianValue, reqLength, reqBytesLength, resiBitsLength);
		updateLossyCompElement_Float(vce->curBytes, preDataBytes, reqBytesLength, resiBitsLength, lce);
		memcpy(preDataBytes,vce->curBytes,4);
		addExactData(exactMidByteArray, exactLeadNumArray, resiBitArray, lce);
		P1[index2D] = vce->data;
	}

    /* Process Row-s2 data s3+2 --> data e3 */
	for (j = 2; j < R3; j++)
	{
		gIndex = s1*r23+s2*r3+s3+j;
		lIndex = j;
		index2D = j;

		pred1D = 2*P1[index2D-1] - P1[index2D-2];
		diff = spaceFillingValue[gIndex] - pred1D;

		itvNum = fabs(diff)/realPrecision + 1;

		if (itvNum < exe_params->intvCapacity)
		{
			if (diff < 0) itvNum = -itvNum;
			type[lIndex] = (int) (itvNum/2) + exe_params->intvRadius;
			P1[index2D] = pred1D + 2 * (type[lIndex] - exe_params->intvRadius) * realPrecision;
		}
		else
		{
			type[lIndex] = 0;
			compressSingleFloatValue(vce, spaceFillingValue[gIndex], realPrecision, medianValue, reqLength, reqBytesLength, resiBitsLength);
			updateLossyCompElement_Float(vce->curBytes, preDataBytes, reqBytesLength, resiBitsLength, lce);
			memcpy(preDataBytes,vce->curBytes,4);
			addExactData(exactMidByteArray, exactLeadNumArray, resiBitArray, lce);
			P1[index2D] = vce->data;
		}
	}

	/* Process Row-s2+1 --> Row-e2 */
	for (i = 1; i < R2; i++)
	{
		/* Process row-s2+i data s3 */
		gIndex = s1*r23+(s2+i)*r3+s3;
		lIndex = i*R3;
		index2D = i*R3;

		pred1D  = P1[index2D-R3];
		diff = spaceFillingValue[gIndex] - pred1D;

		itvNum = fabs(diff)/realPrecision + 1;

		if (itvNum < exe_params->intvCapacity)
		{
			if (diff < 0) itvNum = -itvNum;
			type[lIndex] = (int) (itvNum/2) + exe_params->intvRadius;
			P1[index2D] = pred1D + 2 * (type[lIndex] - exe_params->intvRadius) * realPrecision;
		}
		else
		{
			type[lIndex] = 0;
			compressSingleFloatValue(vce, spaceFillingValue[gIndex], realPrecision, medianValue, reqLength, reqBytesLength, resiBitsLength);
			updateLossyCompElement_Float(vce->curBytes, preDataBytes, reqBytesLength, resiBitsLength, lce);
			memcpy(preDataBytes,vce->curBytes,4);
			addExactData(exactMidByteArray, exactLeadNumArray, resiBitArray, lce);
			P1[index2D] = vce->data;
		}

		/* Process row-s2+i data s3+1 --> data e3*/
		for (j = 1; j < R3; j++)
		{
			gIndex = s1*r23+(s2+i)*r3+s3+j;
			lIndex = i*R3+j;
			index2D = i*R3+j;

			pred2D  = P1[index2D-1] + P1[index2D-R3] - P1[index2D-R3-1];
			diff = spaceFillingValue[gIndex] - pred2D;

			itvNum = fabs(diff)/realPrecision + 1;

			if (itvNum < exe_params->intvCapacity)
			{
				if (diff < 0) itvNum = -itvNum;
				type[lIndex] = (int) (itvNum/2) + exe_params->intvRadius;
				P1[index2D] = pred2D + 2 * (type[lIndex] - exe_params->intvRadius) * realPrecision;
			}
			else
			{
				type[lIndex] = 0;
				compressSingleFloatValue(vce, spaceFillingValue[gIndex], realPrecision, medianValue, reqLength, reqBytesLength, resiBitsLength);
				updateLossyCompElement_Float(vce->curBytes, preDataBytes, reqBytesLength, resiBitsLength, lce);
				memcpy(preDataBytes,vce->curBytes,4);
				addExactData(exactMidByteArray, exactLeadNumArray, resiBitArray, lce);
				P1[index2D] = vce->data;
			}
		}
	}


	///////////////////////////	Process layer-s1+1 --> layer-e1 ///////////////////////////

	for (k = 1; k < R1; k++)
	{
		/* Process Row-s2 data s3*/
		gIndex = (s1+k)*r23+s2*r3+s3;
		lIndex = k*R23;
		index2D = 0;

		pred1D = P1[index2D];
		diff = spaceFillingValue[gIndex] - pred1D;

		itvNum = fabs(diff)/realPrecision + 1;

		if (itvNum < exe_params->intvCapacity)
		{
			if (diff < 0) itvNum = -itvNum;
			type[lIndex] = (int) (itvNum/2) + exe_params->intvRadius;
			P0[index2D] = pred1D + 2 * (type[lIndex] - exe_params->intvRadius) * realPrecision;
		}
		else
		{
			type[lIndex] = 0;
			compressSingleFloatValue(vce, spaceFillingValue[gIndex], realPrecision, medianValue, reqLength, reqBytesLength, resiBitsLength);
			updateLossyCompElement_Float(vce->curBytes, preDataBytes, reqBytesLength, resiBitsLength, lce);
			memcpy(preDataBytes,vce->curBytes,4);
			addExactData(exactMidByteArray, exactLeadNumArray, resiBitArray, lce);
			P0[index2D] = vce->data;
		}

	    /* Process Row-s2 data s3+1 --> data e3 */
		for (j = 1; j < R3; j++)
		{
			gIndex = (s1+k)*r23+s2*r3+s3+j;
			lIndex = k*R23+j;
			index2D = j;

			pred2D = P0[index2D-1] + P1[index2D] - P1[index2D-1];
			diff = spaceFillingValue[gIndex] - pred2D;

			itvNum = fabs(diff)/realPrecision + 1;

			if (itvNum < exe_params->intvCapacity)
			{
				if (diff < 0) itvNum = -itvNum;
				type[lIndex] = (int) (itvNum/2) + exe_params->intvRadius;
				P0[index2D] = pred2D + 2 * (type[lIndex] - exe_params->intvRadius) * realPrecision;
			}
			else
			{
				type[lIndex] = 0;
				compressSingleFloatValue(vce, spaceFillingValue[gIndex], realPrecision, medianValue, reqLength, reqBytesLength, resiBitsLength);
				updateLossyCompElement_Float(vce->curBytes, preDataBytes, reqBytesLength, resiBitsLength, lce);
				memcpy(preDataBytes,vce->curBytes,4);
				addExactData(exactMidByteArray, exactLeadNumArray, resiBitArray, lce);
				P0[index2D] = vce->data;
			}
		}

	    /* Process Row-s2+1 --> Row-e2 */
		for (i = 1; i < R2; i++)
		{
			/* Process Row-s2+i data s3 */
			gIndex = (s1+k)*r23+(s2+i)*r3+s3;
			lIndex = k*R23+i*R3;
			index2D = i*R3;

			pred2D = P0[index2D-R3] + P1[index2D] - P1[index2D-R3];
			diff = spaceFillingValue[gIndex] - pred2D;

			itvNum = fabs(diff)/realPrecision + 1;

			if (itvNum < exe_params->intvCapacity)
			{
				if (diff < 0) itvNum = -itvNum;
				type[lIndex] = (int) (itvNum/2) + exe_params->intvRadius;
				P0[index2D] = pred2D + 2 * (type[lIndex] - exe_params->intvRadius) * realPrecision;
			}
			else
			{
				type[lIndex] = 0;
				compressSingleFloatValue(vce, spaceFillingValue[gIndex], realPrecision, medianValue, reqLength, reqBytesLength, resiBitsLength);
				updateLossyCompElement_Float(vce->curBytes, preDataBytes, reqBytesLength, resiBitsLength, lce);
				memcpy(preDataBytes,vce->curBytes,4);
				addExactData(exactMidByteArray, exactLeadNumArray, resiBitArray, lce);
				P0[index2D] = vce->data;
			}

			/* Process Row-s2+i data s3+1 --> data e3 */
			for (j = 1; j < R3; j++)
			{
				gIndex = (s1+k)*r23+(s2+i)*r3+s3+j;
				lIndex = k*R23+i*R3+j;
				index2D = i*R3+j;

//				printf ("global index = %d, local index = %d\n", gIndex, lIndex);

				pred3D = P0[index2D-1] + P0[index2D-R3]+ P1[index2D] - P0[index2D-R3-1] - P1[index2D-R3] - P1[index2D-1] + P1[index2D-R3-1];
				diff = spaceFillingValue[gIndex] - pred3D;

				itvNum = fabs(diff)/realPrecision + 1;

				if (itvNum < exe_params->intvCapacity)
				{
					if (diff < 0) itvNum = -itvNum;
					type[lIndex] = (int) (itvNum/2) + exe_params->intvRadius;
					P0[index2D] = pred3D + 2 * (type[lIndex] - exe_params->intvRadius) * realPrecision;
				}
				else
				{
					type[lIndex] = 0;
					compressSingleFloatValue(vce, spaceFillingValue[gIndex], realPrecision, medianValue, reqLength, reqBytesLength, resiBitsLength);
					updateLossyCompElement_Float(vce->curBytes, preDataBytes, reqBytesLength, resiBitsLength, lce);
					memcpy(preDataBytes,vce->curBytes,4);
					addExactData(exactMidByteArray, exactLeadNumArray, resiBitArray, lce);
					P0[index2D] = vce->data;
				}
			}
		}

		float *Pt;
		Pt = P1;
		P1 = P0;
		P0 = Pt;
	}

	free(P0);
	free(P1);
	size_t exactDataNum = exactLeadNumArray->size;

	TightDataPointStorageF* tdps;

	new_TightDataPointStorageF(&tdps, dataLength, exactDataNum,
			type, exactMidByteArray->array, exactMidByteArray->size,
			exactLeadNumArray->array,
			resiBitArray->array, resiBitArray->size,
			resiBitsLength,
			realPrecision, medianValue, (char)reqLength, quantization_intervals, NULL, 0, 0);

	//free memory
	free_DIA(exactLeadNumArray);
	free_DIA(resiBitArray);
	free(type);
	free(vce);
	free(lce);
	free(exactMidByteArray); //exactMidByteArray->array has been released in free_TightDataPointStorageF(tdps);

	return tdps;
}

TightDataPointStorageF* SZ_compress_float_4D_MDQ_subblock(float *oriData, double realPrecision, float valueRangeSize, float medianValue_f,
size_t r1, size_t r2, size_t r3, size_t r4, size_t s1, size_t s2, size_t s3, size_t s4, size_t e1, size_t e2, size_t e3, size_t e4)
{
	unsigned int quantization_intervals;
	if(exe_params->optQuantMode==1)
	{
		quantization_intervals = optimize_intervals_float_4D_subblock(oriData, realPrecision, r1, r2, r3, r4, s1, s2, s3, s4, e1, e2, e3, e4);
		updateQuantizationInfo(quantization_intervals);
	}
	else
		quantization_intervals = exe_params->intvCapacity;

	size_t i,j,k; 
	int reqLength;
	float pred1D, pred2D, pred3D;
	float diff = 0.0;
	double itvNum = 0;
	float *P0, *P1;

	size_t R1 = e1 - s1 + 1;
	size_t R2 = e2 - s2 + 1;
	size_t R3 = e3 - s3 + 1;
	size_t R4 = e4 - s4 + 1;

	size_t dataLength = R1*R2*R3*R4;

	size_t r34 = r3*r4;
	size_t r234 = r2*r3*r4;
	size_t R34 = R3*R4;
	size_t R234 = R2*R3*R4;

	P0 = (float*)malloc(R34*sizeof(float));
	P1 = (float*)malloc(R34*sizeof(float));

	float medianValue = medianValue_f;
	short radExpo = getExponent_float(valueRangeSize/2);
	computeReqLength_float(realPrecision, radExpo, &reqLength, &medianValue);

	int* type = (int*) malloc(dataLength*sizeof(int));

	float* spaceFillingValue = oriData; //

	DynamicIntArray *exactLeadNumArray;
	new_DIA(&exactLeadNumArray, DynArrayInitLen);

	DynamicByteArray *exactMidByteArray;
	new_DBA(&exactMidByteArray, DynArrayInitLen);

	DynamicIntArray *resiBitArray;
	new_DIA(&resiBitArray, DynArrayInitLen);

	unsigned char preDataBytes[4];
	intToBytes_bigEndian(preDataBytes, 0);

	int reqBytesLength = reqLength/8;
	int resiBitsLength = reqLength%8;

	FloatValueCompressElement *vce = (FloatValueCompressElement*)malloc(sizeof(FloatValueCompressElement));
	LossyCompressionElement *lce = (LossyCompressionElement*)malloc(sizeof(LossyCompressionElement));


	size_t l;
	for (l = 0; l < R1; l++)
	{

		///////////////////////////	Process layer-s2 ///////////////////////////
		/* Process Row-s3 data s4*/
		size_t gIndex; 	//global index
		size_t lIndex; 	//local index
		size_t index2D; 	//local 2D index

		gIndex = (s1+l)*r234+s2*r34+s3*r4+s4;
		lIndex = l*R234;
		index2D = 0;

		type[lIndex] = 0;
		compressSingleFloatValue(vce, spaceFillingValue[gIndex], realPrecision, medianValue, reqLength, reqBytesLength, resiBitsLength);
		updateLossyCompElement_Float(vce->curBytes, preDataBytes, reqBytesLength, resiBitsLength, lce);
		memcpy(preDataBytes,vce->curBytes,4);
		addExactData(exactMidByteArray, exactLeadNumArray, resiBitArray, lce);
		P1[index2D] = vce->data;

		/* Process Row-s3 data s4+1*/
		gIndex = (s1+l)*r234+s2*r34+s3*r4+s4+1;
		lIndex = l*R234+1;
		index2D = 1;

		pred1D = P1[index2D-1];
		diff = spaceFillingValue[gIndex] - pred1D;

		itvNum = fabs(diff)/realPrecision + 1;

		if (itvNum < exe_params->intvCapacity)
		{
			if (diff < 0) itvNum = -itvNum;
			type[lIndex] = (int) (itvNum/2) + exe_params->intvRadius;
			P1[index2D] = pred1D + 2 * (type[lIndex] - exe_params->intvRadius) * realPrecision;
		}
		else
		{
			type[lIndex] = 0;
			compressSingleFloatValue(vce, spaceFillingValue[gIndex], realPrecision, medianValue, reqLength, reqBytesLength, resiBitsLength);
			updateLossyCompElement_Float(vce->curBytes, preDataBytes, reqBytesLength, resiBitsLength, lce);
			memcpy(preDataBytes,vce->curBytes,4);
			addExactData(exactMidByteArray, exactLeadNumArray, resiBitArray, lce);
			P1[index2D] = vce->data;
		}

		/* Process Row-s3 data s4+2 --> data e4 */
		for (j = 2; j < R4; j++)
		{
			gIndex = (s1+l)*r234+s2*r34+s3*r4+s4+j;
			lIndex = l*R234+j;
			index2D = j;

			pred1D = 2*P1[index2D-1] - P1[index2D-2];
			diff = spaceFillingValue[gIndex] - pred1D;

			itvNum = fabs(diff)/realPrecision + 1;

			if (itvNum < exe_params->intvCapacity)
			{
				if (diff < 0) itvNum = -itvNum;
				type[lIndex] = (int) (itvNum/2) + exe_params->intvRadius;
				P1[index2D] = pred1D + 2 * (type[lIndex] - exe_params->intvRadius) * realPrecision;
			}
			else
			{
				type[lIndex] = 0;
				compressSingleFloatValue(vce, spaceFillingValue[gIndex], realPrecision, medianValue, reqLength, reqBytesLength, resiBitsLength);
				updateLossyCompElement_Float(vce->curBytes, preDataBytes, reqBytesLength, resiBitsLength, lce);
				memcpy(preDataBytes,vce->curBytes,4);
				addExactData(exactMidByteArray, exactLeadNumArray, resiBitArray, lce);
				P1[index2D] = vce->data;
			}
		}

		/* Process Row-s3+1 --> Row-e3 */
		for (i = 1; i < R3; i++)
		{
			/* Process row-s2+i data s3 */
			gIndex = (s1+l)*r234+s2*r34+(s3+i)*r4+s4;
			lIndex = l*R234+i*R4;
			index2D = i*R4;

			pred1D  = P1[index2D-R4];
			diff = spaceFillingValue[gIndex] - pred1D;

			itvNum = fabs(diff)/realPrecision + 1;

			if (itvNum < exe_params->intvCapacity)
			{
				if (diff < 0) itvNum = -itvNum;
				type[lIndex] = (int) (itvNum/2) + exe_params->intvRadius;
				P1[index2D] = pred1D + 2 * (type[lIndex] - exe_params->intvRadius) * realPrecision;
			}
			else
			{
				type[lIndex] = 0;
				compressSingleFloatValue(vce, spaceFillingValue[gIndex], realPrecision, medianValue, reqLength, reqBytesLength, resiBitsLength);
				updateLossyCompElement_Float(vce->curBytes, preDataBytes, reqBytesLength, resiBitsLength, lce);
				memcpy(preDataBytes,vce->curBytes,4);
				addExactData(exactMidByteArray, exactLeadNumArray, resiBitArray, lce);
				P1[index2D] = vce->data;
			}

			/* Process row-s3+i data s4+1 --> data e4*/
			for (j = 1; j < R4; j++)
			{
				gIndex = (s1+l)*r234+s2*r34+(s3+i)*r4+s4+j;
				lIndex = l*R234+i*R4+j;
				index2D = i*R4+j;

				pred2D  = P1[index2D-1] + P1[index2D-R4] - P1[index2D-R4-1];
				diff = spaceFillingValue[gIndex] - pred2D;

				itvNum = fabs(diff)/realPrecision + 1;

				if (itvNum < exe_params->intvCapacity)
				{
					if (diff < 0) itvNum = -itvNum;
					type[lIndex] = (int) (itvNum/2) + exe_params->intvRadius;
					P1[index2D] = pred2D + 2 * (type[lIndex] - exe_params->intvRadius) * realPrecision;
				}
				else
				{
					type[lIndex] = 0;
					compressSingleFloatValue(vce, spaceFillingValue[gIndex], realPrecision, medianValue, reqLength, reqBytesLength, resiBitsLength);
					updateLossyCompElement_Float(vce->curBytes, preDataBytes, reqBytesLength, resiBitsLength, lce);
					memcpy(preDataBytes,vce->curBytes,4);
					addExactData(exactMidByteArray, exactLeadNumArray, resiBitArray, lce);
					P1[index2D] = vce->data;
				}
			}
		}


		///////////////////////////	Process layer-s2+1 --> layer-e2 ///////////////////////////

		for (k = 1; k < R2; k++)
		{
			/* Process Row-s3 data s4*/
			gIndex = (s1+l)*r234+(s2+k)*r34+s3*r4+s4;
			lIndex = l*R234+k*R34;
			index2D = 0;

			pred1D = P1[index2D];
			diff = spaceFillingValue[gIndex] - pred1D;

			itvNum = fabs(diff)/realPrecision + 1;

			if (itvNum < exe_params->intvCapacity)
			{
				if (diff < 0) itvNum = -itvNum;
				type[lIndex] = (int) (itvNum/2) + exe_params->intvRadius;
				P0[index2D] = pred1D + 2 * (type[lIndex] - exe_params->intvRadius) * realPrecision;
			}
			else
			{
				type[lIndex] = 0;
				compressSingleFloatValue(vce, spaceFillingValue[gIndex], realPrecision, medianValue, reqLength, reqBytesLength, resiBitsLength);
				updateLossyCompElement_Float(vce->curBytes, preDataBytes, reqBytesLength, resiBitsLength, lce);
				memcpy(preDataBytes,vce->curBytes,4);
				addExactData(exactMidByteArray, exactLeadNumArray, resiBitArray, lce);
				P0[index2D] = vce->data;
			}

			/* Process Row-s3 data s4+1 --> data e4 */
			for (j = 1; j < R4; j++)
			{
				gIndex = (s1+l)*r234+(s2+k)*r34+s3*r4+s4+j;
				lIndex = l*R234+k*R34+j;
				index2D = j;

				pred2D = P0[index2D-1] + P1[index2D] - P1[index2D-1];
				diff = spaceFillingValue[gIndex] - pred2D;

				itvNum = fabs(diff)/realPrecision + 1;

				if (itvNum < exe_params->intvCapacity)
				{
					if (diff < 0) itvNum = -itvNum;
					type[lIndex] = (int) (itvNum/2) + exe_params->intvRadius;
					P0[index2D] = pred2D + 2 * (type[lIndex] - exe_params->intvRadius) * realPrecision;
				}
				else
				{
					type[lIndex] = 0;
					compressSingleFloatValue(vce, spaceFillingValue[gIndex], realPrecision, medianValue, reqLength, reqBytesLength, resiBitsLength);
					updateLossyCompElement_Float(vce->curBytes, preDataBytes, reqBytesLength, resiBitsLength, lce);
					memcpy(preDataBytes,vce->curBytes,4);
					addExactData(exactMidByteArray, exactLeadNumArray, resiBitArray, lce);
					P0[index2D] = vce->data;
				}
			}

			/* Process Row-s3+1 --> Row-e3 */
			for (i = 1; i < R3; i++)
			{
				/* Process Row-s3+i data s4 */
				gIndex = (s1+l)*r234+(s2+k)*r34+(s3+i)*r4+s4;
				lIndex = l*R234+k*R34+i*R4;
				index2D = i*R4;

				pred2D = P0[index2D-R4] + P1[index2D] - P1[index2D-R4];
				diff = spaceFillingValue[gIndex] - pred2D;

				itvNum = fabs(diff)/realPrecision + 1;

				if (itvNum < exe_params->intvCapacity)
				{
					if (diff < 0) itvNum = -itvNum;
					type[lIndex] = (int) (itvNum/2) + exe_params->intvRadius;
					P0[index2D] = pred2D + 2 * (type[lIndex] - exe_params->intvRadius) * realPrecision;
				}
				else
				{
					type[lIndex] = 0;
					compressSingleFloatValue(vce, spaceFillingValue[gIndex], realPrecision, medianValue, reqLength, reqBytesLength, resiBitsLength);
					updateLossyCompElement_Float(vce->curBytes, preDataBytes, reqBytesLength, resiBitsLength, lce);
					memcpy(preDataBytes,vce->curBytes,4);
					addExactData(exactMidByteArray, exactLeadNumArray, resiBitArray, lce);
					P0[index2D] = vce->data;
				}

				/* Process Row-s3+i data s4+1 --> data e4 */
				for (j = 1; j < R4; j++)
				{
					gIndex = (s1+l)*r234+(s2+k)*r34+(s3+i)*r4+s4+j;
					lIndex = l*R234+k*R34+i*R4+j;
					index2D = i*R4+j;

//					printf ("global index = %d, local index = %d\n", gIndex, lIndex);

					pred3D = P0[index2D-1] + P0[index2D-R4]+ P1[index2D] - P0[index2D-R4-1] - P1[index2D-R4] - P1[index2D-1] + P1[index2D-R4-1];
					diff = spaceFillingValue[gIndex] - pred3D;

					itvNum = fabs(diff)/realPrecision + 1;

					if (itvNum < exe_params->intvCapacity)
					{
						if (diff < 0) itvNum = -itvNum;
						type[lIndex] = (int) (itvNum/2) + exe_params->intvRadius;
						P0[index2D] = pred3D + 2 * (type[lIndex] - exe_params->intvRadius) * realPrecision;
					}
					else
					{
						type[lIndex] = 0;
						compressSingleFloatValue(vce, spaceFillingValue[gIndex], realPrecision, medianValue, reqLength, reqBytesLength, resiBitsLength);
						updateLossyCompElement_Float(vce->curBytes, preDataBytes, reqBytesLength, resiBitsLength, lce);
						memcpy(preDataBytes,vce->curBytes,4);
						addExactData(exactMidByteArray, exactLeadNumArray, resiBitArray, lce);
						P0[index2D] = vce->data;
					}
				}
			}

			float *Pt;
			Pt = P1;
			P1 = P0;
			P0 = Pt;
		}

	}

	free(P0);
	free(P1);
	size_t exactDataNum = exactLeadNumArray->size;

	TightDataPointStorageF* tdps;

	new_TightDataPointStorageF(&tdps, dataLength, exactDataNum,
			type, exactMidByteArray->array, exactMidByteArray->size,
			exactLeadNumArray->array,
			resiBitArray->array, resiBitArray->size,
			resiBitsLength,
			realPrecision, medianValue, (char)reqLength, quantization_intervals, NULL, 0, 0);

	//free memory
	free_DIA(exactLeadNumArray);
	free_DIA(resiBitArray);
	free(type);
	free(vce);
	free(lce);
	free(exactMidByteArray); //exactMidByteArray->array has been released in free_TightDataPointStorageF(tdps);

	return tdps;
}

unsigned int optimize_intervals_float_3D_opt(float *oriData, size_t r1, size_t r2, size_t r3, double realPrecision)
{	
	size_t i;
	size_t radiusIndex;
	size_t r23=r2*r3;
	float pred_value = 0, pred_err;
	size_t *intervals = (size_t*)malloc(conf_params->maxRangeRadius*sizeof(size_t));
	memset(intervals, 0, conf_params->maxRangeRadius*sizeof(size_t));
	size_t totalSampleSize = 0;//(r1-1)*(r2-1)*(r3-1)/conf_params->sampleDistance;

	size_t offset_count = conf_params->sampleDistance - 2; // count r3 offset
	size_t offset_count_2;
	float * data_pos = oriData + r23 + r3 + offset_count;
	size_t n1_count = 1, n2_count = 1; // count i,j sum
	size_t len = r1 * r2 * r3;
	while(data_pos - oriData < len){
		totalSampleSize++;
		pred_value = data_pos[-1] + data_pos[-r3] + data_pos[-r23] - data_pos[-1-r23] - data_pos[-r3-1] - data_pos[-r3-r23] + data_pos[-r3-r23-1];
		pred_err = fabs(pred_value - *data_pos);
		radiusIndex = (pred_err/realPrecision+1)/2;
		if(radiusIndex>=conf_params->maxRangeRadius)
		{
			radiusIndex = conf_params->maxRangeRadius - 1;
			//printf("radiusIndex=%d\n", radiusIndex);
		}
		intervals[radiusIndex]++;
		// printf("TEST: %ld, i: %ld\tj: %ld\tk: %ld\n", data_pos - oriData);
		// fflush(stdout);
		offset_count += conf_params->sampleDistance;
		if(offset_count >= r3){
			n2_count ++;
			if(n2_count == r2){
				n1_count ++;
				n2_count = 1;
				data_pos += r3;
			}
			offset_count_2 = (n1_count + n2_count) % conf_params->sampleDistance;
			data_pos += (r3 + conf_params->sampleDistance - offset_count) + (conf_params->sampleDistance - offset_count_2);
			offset_count = (conf_params->sampleDistance - offset_count_2);
			if(offset_count == 0) offset_count ++;
		}
		else data_pos += conf_params->sampleDistance;
	}	
	// printf("sample_count: %ld\n", sample_count);
	// fflush(stdout);
	// if(*max_freq < 0.15) *max_freq *= 2;
	//compute the appropriate number
	size_t targetCount = totalSampleSize*conf_params->predThreshold;
	size_t sum = 0;
	for(i=0;i<conf_params->maxRangeRadius;i++)
	{
		sum += intervals[i];
		if(sum>targetCount)
			break;
	}
	if(i>=conf_params->maxRangeRadius)
		i = conf_params->maxRangeRadius-1;
	unsigned int accIntervals = 2*(i+1);
	unsigned int powerOf2 = roundUpToPowerOf2(accIntervals);

	if(powerOf2<32)
		powerOf2 = 32;
	free(intervals);
	//printf("targetCount=%d, sum=%d, totalSampleSize=%d, ratio=%f, accIntervals=%d, powerOf2=%d\n", targetCount, sum, totalSampleSize, (double)sum/(double)totalSampleSize, accIntervals, powerOf2);
	return powerOf2;
}

size_t SZ_compress_float_3D_MDQ_RA_block(float * block_ori_data, float * mean, size_t dim_0, size_t dim_1, size_t dim_2, size_t block_dim_0, size_t block_dim_1, size_t block_dim_2, double realPrecision, float * P0, float * P1, int * type, float * unpredictable_data){

	size_t dim0_offset = dim_1 * dim_2;
	size_t dim1_offset = dim_2;

	// data_pos = block_ori_data;
	// for(size_t i=0; i<block_dim_0; i++){
	// 	for(size_t j=0; j<block_dim_1; j++){
	// 		for(size_t k=0; k<block_dim_2; k++){
	// 			sum += *data_pos;
	// 			data_pos ++;
	// 		}
	// 		data_pos += dim1_offset - block_dim_2;
	// 	}
	// 	data_pos += dim0_offset - block_dim_1 * dim1_offset;
	// }
	// size_t num_elements = block_dim_0 * block_dim_1 * block_dim_2;
	// if(num_elements > 0) mean[0] = sum / num_elements;
	// else mean[0] = 0.0;
	mean[0] = block_ori_data[0];

	size_t unpredictable_count = 0;
	size_t r1, r2, r3;
	r1 = block_dim_0;
	r2 = block_dim_1;
	r3 = block_dim_2;

	float * cur_data_pos = block_ori_data;
	float curData;
	float pred1D, pred2D, pred3D;
	double itvNum;
	double diff;
	size_t i, j, k;
	size_t r23 = r2*r3;
	// Process Row-0 data 0
	pred1D = mean[0];
	curData = *cur_data_pos;
	diff = curData - pred1D;
	itvNum = fabs(diff)/realPrecision + 1;
	if (itvNum < exe_params->intvCapacity){
		if (diff < 0) itvNum = -itvNum;
		type[0] = (int) (itvNum/2) + exe_params->intvRadius;
		P1[0] = pred1D + 2 * (type[0] - exe_params->intvRadius) * realPrecision;
		//ganrantee comporession error against the case of machine-epsilon
		if(fabs(curData-P1[0])>realPrecision){	
			type[0] = 0;
			P1[0] = curData;
			unpredictable_data[unpredictable_count ++] = curData;
		}		
	}
	else{
		type[0] = 0;
		P1[0] = curData;
		unpredictable_data[unpredictable_count ++] = curData;
	}

	/* Process Row-0 data 1*/
	pred1D = P1[0];
	curData = cur_data_pos[1];
	diff = curData - pred1D;
	itvNum = fabs(diff)/realPrecision + 1;
	if (itvNum < exe_params->intvCapacity){
		if (diff < 0) itvNum = -itvNum;
		type[1] = (int) (itvNum/2) + exe_params->intvRadius;
		P1[1] = pred1D + 2 * (type[1] - exe_params->intvRadius) * realPrecision;
		//ganrantee comporession error against the case of machine-epsilon
		if(fabs(curData-P1[1])>realPrecision){	
			type[1] = 0;
			P1[1] = curData;	
			unpredictable_data[unpredictable_count ++] = curData;
		}		
	}
	else{
		type[1] = 0;
		P1[1] = curData;
		unpredictable_data[unpredictable_count ++] = curData;
	}
    /* Process Row-0 data 2 --> data r3-1 */
	for (j = 2; j < r3; j++){
		pred1D = 2*P1[j-1] - P1[j-2];
		curData = cur_data_pos[j];
		diff = curData - pred1D;
		itvNum = fabs(diff)/realPrecision + 1;
		if (itvNum < exe_params->intvCapacity){
			if (diff < 0) itvNum = -itvNum;
			type[j] = (int) (itvNum/2) + exe_params->intvRadius;
			P1[j] = pred1D + 2 * (type[j] - exe_params->intvRadius) * realPrecision;
			//ganrantee comporession error against the case of machine-epsilon
			if(fabs(curData-P1[j])>realPrecision){	
				type[j] = 0;
				P1[j] = curData;	
				unpredictable_data[unpredictable_count ++] = curData;
			}			
		}
		else{
			type[j] = 0;
			P1[j] = curData;
			unpredictable_data[unpredictable_count ++] = curData;
		}
	}
	cur_data_pos += dim1_offset;

	/* Process Row-1 --> Row-r2-1 */
	size_t index;
	for (i = 1; i < r2; i++)
	{
		/* Process row-i data 0 */
		index = i*r3;	
		pred1D = P1[index-r3];
		curData = *cur_data_pos;
		diff = curData - pred1D;

		itvNum = fabs(diff)/realPrecision + 1;

		if (itvNum < exe_params->intvCapacity)
		{
			if (diff < 0) itvNum = -itvNum;
			type[index] = (int) (itvNum/2) + exe_params->intvRadius;
			P1[index] = pred1D + 2 * (type[index] - exe_params->intvRadius) * realPrecision;
			
			//ganrantee comporession error against the case of machine-epsilon
			if(fabs(curData-P1[index])>realPrecision)
			{	
				type[index] = 0;
				P1[index] = curData;	
				unpredictable_data[unpredictable_count ++] = curData;
			}			
		}
		else
		{
			type[index] = 0;
			P1[index] = curData;
			unpredictable_data[unpredictable_count ++] = curData;
		}

		/* Process row-i data 1 --> data r3-1*/
		for (j = 1; j < r3; j++)
		{
			index = i*r3+j;
			pred2D = P1[index-1] + P1[index-r3] - P1[index-r3-1];

			curData = cur_data_pos[j];
			diff = curData - pred2D;

			itvNum = fabs(diff)/realPrecision + 1;

			if (itvNum < exe_params->intvCapacity)
			{
				if (diff < 0) itvNum = -itvNum;
				type[index] = (int) (itvNum/2) + exe_params->intvRadius;
				P1[index] = pred2D + 2 * (type[index] - exe_params->intvRadius) * realPrecision;
				
				//ganrantee comporession error against the case of machine-epsilon
				if(fabs(curData-P1[index])>realPrecision)
				{	
					type[index] = 0;
					P1[index] = curData;	
					unpredictable_data[unpredictable_count ++] = curData;
				}				
			}
			else
			{
				type[index] = 0;
				P1[index] = curData;
				unpredictable_data[unpredictable_count ++] = curData;
			}
		}
		cur_data_pos += dim1_offset;
	}
	cur_data_pos += dim0_offset - r2 * dim1_offset;

	///////////////////////////	Process layer-1 --> layer-r1-1 ///////////////////////////

	for (k = 1; k < r1; k++)
	{
		/* Process Row-0 data 0*/
		index = k*r23;
		pred1D = P1[0];
		curData = *cur_data_pos;
		diff = curData - pred1D;
		itvNum = fabs(diff)/realPrecision + 1;
		if (itvNum < exe_params->intvCapacity)
		{
			if (diff < 0) itvNum = -itvNum;
			type[index] = (int) (itvNum/2) + exe_params->intvRadius;
			P0[0] = pred1D + 2 * (type[index] - exe_params->intvRadius) * realPrecision;
			//ganrantee comporession error against the case of machine-epsilon
			if(fabs(curData-P0[0])>realPrecision)
			{	
				type[index] = 0;
				P0[0] = curData;	
				unpredictable_data[unpredictable_count ++] = curData;
			}			
		}
		else
		{
			type[index] = 0;
			P0[0] = curData;
			unpredictable_data[unpredictable_count ++] = curData;
		}
	    /* Process Row-0 data 1 --> data r3-1 */
		for (j = 1; j < r3; j++)
		{
			//index = k*r2*r3+j;
			index ++;
			pred2D = P0[j-1] + P1[j] - P1[j-1];
			curData = cur_data_pos[j];
			diff = curData - pred2D;
			itvNum = fabs(diff)/realPrecision + 1;
			if (itvNum < exe_params->intvCapacity)
			{
				if (diff < 0) itvNum = -itvNum;
				type[index] = (int) (itvNum/2) + exe_params->intvRadius;
				P0[j] = pred2D + 2 * (type[index] - exe_params->intvRadius) * realPrecision;
				//ganrantee comporession error against the case of machine-epsilon
				if(fabs(curData-P0[j])>realPrecision)
				{	
					type[index] = 0;
					P0[j] = curData;	
					unpredictable_data[unpredictable_count ++] = curData;
				}
			}
			else
			{
				type[index] = 0;
				P0[j] = curData;
				unpredictable_data[unpredictable_count ++] = curData;
			}
		}

		cur_data_pos += dim1_offset;
	    /* Process Row-1 --> Row-r2-1 */
		size_t index2D;
		for (i = 1; i < r2; i++)
		{
			/* Process Row-i data 0 */
			index = k*r23 + i*r3;
			index2D = i*r3;		
			pred2D = P0[index2D-r3] + P1[index2D] - P1[index2D-r3];
			curData = *cur_data_pos;
			diff = curData - pred2D;

			itvNum = fabs(diff)/realPrecision + 1;

			if (itvNum < exe_params->intvCapacity)
			{
				if (diff < 0) itvNum = -itvNum;
				type[index] = (int) (itvNum/2) + exe_params->intvRadius;
				P0[index2D] = pred2D + 2 * (type[index] - exe_params->intvRadius) * realPrecision;
				//ganrantee comporession error against the case of machine-epsilon
				if(fabs(curData-P0[index2D])>realPrecision)
				{	
					type[index] = 0;
					P0[index2D] = curData;	
					unpredictable_data[unpredictable_count ++] = curData;
				}				
			}
			else
			{
				type[index] = 0;
				P0[index2D] = curData;
				unpredictable_data[unpredictable_count ++] = curData;
			}

			/* Process Row-i data 1 --> data r3-1 */
			for (j = 1; j < r3; j++)
			{
				//index = k*r2*r3 + i*r3 + j;			
				index ++;
				index2D = i*r3 + j;
				pred3D = P0[index2D-1] + P0[index2D-r3]+ P1[index2D] - P0[index2D-r3-1] - P1[index2D-r3] - P1[index2D-1] + P1[index2D-r3-1];
				curData = cur_data_pos[j];
				diff = curData - pred3D;

				itvNum = fabs(diff)/realPrecision + 1;

				if (itvNum < exe_params->intvCapacity)
				{
					if (diff < 0) itvNum = -itvNum;
					type[index] = (int) (itvNum/2) + exe_params->intvRadius;
					P0[index2D] = pred3D + 2 * (type[index] - exe_params->intvRadius) * realPrecision;
					
					//ganrantee comporession error against the case of machine-epsilon
					if(fabs(curData-P0[index2D])>realPrecision)
					{	
						type[index] = 0;
						P0[index2D] = curData;	
						unpredictable_data[unpredictable_count ++] = curData;
					}					
				}
				else
				{
					type[index] = 0;
					P0[index2D] = curData;
					unpredictable_data[unpredictable_count ++] = curData;
				}
			}
			cur_data_pos += dim1_offset;
		}
		cur_data_pos += dim0_offset - r2 * dim1_offset;
		float *Pt;
		Pt = P1;
		P1 = P0;
		P0 = Pt;
	}

	return unpredictable_count;
}

unsigned int optimize_intervals_float_2D_opt(float *oriData, size_t r1, size_t r2, double realPrecision)
{	
	size_t i;
	size_t radiusIndex;
	float pred_value = 0, pred_err;
	size_t *intervals = (size_t*)malloc(conf_params->maxRangeRadius*sizeof(size_t));
	memset(intervals, 0, conf_params->maxRangeRadius*sizeof(size_t));
	size_t totalSampleSize = 0;//(r1-1)*(r2-1)/conf_params->sampleDistance;

	//float max = oriData[0];
	//float min = oriData[0];

	size_t offset_count = conf_params->sampleDistance - 1; // count r2 offset
	size_t offset_count_2;
	float * data_pos = oriData + r2 + offset_count;
	size_t n1_count = 1; // count i sum
	size_t len = r1 * r2;
	while(data_pos - oriData < len){
		totalSampleSize++;
		pred_value = data_pos[-1] + data_pos[-r2] - data_pos[-r2-1];
		pred_err = fabs(pred_value - *data_pos);
		radiusIndex = (unsigned long)((pred_err/realPrecision+1)/2);
		if(radiusIndex>=conf_params->maxRangeRadius)
			radiusIndex = conf_params->maxRangeRadius - 1;
		intervals[radiusIndex]++;

		offset_count += conf_params->sampleDistance;
		if(offset_count >= r2){
			n1_count ++;
			offset_count_2 = n1_count % conf_params->sampleDistance;
			data_pos += (r2 + conf_params->sampleDistance - offset_count) + (conf_params->sampleDistance - offset_count_2);
			offset_count = (conf_params->sampleDistance - offset_count_2);
			if(offset_count == 0) offset_count ++;
		}
		else data_pos += conf_params->sampleDistance;
	}

	//compute the appropriate number
	size_t targetCount = totalSampleSize*conf_params->predThreshold;
	size_t sum = 0;
	for(i=0;i<conf_params->maxRangeRadius;i++)
	{
		sum += intervals[i];
		if(sum>targetCount)
			break;
	}
	if(i>=conf_params->maxRangeRadius)
		i = conf_params->maxRangeRadius-1;
	unsigned int accIntervals = 2*(i+1);
	unsigned int powerOf2 = roundUpToPowerOf2(accIntervals);

	if(powerOf2<32)
		powerOf2 = 32;

	free(intervals);
	return powerOf2;
}

unsigned int optimize_intervals_float_1D_opt(float *oriData, size_t dataLength, double realPrecision)
{	
	size_t i = 0, radiusIndex;
	float pred_value = 0, pred_err;
	size_t *intervals = (size_t*)malloc(conf_params->maxRangeRadius*sizeof(size_t));
	memset(intervals, 0, conf_params->maxRangeRadius*sizeof(size_t));
	size_t totalSampleSize = 0;//dataLength/conf_params->sampleDistance;

	float * data_pos = oriData + 2;
	while(data_pos - oriData < dataLength){
		totalSampleSize++;
		//pred_value = 2*data_pos[-1] - data_pos[-2];
		pred_value = data_pos[-1];
		pred_err = fabs(pred_value - *data_pos);
		radiusIndex = (unsigned long)((pred_err/realPrecision+1)/2);
		if(radiusIndex>=conf_params->maxRangeRadius)
			radiusIndex = conf_params->maxRangeRadius - 1;			
		intervals[radiusIndex]++;

		data_pos += conf_params->sampleDistance;
	}
	//compute the appropriate number
	size_t targetCount = totalSampleSize*conf_params->predThreshold;
	size_t sum = 0;
	for(i=0;i<conf_params->maxRangeRadius;i++)
	{
		sum += intervals[i];
		if(sum>targetCount)
			break;
	}
	if(i>=conf_params->maxRangeRadius)
		i = conf_params->maxRangeRadius-1;
		
	unsigned int accIntervals = 2*(i+1);
	unsigned int powerOf2 = roundUpToPowerOf2(accIntervals);
	
	if(powerOf2<32)
		powerOf2 = 32;
	
	free(intervals);
	//printf("accIntervals=%d, powerOf2=%d\n", accIntervals, powerOf2);
	return powerOf2;
}

size_t SZ_compress_float_1D_MDQ_RA_block(float * block_ori_data, float * mean, size_t dim_0, size_t block_dim_0, double realPrecision, int * type, float * unpredictable_data){

	mean[0] = block_ori_data[0];
	unsigned short unpredictable_count = 0;

	float curData;
	double itvNum;
	double diff;
	float last_over_thres = mean[0];
	float pred1D;
	size_t type_index = 0;
	float * data_pos = block_ori_data;
	for(size_t i=0; i<block_dim_0; i++){
		curData = *data_pos;

		pred1D = last_over_thres;
		diff = curData - pred1D;
		itvNum = fabs(diff)/realPrecision + 1;
		if (itvNum < exe_params->intvCapacity){
			if (diff < 0) itvNum = -itvNum;
			type[type_index] = (int) (itvNum/2) + exe_params->intvRadius;	
			last_over_thres = pred1D + 2 * (type[type_index] - exe_params->intvRadius) * realPrecision;
			if(fabs(curData-last_over_thres)>realPrecision){
				type[type_index] = 0;
				last_over_thres = curData;
				unpredictable_data[unpredictable_count ++] = curData;
			}

		}
		else{
			type[type_index] = 0;
			unpredictable_data[unpredictable_count ++] = curData;
			last_over_thres = curData;
		}
		type_index ++;
		data_pos ++;
	}
	return unpredictable_count;

}

size_t SZ_compress_float_2D_MDQ_RA_block(float * block_ori_data, float * mean, size_t dim_0, size_t dim_1, size_t block_dim_0, size_t block_dim_1, double realPrecision, float * P0, float * P1, int * type, float * unpredictable_data){

	size_t dim0_offset = dim_1;
	mean[0] = block_ori_data[0];

	size_t unpredictable_count = 0;
	size_t r1, r2;
	r1 = block_dim_0;
	r2 = block_dim_1;

	float * cur_data_pos = block_ori_data;
	float curData;
	float pred1D, pred2D;
	double itvNum;
	double diff;
	size_t i, j;
	/* Process Row-0 data 0*/
	curData = *cur_data_pos;
	pred1D = mean[0];
	diff = curData - pred1D;
	itvNum = fabs(diff)/realPrecision + 1;
	if (itvNum < exe_params->intvCapacity){
		if (diff < 0) itvNum = -itvNum;
		type[0] = (int) (itvNum/2) + exe_params->intvRadius;
		P1[0] = pred1D + 2 * (type[0] - exe_params->intvRadius) * realPrecision;
		//ganrantee comporession error against the case of machine-epsilon
		if(fabs(curData-P1[0])>realPrecision){	
			type[0] = 0;
			P1[0] = curData;
			unpredictable_data[unpredictable_count ++] = curData;
		}		
	}
	else{
		type[0] = 0;
		P1[0] = curData;
		unpredictable_data[unpredictable_count ++] = curData;
	}

	/* Process Row-0 data 1*/
	curData = cur_data_pos[1];
	pred1D = P1[0];
	diff = curData - pred1D;
	itvNum = fabs(diff)/realPrecision + 1;
	if (itvNum < exe_params->intvCapacity){
		if (diff < 0) itvNum = -itvNum;
		type[1] = (int) (itvNum/2) + exe_params->intvRadius;
		P1[1] = pred1D + 2 * (type[1] - exe_params->intvRadius) * realPrecision;
		//ganrantee comporession error against the case of machine-epsilon
		if(fabs(curData-P1[1])>realPrecision){	
			type[1] = 0;
			P1[1] = curData;	
			unpredictable_data[unpredictable_count ++] = curData;
		}		
	}
	else{
		type[1] = 0;
		P1[1] = curData;
		unpredictable_data[unpredictable_count ++] = curData;
	}

    /* Process Row-0 data 2 --> data r2-1 */
	for (j = 2; j < r2; j++)
	{
		curData = cur_data_pos[j];
		pred1D = 2*P1[j-1] - P1[j-2];
		diff = curData - pred1D;
		itvNum = fabs(diff)/realPrecision + 1;
		if (itvNum < exe_params->intvCapacity){
			if (diff < 0) itvNum = -itvNum;
			type[j] = (int) (itvNum/2) + exe_params->intvRadius;
			P1[j] = pred1D + 2 * (type[j] - exe_params->intvRadius) * realPrecision;
			//ganrantee comporession error against the case of machine-epsilon
			if(fabs(curData-P1[j])>realPrecision){	
				type[j] = 0;
				P1[j] = curData;	
				unpredictable_data[unpredictable_count ++] = curData;
			}			
		}
		else{
			type[j] = 0;
			P1[j] = curData;
			unpredictable_data[unpredictable_count ++] = curData;
		}
	}
	cur_data_pos += dim0_offset;
	/* Process Row-1 --> Row-r1-1 */
	size_t index;
	for (i = 1; i < r1; i++)
	{	
		/* Process row-i data 0 */
		index = i*r2;
		curData = *cur_data_pos;
		pred1D = P1[0];
		diff = curData - pred1D;
		itvNum = fabs(diff)/realPrecision + 1;
		if (itvNum < exe_params->intvCapacity){
			if (diff < 0) itvNum = -itvNum;
			type[index] = (int) (itvNum/2) + exe_params->intvRadius;
			P0[0] = pred1D + 2 * (type[j] - exe_params->intvRadius) * realPrecision;
			//ganrantee comporession error against the case of machine-epsilon
			if(fabs(curData-P0[0])>realPrecision){	
				type[index] = 0;
				P0[0] = curData;	
				unpredictable_data[unpredictable_count ++] = curData;
			}			
		}
		else{
			type[index] = 0;
			P0[0] = curData;
			unpredictable_data[unpredictable_count ++] = curData;
		}
									
		/* Process row-i data 1 --> r2-1*/
		for (j = 1; j < r2; j++)
		{
			index = i*r2+j;
			curData = cur_data_pos[j];
			pred2D = P0[j-1] + P1[j] - P1[j-1];
			diff = curData - pred2D;
			itvNum = fabs(diff)/realPrecision + 1;
			if (itvNum < exe_params->intvCapacity)
			{
				if (diff < 0) itvNum = -itvNum;
				type[index] = (int) (itvNum/2) + exe_params->intvRadius;
				P0[j] = pred2D + 2 * (type[index] - exe_params->intvRadius) * realPrecision;
				
				//ganrantee comporession error against the case of machine-epsilon
				if(fabs(curData-P0[j])>realPrecision)
				{	
					type[index] = 0;
					P0[j] = curData;	
					unpredictable_data[unpredictable_count ++] = curData;
				}				
			}
			else
			{
				type[index] = 0;
				P0[j] = curData;
				unpredictable_data[unpredictable_count ++] = curData;
			}
		}
		cur_data_pos += dim0_offset;

		float *Pt;
		Pt = P1;
		P1 = P0;
		P0 = Pt;
	}
	return unpredictable_count;
}

