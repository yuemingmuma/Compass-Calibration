#include <iostream>
#include <fstream>
#include "MagCalibrator.h"
using namespace std;

extern calpara_t params;

void main()
{
	double sample[3];
	ifstream infile;
	char calflg;

	infile.open("magfile.txt");
	if (!infile)
	{
		cout<<"Mag Measurement data file does not exit!"<<endl;
		return;
	}

	while (!infile.eof())
	{
		infile>>sample[0]>>sample[1]>>sample[2];

		calflg = CompassCal(sample);
		if (calflg!=0)
		{
			break;
		}
	}

	cout<<endl;
	if (calflg == 1)
	{
		cout<<"恭喜，校正成功，参数结果如下："<<endl;
	} 
	else
	{
		cout<<"抱歉，校正失败，请重试，本次参数结果如下："<<endl;
	}

	cout<<"diag_x = "<<params.diag[0]<<"  diag_y = "<<params.diag[1]<<"  diag_z  = "<<params.diag[2]<<endl;
	cout<<"offdiag_x = "<<params.offdiag[0]<<"  offdiag_y = "<<params.offdiag[1]<<"  offdiag_z = "<<params.offdiag[2]<<endl;
	cout<<"offset_x  = "<<params.offset[0]<<" offset_y  = "<<params.offset[1]<<"  offset_z = "<<params.offset[2]<<endl;
	cout<<"radius    = "<<params.radius<<endl;
}
