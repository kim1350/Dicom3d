#ifndef INITIAL_OPENGL
#define INITIAL_OPENGL

#include <vtkAutoInit.h>
VTK_MODULE_INIT(vtkRenderingOpenGL2)
VTK_MODULE_INIT(vtkInteractionStyle)
VTK_MODULE_INIT(vtkRenderingContextOpenGL2)
VTK_MODULE_INIT(vtkRenderingFreeType)
#endif

#include <vtkActor.h>
#include <vtkCamera.h>
#include <vtkNamedColors.h>
#include <vtkNew.h>
#include <vtkOutlineFilter.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkVersion.h>
#include <vtkFlyingEdges3D.h>
#include <vtkImageData.h>
#include <vtkSmoothPolyDataFilter.h>
#include <vtkImageThreshold.h>
#include <vtkAlgorithm.h>
#include <vtkObject.h>
#include <vtkCallbackCommand.h>

#include "ReadDicom.h"
#include <array>
using namespace std;

void myVtkProgressCallback(vtkObject* caller, long unsigned int /*eventId*/, void* /*clientData*/, void* /*callData*/)
{
    // display progress in terminal
    vtkAlgorithm* filter = static_cast<vtkAlgorithm*>(caller);
    if (filter->GetProgress() == 0) {
        cout << filter->GetClassName() << " progress:"<<endl;     
    }
    if (filter->GetProgress() > 0.999)
        cout << "100% done!"<<endl;
    else
        cout <<fixed << setprecision(1) << filter->GetProgress() * 100 << "%..";
    cout <<flush;
}


int main(int argc, char* argv[])
{
    cout << "write -h for help" << endl << endl;
    if (argc < 4 or argv[0]=="-h")
    {
        cout << "Usage: " << argv[0] << endl;
        cout << "Minimum example:" << endl;
        cout << "project -p E:\\dicompath -o test.stl" << endl;
        cout << "1) -p path to Dicom directory"<<endl;
        cout << "2) -mv iso value main" << endl;
        cout << "3) -uv iso value upper" << endl; 
        cout << "4) -s smoothing mesh. Write the iteration number" << endl;
        cout << "5) -o file name when saving" << endl;
        cout << "Optional parameters: iso values, smooth iteration number" << endl; 
        cout << "Example: -p E:\\dicompath -mv 500 -uv 1000 -s 10 -o test.stl" << endl;
        return EXIT_FAILURE;
    }

    string path, filename;
    int upValue = 0, mainValue = 0, smoothIter = 0;
    

    for (int a = 1; a < argc; a++)
    {
        string cArg(argv[a]);
        if (cArg.compare("-p") == 0)
        {   
            a++;
            if (a < argc)
            {
                path = string(argv[a]);
            }
            else
            {
                cout << "parameters are set incorrectly";
                return false;
            }
        }
        if (cArg.compare("-mv") == 0)
        {    
            a++;
            if (a < argc)
            {
                mainValue = atoi(argv[a]);
            }
            else
            {
                cout << "parameters are set incorrectly";
                return false;
            }
        }
        if (cArg.compare("-uv") == 0)
        {
            a++;
            if (a < argc)
            {
                upValue = atoi(argv[a]);
            }
            else
            {
                cout << "parameters are set incorrectly";
                return false;
            }
        }
        if (cArg.compare("-s") == 0)
        {
            a++;
            if (a < argc)
            {
                smoothIter = atoi(argv[a]);
            }
            else
            {
                cout << "parameters are set incorrectly";
                return false;
            }
        }
        if (cArg.compare("-o") == 0)
        {
            a++;
            if (a < argc)
            {
                filename = string(argv[a]);
            }
            else
            {
                cout << "parameters are set incorrectly";
                return false;
            }
        }
    }
    vtkNew<vtkCallbackCommand> m_vtkCallback;
    m_vtkCallback->SetCallback(myVtkProgressCallback);

    ReadDicom read;
    ////TUT
    vtkNew<vtkImageData> reader;
    reader->DeepCopy(read.ReadCT(path));

    //cout << endl << endl;
    vtkNew<vtkNamedColors> colors;

    array<unsigned char, 4> skinColor{ {240, 184, 160, 255} };
    colors->SetColor("SkinColor", skinColor.data());
    array<unsigned char, 4> backColor{ {255, 229, 200, 255} };
    colors->SetColor("BackfaceColor", backColor.data());
    array<unsigned char, 4> bkg{ {51, 77, 102, 255} };
    colors->SetColor("BkgColor", bkg.data());

    // Create the renderer, the render window, and the interactor. The renderer
    // draws into the render window, the interactor enables mouse- and
    // keyboard-based interaction with the data within the render window.
    //
    vtkNew<vtkRenderer> aRenderer;
    vtkNew<vtkRenderWindow> renWin;
    renWin->AddRenderer(aRenderer);

    vtkNew<vtkRenderWindowInteractor> iren;
    iren->SetRenderWindow(renWin);
   
   

    if (upValue && mainValue) {
        vtkNew<vtkImageThreshold> imageThreshold;
        imageThreshold->SetInputData(reader);
        imageThreshold->ThresholdByUpper(upValue);
        imageThreshold->ReplaceInOn();
        imageThreshold->SetInValue(mainValue - 1); // mask voxels with a value lower than the lower threshold
        imageThreshold->Update();
        reader->DeepCopy(imageThreshold->GetOutput());
    }
    
    

    vtkNew<vtkFlyingEdges3D> skinExtractor;
    if (m_vtkCallback.Get() != NULL)
    {
        skinExtractor->AddObserver(vtkCommand::ProgressEvent, m_vtkCallback);
    }
    skinExtractor->SetInputData(reader);
    if (mainValue)
    {
        skinExtractor->SetValue(0, mainValue);
    }
    skinExtractor->Update();
    vtkNew<vtkPolyData> polyData;
    polyData->DeepCopy(skinExtractor->GetOutput());
    skinExtractor->Delete();

    ReadDicom filterR;
    filterR.filterSmallObj(polyData, 0.005, m_vtkCallback);

    if (smoothIter) {
        vtkNew<vtkSmoothPolyDataFilter> smoother;
        if (m_vtkCallback.Get() != NULL)
        {
            smoother->AddObserver(vtkCommand::ProgressEvent, m_vtkCallback);
        }
        smoother->SetInputData(polyData);
        smoother->SetNumberOfIterations(smoothIter);
        smoother->SetFeatureAngle(45);
        smoother->SetRelaxationFactor(0.05);
        smoother->Update();
        polyData->DeepCopy(smoother->GetOutput());
    }
    
    
    vtkNew<vtkPolyDataMapper> skinMapper;
    skinMapper->SetInputData(polyData);
    skinMapper->ScalarVisibilityOff();
  
    vtkNew<vtkActor> skin;
    skin->SetMapper(skinMapper);
    skin->GetProperty()->SetDiffuseColor(colors->GetColor3d("SkinColor").GetData());

    vtkNew<vtkProperty> backProp;
    backProp->SetDiffuseColor(colors->GetColor3d("BackfaceColor").GetData());
    skin->SetBackfaceProperty(backProp);

    // An outline provides context around the data.
    //
    vtkNew<vtkOutlineFilter> outlineData;
    outlineData->SetInputData(reader);

    vtkNew<vtkPolyDataMapper> mapOutline;
    mapOutline->SetInputConnection(outlineData->GetOutputPort());

    vtkNew<vtkActor> outline;
    outline->SetMapper(mapOutline);
    outline->GetProperty()->SetColor(colors->GetColor3d("Black").GetData());

    // It is convenient to create an initial view of the data. The FocalPoint
    // and Position form a vector direction. Later on (ResetCamera() method)
    // this vector is used to position the camera to look at the data in
    // this direction.
    vtkNew<vtkCamera> aCamera;
    aCamera->SetViewUp(0, 0, -1);
    aCamera->SetPosition(0, -1, 0);
    aCamera->SetFocalPoint(0, 0, 0);
    aCamera->ComputeViewPlaneNormal();
    aCamera->Azimuth(30.0);
    aCamera->Elevation(30.0);

    // Actors are added to the renderer. An initial camera view is created.
    // The Dolly() method moves the camera towards the FocalPoint,
    // thereby enlarging the image.
    aRenderer->AddActor(outline);
    aRenderer->AddActor(skin);
    aRenderer->SetActiveCamera(aCamera);
    aRenderer->ResetCamera();
    aCamera->Dolly(1.5);

    // Set a background color for the renderer and set the size of the
    // render window (expressed in pixels).
    aRenderer->SetBackground(colors->GetColor3d("BkgColor").GetData());
    renWin->SetSize(1280, 720);
    renWin->SetWindowName("Diplom");

    // Note that when camera movement occurs (as it does in the Dolly()
    // method), the clipping planes often need adjusting. Clipping planes
    // consist of two planes: near and far along the view direction. The
    // near plane clips out objects in front of the plane; the far plane
    // clips out objects behind the plane. This way only what is drawn
    // between the planes is actually rendered.
    aRenderer->ResetCameraClippingRange();

    // Exporter
    //vtkNew<vtkOBJExporter> exporter;
    //exporter->SetFilePrefix("E:/test");
    //exporter->SetActiveRenderer(aRenderer);
    //exporter->SetRenderWindow(renWin);
    //exporter->Write();
   
    
    // Initialize the event loop and then start it.
    renWin->Render();
    iren->Initialize();
    iren->Start();


    return EXIT_SUCCESS;
}

