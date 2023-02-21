
#include "fssimplewindow.h"
#include "ysglfontdata.h"

#include "opencv2/opencv.hpp"

#include "opencv2/objdetect.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc/imgproc.hpp"


void GetPixel(unsigned char pixel[4],
	unsigned char rgba[], int wid, int hei, int x, int y)
{
	int i = y * wid * 4 + x * 4;
	pixel[0] = rgba[i];
	pixel[1] = rgba[i + 1];
	pixel[2] = rgba[i + 2];
	pixel[3] = rgba[i + 3];
}
void SetPixel(unsigned char pixel[4],
	unsigned char rgba[], int wid, int hei, int x, int y)
{
	int i = y * wid * 4 + x * 4;
	rgba[i]= pixel[0];
	rgba[i + 1]= pixel[1];
	rgba[i + 2]= pixel[2];
	rgba[i + 3]= pixel[3];
}

bool OnCloseWindow(void *ptr)
{
	unsigned int* closeFlag = (unsigned int *)ptr;
	*closeFlag = 1;
	return false;
}

int main(void)
{
	const int MAX_NUM_VIDEO = 1;
	cv::VideoCapture* capPtr[MAX_NUM_VIDEO];
	cv::Mat frame[MAX_NUM_VIDEO];


#ifdef ANDROID
	const int baseCameraID = CV_CAP_ANDROID;
#else
	const int baseCameraID = 0;
#endif
	for (int i = 0; i < MAX_NUM_VIDEO; ++i)
	{
		capPtr[i] = new cv::VideoCapture(baseCameraID + i);
	}


	cv::QRCodeDetector detector;
	cv::Mat bbox, rectifiedImage;


	const unsigned int MODE0_WAIT_E_OR_L = 0;
	const unsigned int MODE1_SCAN_QR_CODE = 1;
	int mode = MODE0_WAIT_E_OR_L;

	const unsigned int ACTION_ENTER = 0;
	const unsigned int ACTION_LEAVE = 1;
	int action = ACTION_ENTER;

	bool qrCodeDetected = false;
	std::string prevQrCode;

	unsigned int closeWindowSignal = 0;
	FsRegisterCloseWindowCallBack(OnCloseWindow, &closeWindowSignal);

	FsOpenWindow(0, 0, 800, 600, 1);
	while (0==closeWindowSignal)
	{
		auto key = FsInkey();
		FsPollDevice();
		if (FSKEY_ESC == key)
		{
			break;
		}

		if (MODE0_WAIT_E_OR_L == mode)
		{
			if (FSKEY_E == key)
			{
				action = ACTION_ENTER;
				mode = MODE1_SCAN_QR_CODE;
			}
			else if (FSKEY_L == key)
			{
				action = ACTION_LEAVE;
				mode = MODE1_SCAN_QR_CODE;
			}
		}


		int imgWid = 0, imgHei = 0;
		unsigned char* rgba = nullptr;
		for (int i = 0; i < MAX_NUM_VIDEO; ++i)
		{
			if (capPtr[i]->isOpened())
			{
				(*capPtr[i]) >> frame[i];
				printf("Yes! [%d]\n", i);
				printf("%d x %d\n", frame[i].size[0], frame[i].size[1]);

				imgWid = frame[i].size[1];
				imgHei = frame[i].size[0];

				rgba = new unsigned char[imgWid * imgHei * 4];

				for (int y = 0; y < frame[i].size[0]; ++y)
				{
					for (int x = 0; x < frame[i].size[1]; ++x)
					{
						cv::Vec3b pix = frame[i].at<cv::Vec3b>(frame[i].size[0]-1-y, x);
						rgba[(y * imgWid + x) * 4    ] = pix[2];
						rgba[(y * imgWid + x) * 4 + 1] = pix[1];
						rgba[(y * imgWid + x) * 4 + 2] = pix[0];
						rgba[(y * imgWid + x) * 4 + 3] = 255;
					}
				}


				std::string data = 
					detector.detectAndDecode(frame[i], bbox, rectifiedImage);
				if (0 < data.size())
				{
					std::cout << "Found:" << data << std::endl;
					if (data != prevQrCode)
					{
						// Generate a file in the mapped Google drive folder here.
					}
					qrCodeDetected = true;
					prevQrCode = data;
				}
				else
				{
					qrCodeDetected = false;
				}

				if (MODE1_SCAN_QR_CODE == mode && true == qrCodeDetected)
				{
					glClearColor(0, 1, 0, 0);
				}
				else
				{
					glClearColor(1, 1, 1, 1);
				}
				glClear(GL_COLOR_BUFFER_BIT);
				if (MODE1_SCAN_QR_CODE == mode)
				{
					glRasterPos2d(0, 599);
					glDrawPixels(imgWid, imgHei, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
				}
				else
				{
					glRasterPos2i(100, 100);
					YsGlDrawFontBitmap20x28("[E]...Enter-Mode");
					glRasterPos2i(100, 140);
					YsGlDrawFontBitmap20x28("[L]...Leave-Mode");
				}
				FsSwapBuffers();
			}
			else
			{
				printf("Moo! [%d]\n", i);
			}
		}

		delete[] rgba;
	}

	for (int i = 0; i < MAX_NUM_VIDEO; ++i)
	{
		delete capPtr[i];
	}

	return 0;
}