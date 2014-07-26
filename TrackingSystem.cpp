#include "TrackingSystem.h"
#include "FeatureDetector.h"
#include "BlinkDetector.h"

static double quadraticMinimum(double xm1, double x0, double xp1) {
	//cout << "values:" << xm1 << " " << x0 << " " << xp1 << endl;
	return (xm1 - xp1) / (2*(xp1 + xm1 - 2*x0));
}

static Point subpixelMinimum(const IplImage *values) {
	CvPoint maxPoint;
	cvMinMaxLoc(values, NULL, NULL, &maxPoint);
	//cout << "max: " << maxPoint.x << " " << maxPoint.y << endl;

	int x = maxPoint.x;
	int y = maxPoint.y;
	Point p(x, y);

	if (x > 0 && x < 6) {
		p.x += quadraticMinimum(cvGetReal2D(values, y, x - 1), cvGetReal2D(values, y, x + 0), cvGetReal2D(values, y, x + 1));
	}

	if (y > 0 && y < 4) {
		p.y += quadraticMinimum(cvGetReal2D(values, y - 1, x), cvGetReal2D(values, y + 0, x), cvGetReal2D(values, y + 1, x));
	}

	return p;
}


TrackingSystem::TrackingSystem(CvSize size):
	pointTracker(size),
	eyeExtractor(pointTracker)
	_headTracker(pointTracker),
	_headCompensation(_headTracker),
{
}


void TrackingSystem::process(const IplImage *frame, IplImage *image) {
	if (tracker_status != STATUS_PAUSED) {
		pointTracker.track(frame, 2);

		if (pointTracker.countactivepoints() < 4) {
			pointTracker.draw(image);
			throw TrackingException();
		}

		_headTracker.updatetracker();
		eyeExtractor.extractEye(frame);	// throws Tracking Exception
		gazeTracker.update(eyeExtractor.eyefloat.get(), eyeExtractor.eyegrey.get());
		gazeTracker.update_left(eyeExtractor.eyefloat_left.get(), eyeExtractor.eyegrey_left.get());

		displayEye(image, 0, 0, 0, 2);
		pointTracker.draw(image);
		_headTracker.draw(image);
	}
}

void TrackingSystem::displayEye(IplImage *image, int baseX, int baseY, int stepX, int stepY) {
	CvSize eyeSize = EyeExtractor::eyesize;
	int eyeDX = EyeExtractor::eyedx;
	int eyeDY = EyeExtractor::eyedy;

	static IplImage *eyeGreyTemp = cvCreateImage(eyeSize, 8, 1);
	static FeatureDetector features(EyeExtractor::eyesize);
	static FeatureDetector features_left(EyeExtractor::eyesize);

	features.addSample(eyeExtractor.eyegrey.get());

	baseX *= 2 * eyeDX;
	baseY *= 2 * eyeDY;
	stepX *= 2 * eyeDX;
	stepY *= 2 * eyeDY;

	gazeTracker.draw(image, eyeDX, eyeDY);

	cvSetImageROI(image, cvRect(baseX, baseY, eyeDX * 2, eyeDY * 2));
	cvCvtColor(eyeExtractor.eyegrey.get(), image, CV_GRAY2RGB);

	cvSetImageROI(image, cvRect(baseX + stepX * 1, baseY + stepY * 1, eyeDX * 2, eyeDY * 2));
	cvCvtColor(eyeExtractor.eyegrey.get(), image, CV_GRAY2RGB);

	cvConvertScale(features.getMean().get(), eyeGreyTemp);
	cvSetImageROI(image, cvRect(baseX, baseY, eyeDX * 2, eyeDY * 2));
	cvCvtColor(eyeGreyTemp, image, CV_GRAY2RGB);

	// ONUR DUPLICATED CODE FOR LEFT EYE
	features_left.addSample(eyeExtractor.eyegrey_left.get());

	cvSetImageROI(image, cvRect(baseX + 100, baseY, eyeDX * 2, eyeDY * 2));
	cvCvtColor(eyeExtractor.eyegrey_left.get(), image, CV_GRAY2RGB);

	cvSetImageROI(image, cvRect(baseX + 100, baseY + stepY * 1, eyeDX * 2, eyeDY * 2));
	cvCvtColor(eyeExtractor.eyegrey_left.get(), image, CV_GRAY2RGB);

	cvConvertScale(features_left.getMean().get(), eyeGreyTemp);
	cvSetImageROI(image, cvRect(baseX + 100, baseY, eyeDX * 2, eyeDY * 2));
	cvCvtColor(eyeGreyTemp, image, CV_GRAY2RGB);

	//features.getVariance(eyeGreyTemp);
	//cvSetImageROI(image, cvRect(baseX, baseY+stepY * 2, eyeDX * 2, eyeDY * 2));
	//cvCvtColor(eyeGreyTemp, image, CV_GRAY2RGB);

	// compute the x-derivative
	//static IplImage *eyeGreyTemp1 = cvCreateImage(eyeSize, IPL_DEPTH_32F, 1);
	//static scoped_ptr<IplImage> eyeGreyTemp2(cvCreateImage(eyeSize, IPL_DEPTH_32F, 1));
	//static IplImage *eyeGreyTemp3 = cvCreateImage(eyeSize, IPL_DEPTH_32F, 1);
	//static IplImage *eyeGreyTemp4 = cvCreateImage(cvSize(7,5), IPL_DEPTH_32F, 1);

	//features.getMean(eyeGreyTemp1);
	//cvConvertScale(eyeExtractor.eyegrey, eyeGreyTemp2.get());
	//double distance = cvNorm(eyeGreyTemp1, eyeGreyTemp2.get(), CV_L2);
	//static BlinkDetector blinkDetector;
	//blinkDetector.update(eyeGreyTemp2);
	//cout << "distance: " << distance << " blink: " << blinkDetector.getState() << endl;

	//cvSetImageROI(eyeGreyTemp1, cvRect(2, 2, eyeDX * 2 - 6, eyeDY * 2 - 4));
	//cvMatchTemplate(eyeGreyTemp2.get(), eyeGreyTemp1, eyeGreyTemp4, CV_TM_SQDIFF);
	//cvResetImageROI(eyeGreyTemp1);

	//for (int i = 0; i < 5; i++) {
	//	cout << endl;
	//	for (int j = 0; j < 7; j++) {
	//		cout << cvGetReal2D(eyeGreyTemp4, i, j) / 1e6 << " ";
	//	}
	//}
	//cout << endl;

	//CvPoint maxPoint;
	//cvMinMaxLoc(eyeGreyTemp4, NULL, NULL, &maxPoint);
	//cout << "max: " << maxPoint.x << " " << maxPoint.y << endl;

	//cvSetImageROI(eyeExtractor.eyegrey, cvRect(maxPoint.x, maxPoint.y, eyeDX * 2 - 6, eyeDY * 2 - 4));
	//cvSetImageROI(image, cvRect(baseX, baseY + stepY * 3, eyeDX * 2 - 6, eyeDY * 2 - 4));
	//cvCvtColor(eyeExtractor.eyegrey, image, CV_GRAY2RGB);
	//cvResetImageROI(eyeExtractor.eyegrey);

	//Point maxPoint = subpixelMinimum(eyeGreyTemp4);
	//cout << "max: " << maxPoint.x << " " << maxPoint.y << endl;

	//pointTracker.currentpoints[0].x += 0.4 * (maxPoint.x - 3.0);
	//pointTracker.currentpoints[0].y += 0.4 * (maxPoint.y - 2.0);

	//cvSub(eyeExtractor.eyefloat, eyeGreyTemp1, eyeGreyTemp3);
	//cvSetImageROI(eyeGreyTemp1, cvRect(0, 0, eyeDX * 2 - 1, eyeDY * 2));
	//cvSetImageROI(eyeGreyTemp2, cvRect(1, 0, eyeDX * 2 - 1, eyeDY * 2));
	//cvCopy(eyeGreyTemp1, eyeGreyTemp2);
	//cvResetImageROI(eyeGreyTemp1);
	//cvResetImageROI(eyeGreyTemp2);
	//cvAddS(eyeGreyTemp1, cvScalar(128.0), eyeGreyTemp1);
	//cvSub(eyeGreyTemp1, eyeGreyTemp2, eyeGreyTemp1);

	//cvSetImageROI(image, cvRect(baseX, baseY + stepY * 2, eyeDX * 2, eyeDY * 2));
	//cvConvertScale(eyeGreyTemp1, eyeGreyTemp);

	//cvMul(eyeGreyTemp3, eyeGreyTemp1, eyeGreyTemp3);
	//cout << "x movement: " << cvAvg(eyeGreyTemp3).val[0] << endl;

	//cvCvtColor(eyeGreyTemp, image, CV_GRAY2RGB);

	cvResetImageROI(image);
}
