#define _CRT_SECURE_NO_WARNINGS

#include <time.h>
#include <ctype.h>
#include <fstream>

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


class CommandParameterInfo
{
public:
	std::string storeDir="./Attendance";

	bool RecognizeCommandParameters(int ac,char *av[])
	{
		for(int i=1; i<ac; ++i)
		{
			std::string OPT=av[i];
			for(auto &c : OPT)
			{
				c=toupper(c);
			}
			if("-STOREDIR"==av[i])
			{
				if(i+1<ac)
				{
					storeDir=av[i+1];
					++i;
				}
				else
				{
					fprintf(stderr,"Too few arguments for parameter %s\n",av[i]);
					return false;
				}
			}
			else
			{
				fprintf(stderr,"Unrecognized parameter %s\n",av[i]);
				return false;
			}
		}
		return true;
	}
};

int main(int argc,char *argv[])
{
	CommandParameterInfo cpi;
	if(true!=cpi.RecognizeCommandParameters(argc,argv))
	{
		fprintf(stderr,"Command-parameter error.\n");
		return 1;
	}


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

	bool qrCodeDetected = false,errorOccured=false;
	std::string prevQrCode;

	std::string message;

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
				printf("video %d: %d x %d\n", i, frame[i].size[0], frame[i].size[1]);

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


				std::string data;
				try
				{
					data=detector.detectAndDecode(frame[i], bbox, rectifiedImage);
				}
				catch(cv::Exception &e)
				{
					std::cout << e.msg << std::endl;
					data.clear();
				}
				if (0 < data.size())
				{
					std::cout << "Found:" << data << std::endl;
					if (data != prevQrCode)
					{
						// Generate a file in the mapped Google drive folder here.

						std::string output;
						auto t=time(NULL);
						auto tm=localtime(&t);

						char fmt[256];
						sprintf(fmt,"%04d%02d%02d_%02d%02d_",
						   1900+tm->tm_year,
						   1+tm->tm_mon,
						   tm->tm_mday,
						   tm->tm_hour,
						   tm->tm_min);

						output=fmt;
						if(ACTION_ENTER==action)
						{
							output+="ENTER_";
						}
						else
						{
							output+="LEAVE_";
						}

						output+=data;

						std::string fileName=cpi.storeDir;
						if(0<fileName.size() && '/'!=fileName.back() && '\\'!=fileName.back())
						{
							fileName.push_back('/');
						}
						fileName+=output;

						std::cout << fileName << std::endl;
						std::ofstream ofp(fileName);
						if(ofp.is_open())
						{
							ofp << output << std::endl;
							ofp.close();
							errorOccured=false;
							message="Registered Enter/Leave Log for "+data;
							std::cout << message << std::endl;
						}
						else
						{
							errorOccured=true;
							message="ERROR!  Cannot Register Enter/Leave Log!";
							std::cout << message << std::endl;
						}
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
					if(true==errorOccured)
					{
						glClearColor(1, 0, 0, 0);
					}
					else
					{
						glClearColor(0, 1, 0, 0);
					}
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
					std::cout << message << std::endl;
					glRasterPos2i(0,32);
					YsGlDrawFontBitmap20x28(message.data());
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