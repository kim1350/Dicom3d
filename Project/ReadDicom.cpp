#define _CRT_SECURE_NO_WARNINGS
#include "ReadDicom.h"
#include <iostream>
#include "vtkDICOMDirectory.h"
#include "vtkDICOMItem.h"
#include "vtkStringArray.h"
#include "vtkDICOMReader.h"
#include <vtkSmoothPolyDataFilter.h>
#include <vtkPolyDataConnectivityFilter.h>

using namespace std;
vtkNew<vtkImageData> ReadDicom::ReadCT(const std::string& pathToDicom)
{
    vtkNew<vtkDICOMDirectory> dicomDirectory;

    dicomDirectory->SetDirectoryName(pathToDicom.c_str());
    dicomDirectory->SetScanDepth(1);
    dicomDirectory->Update();

    cout << "Read DICOM images located under " << pathToDicom << endl;
    cout << "Nbr of patients = " << dicomDirectory->GetNumberOfPatients() << ", ";
    cout << "Nbr of studies = " << dicomDirectory->GetNumberOfStudies() << ", ";

    const int& nbrOfSeries = dicomDirectory->GetNumberOfSeries();
    cout << "Nbr of series = " << nbrOfSeries << endl;
    for (int s = 0; s < nbrOfSeries; s++)
    {
        const vtkDICOMItem& dicomSeries_s = dicomDirectory->GetSeriesRecord(s);
        vtkStringArray* files_s = dicomDirectory->GetFileNamesForSeries(s);
        vtkIdType nbrOfSlices_s = files_s->GetNumberOfValues();

        cout << "(" << s << ")  :  " << nbrOfSlices_s << " files, name = " << dicomSeries_s.Get(DC::SeriesDescription).AsString() << endl;
    }


    // choose a particular dicom serie
    int s_nbr;
    vtkNew<vtkImageData> Null;
    if (nbrOfSeries == 1) // only one
    {
        s_nbr = 0;
    }
    else if (nbrOfSeries > 1) // multiple dicom series
    {
        cout << "Which DICOM series you wish to load? ";
        int scanRes = std::scanf("%d", &s_nbr);
        if (scanRes == 1 && (s_nbr < 0 || s_nbr >= nbrOfSeries))
        {
            cerr << "Wrong DICOM serie index" << endl;
            return Null;
        }
    }
    else
    {
        cerr << "No DICOM data in directory" << endl;
        return Null;
    }


    s_nbr = 0;
    // load dicom serie
    const vtkDICOMItem& selected_serie = dicomDirectory->GetSeriesRecord(s_nbr);
    cout << endl << "Load serie " << s_nbr << ", " << selected_serie.Get(DC::SeriesDescription).AsString() << endl;
    vtkStringArray* files_s = dicomDirectory->GetFileNamesForSeries(s_nbr);
    vtkNew<vtkDICOMReader> reader;
    reader->SetFileNames(dicomDirectory->GetFileNamesForSeries(s_nbr));
    //cout << dicomDirectory->GetFileNamesForSeries(s_nbr);

    reader->Update();
    vtkNew<vtkImageData> rawVolumeData;
    rawVolumeData->DeepCopy(reader->GetOutput());

    return  rawVolumeData;
}

void ReadDicom::filterSmallObj(vtkSmartPointer<vtkPolyData> mesh, double ratio, vtkSmartPointer<vtkCallbackCommand> callback)
{
    
    cout << "Remove small connected objects: Size ratio = " << fixed << setprecision(3) << ratio << endl;

    vtkNew<vtkPolyDataConnectivityFilter> connectivityFilter ;

    cout << connectivityFilter->GetClassName() <<" progress:" << endl;
    /*
    if (callback.Get() != NULL)
    {
        connectivityFilter->AddObserver(vtkCommand::ProgressEvent, callback);
    }
    */
    connectivityFilter->SetInputData(mesh);
    connectivityFilter->SetExtractionModeToAllRegions();
    connectivityFilter->Update();
    cout << "0.0%..";
    // remove objects consisting of less than ratio vertexes of the biggest object
    vtkIdTypeArray* regionSizes = connectivityFilter->GetRegionSizes();
    long maxSize = 0;
    for (int regions = 0; regions < connectivityFilter->GetNumberOfExtractedRegions(); regions++)
        if (regionSizes->GetValue(regions) > maxSize)
            maxSize = regionSizes->GetValue(regions);

    cout << "50.0%..";
    // append regions of sizes over the threshold
    connectivityFilter->SetExtractionModeToSpecifiedRegions();
    for (int regions = 0; regions < connectivityFilter->GetNumberOfExtractedRegions(); regions++)
        if (regionSizes->GetValue(regions) > maxSize * ratio)
            connectivityFilter->AddSpecifiedRegion(regions);
    
    connectivityFilter->Update();
    mesh->DeepCopy(connectivityFilter->GetOutput());
    cout << "100% done" << endl;
    
}


