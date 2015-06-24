
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

#include <QImage>
#include <QFileDialog>
#include <QMessageBox>
#include <QString>

#include "reconstruction3d.h"

reconstruction3D::reconstruction3D()
{
    //viewer = boost::shared_ptr<pcl::visualization::PCLVisualizer>(new pcl::visualization::PCLVisualizer("Simple Cloud Viewer"));
    cloud = pcl::PointCloud<pcl::PointXYZ>::Ptr(new pcl::PointCloud<pcl::PointXYZ>);


    //viewer->setShowFPS(true);
}

reconstruction3D::~reconstruction3D()
{

}
/*
void testPowerCrust(){
    QString file = QFileDialog::getOpenFileName( 0,
                               QString("Choose Depth map"),           // window name
                               "../",                                       // relative folder
                               QString("Depth Images(*.PNG *.JPG);;Point clouds (*.PCD)"));  // filetype
    if (!file.isNull()){
        vtkSmartPointer<vtkPLYReader> PlyReader = vtkSmartPointer<vtkPLYReader>::New();
        PlyReader->SetFileName ( file.toStdString().c_str() );
        PlyReader->Update();
        std::cout << "Number of points " << PlyReader->GetOutput()->GetPoints()->GetNumberOfPoints() << std::endl;

        vtkSmartPointer<vtkPowerCrustSurfaceReconstruction> SurfaceReconstructor = vtkSmartPointer<vtkPowerCrustSurfaceReconstruction>::New();
        SurfaceReconstructor->SetInput ( PlyReader->GetOutput() );
        SurfaceReconstructor->Update();

        std::string OutFileName = "Output";
        OutFileName.append ( ".ply" );
        std::cout << "Writing: " << OutFileName << std::endl;

        vtkSmartPointer<vtkPLYWriter> PlyWriter = vtkSmartPointer<vtkPLYWriter>::New();
        PlyWriter->SetFileName ( OutFileName.c_str() );
        PlyWriter->SetInput ( SurfaceReconstructor->GetOutput() );
        PlyWriter->Write();
    }
}*/


void reconstruction3D::addDepthMap(){
    // QImage file
    QString file = QFileDialog::getOpenFileName( 0,
                               QString("Choose Depth map"),           // window name
                               "../",                                       // relative folder
                               QString("Depth Images(*.PNG *.JPG);;Point clouds (*.PCD)"));  // filetype
    if (!file.isNull()){
        depthmaps.push_back(cv::imread(file.toStdString()));
    }
}

void reconstruction3D::calculatePointCloud(){

    // set initial settings of point cloud
    cloud->width    += depthmaps[0].cols;
    cloud->height   += depthmaps[0].rows;
    cloud->is_dense = true;
    cloud->points.resize (cloud->width * cloud->height * depthmaps.size());
    int depth_idx = 0;
    int cn = depthmaps[0].channels();

    // Fill in the cloud data
    for (int i=0; i<depthmaps.size(); i++){
        for(int y=0; y<cloud->height; y++){
            for(int x=0; x<cloud->width; x++){
                pcl::PointXYZ& pt = cloud->points[depth_idx++];
                pt.z = -depthmaps[i].data[y*depthmaps[i].cols*cn + x*cn + 0]/255.0f;// / 24.135f / 2;// / 255.0f * 2;
                pt.x = x * 1.0f / cloud->width + 0.2*i;
                pt.y = y * 1.0f / cloud->height;
            }
        }
    }
}

void reconstruction3D::approximateNormals(){
    if (cloud->empty())
        return;
    // Create a KD-Tree
    //pcl::search::KdTree<pcl::PointXYZ>::Ptr tree (new pcl::search::KdTree<pcl::PointXYZ>);
    //cloud_normals = pcl::PointCloud<pcl::PointNormal>::Ptr(new pcl::PointCloud<pcl::PointNormal>);

    // Init object (second point type is for the normals, even if unused)
    pcl::MovingLeastSquares<pcl::PointXYZ, pcl::PointXYZ> mls;

    mls.setComputeNormals (true);

    // Set parameters
    mls.setInputCloud (cloud);
    mls.setSearchRadius (0.03);
    mls.setPolynomialFit (true);

    mls.setPolynomialOrder (2);
    mls.setUpsamplingMethod (pcl::MovingLeastSquares<pcl::PointXYZ, pcl::PointXYZ>::SAMPLE_LOCAL_PLANE);
    //mls.setUpsamplingMethod(pcl::MovingLeastSquares<pcl::PointXYZ,pcl::PointXYZ>::VOXEL_GRID_DILATION);
    mls.setDilationVoxelSize(0.005); // none
    mls.setUpsamplingRadius (0.005); // 0.005 : higher -> fills up holes
    mls.setUpsamplingStepSize (0.003); // 0.003

    pcl::PointCloud<pcl::PointXYZ>::Ptr cloud_smoothed (new pcl::PointCloud<pcl::PointXYZ> ());
    mls.process (*cloud_smoothed);

    pcl::NormalEstimationOMP<pcl::PointXYZ, pcl::Normal> ne;
    ne.setNumberOfThreads (8);
    ne.setInputCloud (cloud_smoothed);
    ne.setRadiusSearch (0.25); // 0.01 : higher -> smoother?!
    Eigen::Vector4f centroid;
    compute3DCentroid (*cloud_smoothed, centroid);
    ne.setViewPoint (centroid[0], centroid[1], centroid[2]);

    pcl::PointCloud<pcl::Normal>::Ptr cloud_normals (new pcl::PointCloud<pcl::Normal> ());
    ne.compute (*cloud_normals);

    /*for (size_t i = 0; i < cloud_normals->size (); ++i)
    {
     cloud_normals->points[i].normal_x *= -1;
     cloud_normals->points[i].normal_y *= -1;
     cloud_normals->points[i].normal_z *= -1;
    }*/
    cloud_smoothed_normals = pcl::PointCloud<pcl::PointNormal>::Ptr (new pcl::PointCloud<pcl::PointNormal> ());
    concatenateFields (*cloud_smoothed, *cloud_normals, *cloud_smoothed_normals);
}

void reconstruction3D::calculateMesh(){


    pcl::search::KdTree<pcl::PointNormal>::Ptr tree2 (new pcl::search::KdTree<pcl::PointNormal>);
    tree2->setInputCloud (cloud_smoothed_normals);


    // Initialize objects
    pcl::GreedyProjectionTriangulation<pcl::PointNormal> gp3;

    // Set the maximum distance between connected points (maximum edge length)
    gp3.setSearchRadius (0.025); // 0.025

    // Set typical values for the parameters
    gp3.setMu (std::max(this->mesh_mu,12.5)); // 2.5
    gp3.setMaximumNearestNeighbors (100);   // 100
    gp3.setMaximumSurfaceAngle(M_PI/180.0f * 45); // 90 degrees, before 45
    gp3.setMinimumAngle(M_PI/180.0f * 10); // 10 degrees
    gp3.setMaximumAngle(M_PI/180.0f * 120); // 90 degrees, before 120
    gp3.setNormalConsistency(false);

    // Get result
    gp3.setInputCloud (cloud_smoothed_normals);
    gp3.setSearchMethod (tree2);
    gp3.reconstruct (triangles);

    /*
    // Poisson always creates closed/"watertight" meshes
    if (cloud_smoothed_normals->empty())
        return;

    pcl::Poisson<pcl::PointNormal> poisson;
    poisson.setDepth (9);
    poisson.setInputCloud(cloud_smoothed_normals);
    poisson.reconstruct (triangles);
    */
}

void reconstruction3D::showPointCloud(){
    pcl::visualization::CloudViewer cloud_viewer("Simple Cloud Viewer");
    is_viewing = false;
    cloud_viewer.showCloud (cloud);
    is_viewing = true;
    while (!cloud_viewer.wasStopped() && is_viewing)
    {

    }
    is_viewing = false;
}

void reconstruction3D::saveCloudToPLY(){
    if (!cloud->empty()){
        QString filename = QFileDialog::getSaveFileName(0,
                                                    "Save File",
                                                    QDir::currentPath(),
                                                    "Point Clouds (*.ply)");
        pcl::io::savePLYFile(filename.toStdString(), *cloud);
    }
}

void reconstruction3D::showMesh(){
    // Additional vertex information
    //std::vector<int> parts = gp3.getPartIDs();
    //std::vector<int> states = gp3.getPointStates();
    //viewer.removeAllShapes();

    /*pcl::visualization::CloudViewer viewer ("Simple Cloud Viewer");
    viewer.showCloud (cloud);
    while (!viewer.wasStopped ())
       {
       }
    */

    pcl::visualization::PCLVisualizer* viewer;
    viewer = new pcl::visualization::PCLVisualizer("3D Viewer");
    viewer->addPolygonMesh(triangles);
    while (!viewer->wasStopped ()){
        viewer->spinOnce ();
        Sleep(100);
    }
    HWND hWnd = (HWND)viewer->getRenderWindow()->GetGenericWindowId();
    delete viewer;
    DestroyWindow(hWnd);
    is_viewing = false;
}

void reconstruction3D::setMeshMu(double mu){
    this->mesh_mu = mu;
}
