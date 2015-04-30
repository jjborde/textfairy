//============================================================================
// Name        : HelloWorld2.cpp
// Author      : 
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <iostream>
#include <allheaders.h>
#include <image_processing_util.h>
#include "baseapi.h"
#include <ocrclass.h>
#include "binarize.h"
#include "dewarp_textfairy.h"
#include "RunningStats.h"
#include <stdlib.h>
#include <algorithm>    // std::max
#include <math.h>       /* pow */
using namespace std;

static Pixa* pixaDebugDisplay = pixaCreate(0);


void messageJavaCallback(int message) {
	printf("pixJavmessageJavaCallback(%i)\n",message);
}

void pixJavaCallback(Pix* pix) {
	pixaAddPix(pixaDebugDisplay, pix, L_CLONE);
}

/**
 * callback for tesseracts monitor
 */
bool progressJavaCallback(void* progress_this,int progress, int left, int right, int top, int bottom) {
	printf("progress = %i, (%i,%i) - (%i,%i)\n",progress, left, bottom,right,top);
	return true;
}

/**
 * callback for tesseracts monitor
 */
bool cancelFunc(void* cancel_this, int words) {
	return false;
}

void testScaleWithBitmap(){
	Pix* pixOrg = pixRead("images/bmpfile.bmp");
    Pix* pixd = pixRemoveColormap(pixOrg, REMOVE_CMAP_BASED_ON_SRC);
    if(pixd==NULL){
    	fprintf(stderr, "converting to grey");
    	pixd=pixConvertTo8(pixOrg, false);
    }
	l_int32 d = pixGetDepth(pixd);
	fprintf(stderr, "d = %i.",d);

	pixd = pixScaleGeneral(pixd, 0.5, 0.5,0, 0);
	pixDisplay(pixd,0,0);

}

void doOcr(){
	Pix* pixOrg = pixRead("images/bmpfile.bmp");
	Pix* pixText;
	pixJavaCallback(pixOrg);

	bookpage(pixOrg, &pixText , messageJavaCallback, pixJavaCallback, true);

	pixWrite("dewarpedTest.png",pixText,IFF_PNG);

	ETEXT_DESC monitor;
	monitor.progress_callback = progressJavaCallback;
	monitor.cancel = cancelFunc;
	monitor.cancel_this = NULL;
	monitor.progress_this = NULL;

	tesseract::TessBaseAPI api;
	int rc =api.Init("/Users/renard/devel/textfairy", "bul+eng", tesseract::OEM_DEFAULT);
	if (rc) {
		fprintf(stderr, "Could not initialize tesseract.\n");
		exit(1);
	}
	startTimer();

	api.SetPageSegMode(tesseract::PSM_AUTO);
	api.SetInputName("pdfimage");
  	api.SetImage(pixText);
	api.Recognize(&monitor);
	char* text = api.GetUTF8Text();
	log("ocr time: %f",stopTimer());

	printf("%s",text);
	free(text);
	api.End();
	pixWriteImpliedFormat("testpng.png",pixText,85,0);
	Pix* pixd = pixaDisplayTiledAndScaled(pixaDebugDisplay, 8, 640, 3, 0, 25, 2);
	pixDisplay(pixd, 0, 0);
	pixDestroy(&pixd);

	pixaClear(pixaDebugDisplay);
	pixDestroy(&pixOrg);
	pixDestroy(&pixText);
}

void perspective(){
	Pix* pixOrg = pixRead("images/renard.png");

	PTA* orgPoints = ptaCreate(4);
	PTA* mappedPoints = ptaCreate(4);
	l_int32 x1,y1,x2,y2,x3,y3,x4,y4;
	x1 = 541;
	y1 = 172;
	x2 = 2235;
	y2 = 0;
	x3 = 2218;
	y3 = 1249;
	x4 = 605;
	y4 = 1002;

	ptaAddPt(orgPoints,x1,y1);
	ptaAddPt(orgPoints,x2,y2);
	ptaAddPt(orgPoints,x3,y3);
	ptaAddPt(orgPoints,x4,y4);



	pixRenderLine(pixOrg,x1,y1,x2,y2,6,L_CLEAR_PIXELS);
	pixRenderLine(pixOrg,x2,y2,x3,y3,6,L_CLEAR_PIXELS);
	pixRenderLine(pixOrg,x3,y3,x4,y4,6,L_CLEAR_PIXELS);
	pixRenderLine(pixOrg,x4,y4,x1,y1,6,L_CLEAR_PIXELS);

	l_int32  x, y, w, h;
	w = pixGetWidth(pixOrg);
	h = pixGetHeight(pixOrg);
	x =0;
	y= 0;
	Box* orgBox = boxCreate(x,y,w,h);
	ptaAddPt(mappedPoints, x, y);
	ptaAddPt(mappedPoints, x + w - 1, y);
	ptaAddPt(mappedPoints, x + w - 1, y + h - 1);
	ptaAddPt(mappedPoints, x, y + h - 1);

	pixRenderBox(pixOrg,orgBox,5,L_SET_PIXELS);
	//Pix* pixBilinar = pixBilinearPta(pixOrg,mappedPoints,orgPoints,L_BRING_IN_WHITE);
	Pix* pixBilinar =pixProjectivePtaColor(pixOrg,mappedPoints,orgPoints,0xffffff00);
	pixDisplay(pixOrg,0,0);
	pixDisplay(pixBilinar,0,0);
	pixDestroy(&pixOrg);
	pixDestroy(&pixBilinar);
}

bool numaGetStdDeviationOnInterval(Numa* na, l_int32 start, l_int32 end, l_float32* stdDev, l_float32* errorPercentage, l_float32* mean){
	l_int32 n = numaGetCount(na);
	if (n < 2) {
		return false;
	}
	if(end>n){
		return false;
	}

	l_int32 val;
	RunningStats stats;
	for (int i = start; i < end; i++) {
		numaGetIValue(na, i, &val);
		stats.Push(val);
	}
	if (stdDev != NULL) {
		*stdDev = stats.PopulationStandardDeviation();
	}
	if(errorPercentage!=NULL){
		if(stats.Mean()>0){
			*errorPercentage = stats.PopulationStandardDeviation() / fabs(stats.Mean());
		} else {
			*errorPercentage = 0;
		}
	}
	if(mean!=NULL){
		*mean = stats.Mean();
	}
	return true;
}


bool numaGetStdDeviation(Numa* na, l_float32* stdDev, l_float32* errorPercentage, l_float32* mean) {
	l_int32 n = numaGetCount(na);
	return numaGetStdDeviationOnInterval(na,0,n,stdDev,errorPercentage,mean);
}

void pixBlurDebug(Pix* pix){
	Pix* pixsg = pixConvertRGBToLuminance(pix);
	Pix* pixEdge = pixTwoSidedEdgeFilter(pixsg, L_VERTICAL_EDGES);
	NUMA* histo = pixGetGrayHistogram(pixEdge, 1);
	pixWrite("lines.png",pixEdge,IFF_PNG);
	Numa*  histonorm = numaNormalizeHistogram(histo,1);
	l_float32 stdDev, mean, error;
	numaGetStdDeviation(histonorm,&stdDev,&error, &mean);
	log("stdDev =%f ,mean=%f, blur = %f", stdDev,mean, mean*stdDev );
	pixDestroy(&pixEdge);
	pixDestroy(&pixsg);
	numaDestroy(&histonorm);

}

void dewarp(){
	Pix* pixOrg = pixRead("images/renard.png");
	Pix* pixsg = pixConvertRGBToLuminance(pixOrg);
	Pix*pixb;

	binarize(pixsg, NULL, &pixb);
	Ptaa* ptaa1 = dewarpGetTextlineCenters(pixb, 0);
	Pix* pixt1 = pixCreateTemplate(pixOrg);
	Pix* pixt2 = pixDisplayPtaa(pixt1, ptaa1);
	//pixDisplayWithTitle(pixt2, 0, 500, "textline centers", 1);

    /* Remove short lines */
	Ptaa* ptaa2 = dewarpRemoveShortLines(pixb, ptaa1, 0.8, 0);

    /* Fit to quadratic */
	int n = ptaaGetCount(ptaa2);
	int i=0;
	NUMA         *nax, *nafit;
	l_float32     a, b, c, d, e;

	for (i = 0; i < n; i++) {
		Pta* pta = ptaaGetPta(ptaa2, i, L_CLONE);
		ptaGetArrays(pta, &nax, NULL);
		//ptaGetQuadraticLSF(pta, &a, &b, &c, &nafit);
		//ptaGetCubicLSF(pta, &a,&b,&c,&d,&nafit);
		ptaGetQuarticLSF(pta,&a,&b,&c,&d,&e,&nafit);
		Pta* ptad = ptaCreateFromNuma(nax, nafit);
		pixDisplayPta(pixt2, pixt2, ptad);
		ptaDestroy(&pta);
		ptaDestroy(&ptad);
		numaDestroy(&nax);
		numaDestroy(&nafit);
	}
	//pixDisplayWithTitle(pixt2, 300, 500, "fitted lines superimposed",1);
	Pix* pixd;
	pixDewarp(pixb,&pixd);
	pixDisplayWithTitle(pixd, 300, 500, "cubic dewarp",1);

}

#include <iostream>
#include <fstream>
#include <sstream>

#include "Codecs.hh"
#include "pdf.hh"
#include "hocr.hh"
#include "jpeg.hh"

void createPdf(const char* imagePath, const char* hocrPath) {
	printf("%s", "creating pdf");

	ofstream pdfOutStream("test.pdf");
	PDFCodec* pdfContext = new PDFCodec(&pdfOutStream);
	bool sloppy = false;
	bool overlayImage = false;
	Image image;
	image.w = image.h = 0;
	std::string fileName(imagePath);
	if (!ImageCodec::Read(fileName, image)) {
		std::cout << "Error reading input file.";
	}

	if (image.resolutionX() <= 0 || image.resolutionY() <= 0) {
		std::cout << "Warning: Image x/y resolution not set, defaulting to: 300 ";
		image.setResolution(300, 300);
	}

	unsigned int res = image.resolutionX();

	std::ifstream file( hocrPath );
	std::stringstream hocr;

	if(file) {
		hocr << file.rdbuf();
		file.close();
		pdfContext->beginPage(72. * image.w / res, 72. * image.h / res);
		pdfContext->setFillColor(0, 0, 0);
		hocr2pdf(hocr, pdfContext, res, sloppy, !overlayImage);

		if (overlayImage) {
			pdfContext->showImage(image, 0, 0, 72. * image.w / res, 72. * image.h / res);
		}
	}


	delete pdfContext;
}


void getValueBetweenTwoFixedColors(float value, int r, int g, int b, int &red, int &green, int &blue) {
  int bR = 255; int bG = 0; int bB=0;    // RGB for our 2nd color (red in this case).

  red   = (float)(bR - r) * value + r;      // Evaluated as -255*value + 255.
  green = (float)(bG - g) * value + g;      // Evaluates as 0.
  blue  = (float)(bB - b) * value + b;      // Evaluates as 255*value + 0.
}

Pix* blurHorizontal(Pix* pixGrey, Pix* pixMedian,const char* name) {

	l_int32    w, h, wpld, wplb, wplm, wplo,wpls;
	l_int32    i, j, x, rval, gval, bval;
	l_uint32  *datad, *datab, *datas,*datao,*datam, *lined, *lineb, *lines, *lineo, *linem;

	Pix* edges = pixTwoSidedEdgeFilter(pixMedian, L_VERTICAL_EDGES);
	//Pix* edges = pixSobelEdgeFilter(pixMedian,L_ALL_EDGES);
	w = pixGetWidth(edges);
	h = pixGetHeight(edges);
	Pix* blurMeasure = pixCreate(w,h,8);

	NUMA* na = pixGetGrayHistogram(edges, 4);
	int thresh;
	numaSplitDistribution(na, 0.0, &thresh, NULL, NULL, NULL, NULL, NULL);
	numaDestroy(&na);
	Pix* pixBinary = pixThresholdToBinary(edges, thresh);
	//pixCloseBrickDwa(pixBinary,pixBinary,2,2);

    //datao = pixGetData(pixOrg);
    datas = pixGetData(pixGrey);
    datad = pixGetData(blurMeasure);
    datab = pixGetData(pixBinary);
    datam = pixGetData(pixMedian);
    //wplo = pixGetWpl(pixOrg);
    wpls = pixGetWpl(pixGrey);
    wpld = pixGetWpl(blurMeasure);
    wplb = pixGetWpl(pixBinary);
    wplm = pixGetWpl(pixMedian);
    RunningStats stats;
    Numa* numaValues = numaCreate(0);
    l_uint32 k= 2;
    for (i = k; i < h-k; i++) {
        //lineo = datao + i * wplo;
        lined = datad + i * wpld;
        lineb = datab + i * wplb;
        linem = datam + i * wplm;
        lines = datas + i * wpls;
        for (j = k; j < w-k; j++) {
            if (!GET_DATA_BIT(lineb, j)) {
            	l_uint32 dom = 0;
            	l_uint32 contrast = 0;
            	for(x = j-k;x <=j+k; x++){
            		dom+= abs((GET_DATA_BYTE(linem,x+2) - GET_DATA_BYTE(linem,x))-(GET_DATA_BYTE(linem,x) - GET_DATA_BYTE(linem,x-2)));
                	contrast+= abs(GET_DATA_BYTE(lines,x) - GET_DATA_BYTE(lines,x-1));
            	}
            	if(contrast>0){
					double sharpness = (((float)dom)/((float)contrast));
					numaAddNumber(numaValues,sharpness);
					stats.Push(sharpness);
					float val = min(255.0, sharpness*50);
					SET_DATA_BYTE(lined,j, val);
					/*
					extractRGBValues(lineo[j], &rval, &gval, &bval);
					double fraction = val/255;
					//fraction = pow(fraction,2.4);
					//float)Math.pow(input, mDoubleFactor);
					int red, green, blue;
					getValueBetweenTwoFixedColors(fraction,rval,gval,bval,red, green, blue);
					composeRGBPixel(red, green, blue, lineo + j);
					*/
            	}

            }
        }
    }

    //pixWrite("blurMeasure.png",blurMeasure, IFF_PNG);
    pixWrite("binary.png",pixBinary, IFF_PNG);
    pixWrite("edges.png",edges, IFF_PNG);
    l_float32 val;
    numaGetRankValue(numaValues,0.5,NULL,0,&val);
    printf(" %s = %f, rank = %f\n",name, stats.Mean(), val);
	//pixDestroy(&pixMedian);
	pixDestroy(&pixBinary);
	pixDestroy(&edges);
	return blurMeasure;
}

Pix* binarizeEdges(Pix* edges){
	NUMA* na = pixGetGrayHistogram(edges, 4);
	int thresh;
	numaSplitDistribution(na, 0.1, &thresh, NULL, NULL, NULL, NULL, NULL);
	if(thresh==1){
		thresh++;
	}
	numaDestroy(&na);
	return pixThresholdToBinary(edges, thresh);
}

Pix* pixMakeBlurMask(Pix* pixGrey, Pix* pixMedian, const char* name) {

	l_int32    width, height, wpld, wplbx, wplby, wplm, wplo,wpls;
	l_int32    y, x, k, rval, gval, bval;
	l_uint32  *datad, *databx,*databy, *datas,*datao,*datam, *lined, *linebx, *lineby, *lines, *lineo, *linem;

	Pix* edgesx = pixTwoSidedEdgeFilter(pixMedian, L_VERTICAL_EDGES);
	Pix* edgesy = pixTwoSidedEdgeFilter(pixMedian, L_HORIZONTAL_EDGES);
	width = pixGetWidth(edgesx);
	height = pixGetHeight(edgesx);
	Pix* blurMeasure = pixCreate(width,height,8);
	Pix* pixBinaryx = binarizeEdges(edgesx);
	Pix* pixBinaryy = binarizeEdges(edgesy);

	//pixCloseBrickDwa(pixBinary,pixBinary,2,2);
	Pix* pixOrg = pixConvert8To32(pixGrey);

    datao = pixGetData(pixOrg);
    datas = pixGetData(pixGrey);
    datad = pixGetData(blurMeasure);
    databx = pixGetData(pixBinaryx);
    databy = pixGetData(pixBinaryy);
    datam = pixGetData(pixMedian);
    wplo = pixGetWpl(pixOrg);
    wpls = pixGetWpl(pixGrey);
    wpld = pixGetWpl(blurMeasure);
    wplbx = pixGetWpl(pixBinaryx);
    wplby = pixGetWpl(pixBinaryy);
    wplm = pixGetWpl(pixMedian);
    RunningStats stats;
    Numa* numaValues = numaCreate(0);
    l_int32 w = 2;
    l_int32 w2 = w*2;
    bool paintBlurMask = true;
    for (y = w2; y < height-w2; y++) {
        lineo = datao + y * wplo;
        linem = datam + y * wplm;
        lined = datad + y * wpld;
        linebx = databx + y * wplbx;
        lineby = databy + y * wplby;
        lines = datas + y * wpls;
        for (x = w2; x < width-w2; x++) {
        	bool hasx = !GET_DATA_BIT(linebx, x);
        	bool hasy = !GET_DATA_BIT(lineby, x);
            if (hasx||hasy) {
            	l_uint32 domx = 0;
            	l_uint32 contrastx = 0;
            	l_uint32 domy = 0;
            	l_uint32 contrasty = 0;
            	for(k = -w;k <=w; k++){
            		//vertical dom
            		if(hasy){
            			l_uint32 row = y+k;
						l_uint8 y1 = GET_DATA_BYTE(datam + (row+2) * wplm,x);
						l_uint8 y2 = GET_DATA_BYTE(datam + row * wplm,x);
						l_uint8 y3 = GET_DATA_BYTE(datam + (row-2) * wplm,x);
						domy+=abs((y1-y2)-(y2-y3));
						y1 = GET_DATA_BYTE(datas + (row-1) * wpls,x);
						y2 = GET_DATA_BYTE(datas + (row) * wpls,x);
						contrasty+= abs(y1-y2);
            		}

                    //horizontal dom
            		if(hasx){
            			l_uint32 column = x+k;
            			l_uint8 x1=GET_DATA_BYTE(linem,column);
						domx+= abs((GET_DATA_BYTE(linem,column+2) - x1)-(x1 - GET_DATA_BYTE(linem,column-2)));
						contrastx+= abs(x1 - GET_DATA_BYTE(lines,column-1));
            		}

            	}
            	double sharpnessx = 0;
            	double sharpnessy = 0;
            	double sharpness = 0;
            	if(contrastx>0){
    				sharpnessx = (((float)domx)/((float)contrastx));
            	}
            	if(contrasty>0){
    				sharpnessy = (((float)domy)/((float)contrasty));
            	}

            	if(sharpnessx>0 && sharpnessy>0){
            		sharpness = sqrt(sharpnessx*sharpnessx + sharpnessy * sharpnessy);
            		//printf("x= %f, y = %f, total = %f, hasx=%i, hasy=%i\n",sharpnessx,sharpnessy,sharpness, hasx, hasy);
            	} else if(sharpnessx>0){
            		sharpness= sharpnessx;
            	} else {
            		sharpness= sharpnessy;
            	}
				numaAddNumber(numaValues,sharpness);
				stats.Push(sharpness);
				float val = min(255.0, (sharpness)*40);
				SET_DATA_BYTE(lined,x, val);
				if(paintBlurMask){
					extractRGBValues(lineo[x], &rval, &gval, &bval);
					//3= sharp, 1 not sharp
					double maxSharpness = 3.0;
					double minSharpness = 1.0;
					double clamped = min(maxSharpness,sharpness);
					clamped = max(minSharpness,clamped);
					//scale range to 0-1
					clamped = (clamped-minSharpness)/(maxSharpness-minSharpness);
					//clamped = pow(clamped,2.4);
					int red, green, blue;
					//printf("sharpness = %f, fraction = %f\n",sharpness, clamped);
					getValueBetweenTwoFixedColors(1-clamped,rval,gval,bval,red, green, blue);
					composeRGBPixel(red, green, blue, lineo + x);
				}

            }
        }
    }

    printf("1\n");

    //pixWrite("blurMeasure.png",blurMeasure, IFF_PNG);
    pixWrite("binaryx.png",pixBinaryx, IFF_PNG);
    printf("2\n");
    pixWrite("binaryy.png",pixBinaryy, IFF_PNG);
    pixWrite("edgesx.png",edgesx, IFF_PNG);
    pixWrite("edgesy.png",edgesy, IFF_PNG);
	pixWrite("blurBended.png",pixOrg, IFF_PNG);
    printf("3\n");
    l_float32 val;
    numaGetRankValue(numaValues,0.5,NULL,0,&val);
    printf(" %s = %f, rank = %f\n",name, stats.Mean(), val);
	pixDestroy(&pixBinaryx);
	pixDestroy(&pixBinaryy);
	pixDestroy(&edgesx);
	pixDestroy(&edgesy);
	return blurMeasure;
}
void blurDetect(const char* image){
	Pix* pixOrg = pixRead(image);
	Pix* pixGrey;
	switch(pixGetDepth(pixOrg)){
		case 1:
			pixGrey = pixConvert1To8(NULL,pixOrg,0,255);
			break;
		case 8:
			pixGrey = pixClone(pixOrg);
			break;
		case 32:
			pixGrey = pixConvertRGBToGrayFast(pixOrg);
			break;
	}
	Pix* pixMedian = pixMedianFilter(pixGrey,4,4);
	Pix* blurMeasure = pixMakeBlurMask(pixGrey, pixMedian, image);
    pixWrite("median.png",pixMedian, IFF_PNG);
    pixWrite("grey.png",pixGrey, IFF_PNG);
    pixWrite("blurMeasure.png",blurMeasure, IFF_PNG);
	pixDestroy(&pixOrg);
	pixDestroy(&pixGrey);
	pixDestroy(&blurMeasure);

}


void testAllBlur(){

	blurDetect("images/sharp2.jpg");
	blurDetect("images/sharp3.png");
	blurDetect("images/sharp4.jpg");
	blurDetect("images/sharp5.jpg");
	blurDetect("images/sharp6.jpg");
	blurDetect("images/sharp7.png");
	printf("BLURRED\n");
	blurDetect("images/blur2.jpg");
	blurDetect("images/blur3.jpg");
	blurDetect("images/blur4.jpg");
	blurDetect("images/blur5.jpg");
	blurDetect("images/blur1.jpg");

}
int main() {
	//createPdf("images/5.png","images/scan_test.html");
	//testScaleWithBitmap();
	blurDetect("images/53.jpg");
	//blurDetect("images/blur3.jpg");
	//testAllBlur();
	//blurDetect("images/blur3.jpg");


	return 0;
}
