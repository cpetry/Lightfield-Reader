#ifndef RECONSTRUCTION3D_H
#define RECONSTRUCTION3D_H
#include <iostream>
#include <string>

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

#include <QString>
#include <QObject>

#include <pcl/surface/gp3.h>
#include <pcl/surface/mls.h>
#include <pcl/surface/poisson.h>
#include <pcl/features/normal_3d.h>
#include <pcl/features/normal_3d_omp.h>
#include <pcl/io/ply_io.h>
#include <pcl/io/pcd_io.h>
#include <pcl/point_types.h>
#include <pcl/registration/icp.h>
#include <pcl/visualization/pcl_visualizer.h>
#include <pcl/visualization/cloud_viewer.h>
#include <pcl/console/time.h>
//#include <pcl/registration/impl/icp.hpp>
//#include <pcl/search/kdtree.h>

#include <vtkPLYReader.h>
#include <vtkRenderWindow.h>
#include <vtkPLYWriter.h>
#include <vtkSmartPointer.h>

#include <Windows.h>

class reconstruction3D : public QObject
{
    Q_OBJECT

public:
    reconstruction3D();
    ~reconstruction3D();

public slots:
    void calcPointCloudFromMultipleDepthMaps();
    void calculatePointCloud(std::vector<cv::Mat> depthmaps);
    void approximateNormals();
    void calculateMesh();
    void showMesh();
    void showPointCloud();
    void saveCloudToPLY();
    void setMeshMu(double mu);


private:
    void calculateArtificialPointCloud(std::vector<cv::Mat> depthmaps);
    void calculateRealPointCloud(std::vector<cv::Mat> depthmaps);
    void saveToPly(QString filename);

    pcl::PolygonMesh triangles;
    pcl::PointCloud<pcl::PointXYZ>::Ptr cloud;
    //pcl::PointCloud<pcl::PointNormal>::Ptr cloud_smoothed_normals;
    boost::shared_ptr<pcl::visualization::PCLVisualizer> viewer;
     bool is_viewing = false;
    double mesh_mu = 2.5;
};

#endif // RECONSTRUCTION3D_H
