#pragma once
#include <vtkSmartPointer.h>
#include <vtkPolyData.h>
#include <vtkImageData.h>
#include <vtkCallbackCommand.h>
#include <string>
#include <vtkNew.h>


class ReadDicom
{
	public:
		vtkNew<vtkImageData> ReadCT(const std::string& pathToDicom);
		
};


