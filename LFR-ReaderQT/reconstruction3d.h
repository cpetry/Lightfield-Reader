#ifndef RECONSTRUCTION3D_H
#define RECONSTRUCTION3D_H


#include <pcl/point_types.h>
#include <pcl/io/pcd_io.h>
#include <pcl/kdtree/kdtree_flann.h>
#include <pcl/features/normal_3d.h>
#include <pcl/surface/gp3.h>
#include <pcl/visualization/pcl_visualizer.h>
#include <pcl/visualization/cloud_viewer.h>
#include <pcl/surface/mls.h>
#include <pcl/surface/poisson.h>
#include <pcl/features/normal_3d_omp.h>
#include <pcl/io/ply_io.h>

#include <vtkPLYReader.h>
#include <vtkRenderWindow.h>
#include <vtkPLYWriter.h>
#include <vtkSmartPointer.h>

#include <QString>
#include <QObject>

#include <Windows.h>

class reconstruction3D : public QObject
{
    Q_OBJECT

public:
    reconstruction3D();
    ~reconstruction3D();

public slots:
    void addDepthMap();
    void calculatePointCloud();
    void approximateNormals();
    void calculateMesh();
    void showMesh();
    void showPointCloud();
    void saveCloudToPLY();
    void setMeshMu(double mu);

private:
    std::vector<cv::Mat> depthmaps;
    pcl::PolygonMesh triangles;
    pcl::PointCloud<pcl::PointXYZ>::Ptr cloud;
    pcl::PointCloud<pcl::PointNormal>::Ptr cloud_smoothed_normals;
    boost::shared_ptr<pcl::visualization::PCLVisualizer> viewer;
     bool is_viewing = false;
    double mesh_mu = 2.5;
};

#endif // RECONSTRUCTION3D_H
